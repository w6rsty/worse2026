#!/usr/bin/env bash
#
# track.sh — atomic, git-backed task tracker for the worse::core::container effort.
#
# Each task is a transaction: begin records a baseline commit, work happens, done
# commits it as one unit (after the agent has verified build+tests are green), and
# abort hard-resets back to the baseline. An interrupted task is an in_progress tx
# with no commit — `resume` surfaces it so the next session can continue or roll back.
#
# Storage (committed to the repo, so it is stable + shareable across sessions):
#   .tracker/container/STATUS.md     dashboard
#   .tracker/container/TASKS.md      work-breakdown (source of truth)
#   .tracker/container/DECISIONS.md  locked-in decisions
#   .tracker/container/PITFALLS.md   accumulated gotchas
#   .tracker/container/journal/      per-session append-only notes
#   .tracker/container/tx/<id>.json  one transaction record per task
#   .tracker/container/.active       id of the currently active task (for hooks)
#
# Usage:
#   track.sh begin <id> <title...>     start a task transaction
#   track.sh checkpoint [id]           snapshot WIP (non-destructive); defaults to active
#   track.sh done [id]                 commit the task atomically (run AFTER green build+tests)
#   track.sh abort [id] --yes          hard-reset to the task's baseline (explicit, destructive)
#   track.sh resume                    list interrupted (in_progress) transactions
#   track.sh status                    print dashboard + active task + open transactions
#   track.sh pitfall <text...>         append a gotcha to PITFALLS.md
#   track.sh task <id> <state>         set a TASKS.md row state (pending|in_progress|done|blocked)
#   track.sh hook-postedit             (hook) read PostToolUse JSON on stdin, log touched file
#
set -euo pipefail

ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
TDIR="$ROOT/.tracker/container"
TX="$TDIR/tx"
ACTIVE_FILE="$TDIR/.active"
JOURNAL_DIR="$TDIR/journal"

now()   { date -u +%Y-%m-%dT%H:%M:%SZ; }
today() { date +%Y-%m-%d; }
head_sha() { git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo "0000000"; }
active_id() { [ -f "$ACTIVE_FILE" ] && cat "$ACTIVE_FILE" || echo ""; }
journal() { mkdir -p "$JOURNAL_DIR"; echo "- $(now) | $*" >> "$JOURNAL_DIR/SESSION-$(today).md"; }
die() { echo "track: $*" >&2; exit 1; }

ensure_dirs() { mkdir -p "$TX" "$JOURNAL_DIR"; }

cmd_begin() {
    ensure_dirs
    local id="${1:-}"; shift || true
    local title="$*"
    [ -n "$id" ] || die "begin needs a <id>"
    [ -n "$title" ] || title="$id"
    local f="$TX/$id.json"
    if [ -f "$f" ] && [ "$(jq -r .status "$f")" = "committed" ]; then
        die "task '$id' already committed; pick a new id"
    fi
    jq -n --arg id "$id" --arg title "$title" --arg base "$(head_sha)" --arg t "$(now)" \
        '{id:$id,title:$title,baseline:$base,status:"in_progress",startedAt:$t,files:[],commit:null,wipStash:null}' \
        > "$f"
    echo "$id" > "$ACTIVE_FILE"
    journal "BEGIN $id — $title (baseline $(head_sha | cut -c1-9))"
    echo "▶ begun '$id' — $title  (baseline $(head_sha | cut -c1-9))"
}

cmd_checkpoint() {
    local id="${1:-$(active_id)}"
    [ -n "$id" ] || { echo "(no active task — nothing to checkpoint)"; return 0; }
    local f="$TX/$id.json"
    [ -f "$f" ] || { echo "(no tx for '$id')"; return 0; }
    local stash=""
    if ! git -C "$ROOT" diff --quiet || ! git -C "$ROOT" diff --cached --quiet; then
        stash="$(git -C "$ROOT" stash create "wip:$id" 2>/dev/null || true)"
    fi
    local tmp; tmp="$(mktemp)"
    jq --arg t "$(now)" --arg s "$stash" '.checkpointedAt=$t | (if $s=="" then . else .wipStash=$s end)' \
        "$f" > "$tmp" && mv "$tmp" "$f"
    [ -n "$stash" ] && journal "CHECKPOINT $id (wip stash ${stash:0:9})" || true
    echo "● checkpoint '$id'${stash:+ (wip ${stash:0:9})}"
}

cmd_done() {
    local id="${1:-$(active_id)}"
    [ -n "$id" ] || die "done needs a <id> (or an active task)"
    local f="$TX/$id.json"
    [ -f "$f" ] || die "no tx for '$id' — begin it first"
    # Stage ONLY the tracker bookkeeping + this task's recorded files. Never `git add -A`
    # — that would sweep up unrelated work-in-progress sitting in the tree.
    git -C "$ROOT" add -- "$ROOT/.tracker" 2>/dev/null || true
    jq -r '.files[]?' "$f" | while IFS= read -r rel; do
        [ -n "$rel" ] || continue
        git -C "$ROOT" add -- "$ROOT/$rel" 2>/dev/null || true
    done
    if git -C "$ROOT" diff --cached --quiet; then
        echo "⚠ nothing staged for '$id' — marking committed against current HEAD"
    else
        local title; title="$(jq -r .title "$f")"
        git -C "$ROOT" commit -q -m "[container] $id: $title" \
            -m "Co-Authored-By: Claude Opus 4.8 (1M context) <noreply@anthropic.com>"
    fi
    local sha; sha="$(head_sha)"
    local tmp; tmp="$(mktemp)"
    jq --arg t "$(now)" --arg c "$sha" '.status="committed" | .committedAt=$t | .commit=$c' \
        "$f" > "$tmp" && mv "$tmp" "$f"
    [ "$(active_id)" = "$id" ] && rm -f "$ACTIVE_FILE" || true
    journal "DONE $id (commit ${sha:0:9})"
    echo "✓ committed '$id'  (${sha:0:9})"
}

cmd_abort() {
    local id="${1:-$(active_id)}"; shift || true
    local yes="no"; [ "${1:-}" = "--yes" ] && yes="yes"
    [ -n "$id" ] || die "abort needs a <id> (or an active task)"
    local f="$TX/$id.json"
    [ -f "$f" ] || die "no tx for '$id'"
    local base; base="$(jq -r .baseline "$f")"
    if [ "$yes" != "yes" ]; then
        echo "abort '$id' will surgically restore this task's files to baseline ${base:0:9}:"
        jq -r '.files[]? | "  - " + .' "$f"
        echo "re-run with --yes to confirm (discards this task's uncommitted edits only;"
        echo "unrelated work-in-progress in the tree is left untouched)"
        return 1
    fi
    # File-scoped rollback: restore tracked files to baseline, delete files new since baseline.
    # No `git reset --hard` / `git clean` on the whole tree — unrelated WIP stays intact.
    jq -r '.files[]?' "$f" | while IFS= read -r rel; do
        [ -n "$rel" ] || continue
        if git -C "$ROOT" cat-file -e "$base:$rel" 2>/dev/null; then
            git -C "$ROOT" checkout "$base" -- "$rel" 2>/dev/null || true
        else
            git -C "$ROOT" rm -f --quiet -- "$rel" 2>/dev/null || rm -f "$ROOT/$rel"
        fi
    done
    local tmp; tmp="$(mktemp)"
    jq --arg t "$(now)" '.status="rolled_back" | .rolledBackAt=$t' "$f" > "$tmp" && mv "$tmp" "$f"
    [ "$(active_id)" = "$id" ] && rm -f "$ACTIVE_FILE" || true
    journal "ABORT $id (restored files to ${base:0:9})"
    echo "↺ rolled back '$id' files to ${base:0:9}"
}

cmd_resume() {
    ensure_dirs
    local found="no"
    for f in "$TX"/*.json; do
        [ -e "$f" ] || continue
        if [ "$(jq -r .status "$f")" = "in_progress" ]; then
            found="yes"
            echo "⏳ $(jq -r '.id+"  — "+.title+"  (baseline "+.baseline[0:9]+", started "+.startedAt+")"' "$f")"
            local nfiles; nfiles="$(jq -r '.files|length' "$f")"
            [ "$nfiles" -gt 0 ] && echo "     files: $(jq -r '.files|join(", ")' "$f")" || true
            local wip; wip="$(jq -r '.wipStash // empty' "$f")"
            [ -n "$wip" ] && echo "     wip stash: ${wip:0:9} (git stash apply ${wip:0:9})" || true
        fi
    done
    [ "$found" = "no" ] && echo "(no interrupted transactions — clean)"
    return 0
}

cmd_status() {
    ensure_dirs
    [ -f "$TDIR/STATUS.md" ] && { echo "===== STATUS ====="; cat "$TDIR/STATUS.md"; echo; }
    local a; a="$(active_id)"
    echo "===== ACTIVE ====="
    [ -n "$a" ] && echo "active task: $a" || echo "active task: (none)"
    echo "===== OPEN TRANSACTIONS ====="
    cmd_resume
}

cmd_pitfall() {
    ensure_dirs
    local text="$*"
    [ -n "$text" ] || die "pitfall needs text"
    [ -f "$TDIR/PITFALLS.md" ] || printf '# Pitfalls\n\n' > "$TDIR/PITFALLS.md"
    printf -- '- %s — %s\n' "$(today)" "$text" >> "$TDIR/PITFALLS.md"
    journal "PITFALL $text"
    echo "✎ logged pitfall. Consider distilling into a memory file if high-signal."
}

cmd_task() {
    local id="${1:-}"; local state="${2:-}"
    [ -n "$id" ] && [ -n "$state" ] || die "task needs <id> <state>"
    journal "TASK $id -> $state"
    echo "(update the '$id' row in TASKS.md to: $state)"
}

# Hook entrypoint: PostToolUse passes JSON on stdin; record the touched file.
cmd_hook_postedit() {
    local a; a="$(active_id)"
    [ -n "$a" ] || exit 0
    local f="$TX/$a.json"
    [ -f "$f" ] || exit 0
    local payload; payload="$(cat || true)"
    local file; file="$(printf '%s' "$payload" | jq -r '.tool_input.file_path // empty' 2>/dev/null || true)"
    [ -n "$file" ] || exit 0
    local rel="${file#$ROOT/}"
    local tmp; tmp="$(mktemp)"
    jq --arg f "$rel" '.files = ((.files + [$f]) | unique)' "$f" > "$tmp" 2>/dev/null && mv "$tmp" "$f" || rm -f "$tmp"
    exit 0
}

main() {
    local sub="${1:-status}"; shift || true
    case "$sub" in
        begin)        cmd_begin "$@";;
        checkpoint)   cmd_checkpoint "$@";;
        done)         cmd_done "$@";;
        abort)        cmd_abort "$@";;
        resume)       cmd_resume "$@";;
        status)       cmd_status "$@";;
        pitfall)      cmd_pitfall "$@";;
        task)         cmd_task "$@";;
        hook-postedit) cmd_hook_postedit "$@";;
        active)       active_id;;
        *)            die "unknown subcommand '$sub' (try: begin|checkpoint|done|abort|resume|status|pitfall|task)";;
    esac
}
main "$@"
