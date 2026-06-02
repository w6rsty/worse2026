# worse::core::container — STATUS

> Dashboard. Updated as tasks change state. Full breakdown in `TASKS.md`,
> rationale in `DECISIONS.md`, gotchas in `PITFALLS.md`.

| Field | Value |
|---|---|
| **Current phase** | **Phase 11 COMPLETE (4/4)** — **P11-bench_machine_output ✓** (`WCB_OUT` → nanobench CSV; per-suite render to beat `title()`'s `mResults.clear()`; 50 rows), **P11-bench_compare ✓** (`scripts/bench_compare.py`, (title,name) key, 15% tol, no-baseline pass), **P11-ci_workflow ✓** (`.github/workflows/container-ci.yml`: changes→test(Debug)→bench(same-runner PR-vs-master)→always-run `gate`), **P11-branch_protection ✓** (applied: `gate` required on master; repo made public to unblock 403). Phase 10 & iteration-1 (S–8) closed. **Next: Phase 9** (Hash<String>, R48) — still blocked on a `String` type. |
| **Active task** | (none) |
| **Next up** | **Phase 9** (R48, string & expensive keys) — **blocked** until a `String` type lands; `P9-hash_bytes_mixer` is the one String-independent piece that could start. CI gate is live (`gate` required on master). |
| **Scope this iteration** | Phase 5c ✓, Phase 6 ✓ (R33–R39), Phase 7 ✓ (R40–R44), **Phase 8 design audit done → 9 tasks queued (R45)** |
| **Overall progress** | branch `feature/container`; **Phase S ✓, 0–5 ✓, 5b ✓, 5c ✓, 6 ✓, 7 ✓, 8 ✓**; **debug 520/520 green; release/NDEBUG 515/515 fully clean** (P8-polish guarded the debug-only death tests); bench: Array push = std/eastl (R42), List push = std / ForwardList › std·eastl (R43), stableSort › eastl (R41), **SwissTable insert −25% (R47)**, hash/flat › std, fixed lists 16-24× std |
| **Open items** | (none for iteration 1) — all deferred work consolidated into **Phase 9 (R48)**, gated on a `String` type. hooks JSON optional (PostToolUse IS active; each tx still `add`s its source files before `done` for safety). |

## Phase ledger
- [x] **Phase S** — tracker + hooks + seeded docs ✓ (S1–S4 done; PostToolUse hook active this session)
- [x] **Phase 0** — shared infra (type_traits, utility, iterator, memory_util) ✓
- [x] **Phase 1** — algorithms (heap, sort, binary_search, nonmodifying, modifying, umbrella) ✓
- [x] **Phase 2** — contiguous containers (Array finish, StaticArray, FixedArray) + bench target ✓
- [x] *(checkpoint — Phase S/0/1/2 reviewed green: 317 tests, release build clean)*
- [x] **Phase 3** — priority_queue, flat_set, flat_map, intrusive_list ✓
- [x] *(checkpoint — Phase 3 reviewed green: 359 tests debug, release/NDEBUG clean)*
- [x] **Phase 4** — hash, hash_table (Robin Hood), unordered_set, unordered_map ✓
- [x] *(checkpoint — Phase 4 complete: 405 tests debug, release/NDEBUG clean + 401 non-death green)*
- [x] **Phase 5** — list (doubly-linked), forward_list (singly-linked) — game-perf EASTL/Unreal shape ✓
- [x] *(checkpoint — Phase 5 complete: 448 tests debug, release/NDEBUG clean + 442 non-death green)*
- [x] **Phase 5b** — fixed_list, fixed_slist (inline zero-heap node pool, hard-cap) ✓
- [x] *(checkpoint — Phase 5b complete: 471 tests debug, release/NDEBUG clean + 463 non-death green)*
- [x] **Phase 5c** — rb_tree + ordered Set/Map (R37, stress-verified vs std::set) ✓
- [x] **Phase 6** — umbrella, bench suites, perf fixes (R33/34), stableSort (R36), range overloads, SwissTable (R38), EASTL 3-way bench (R39) ✓
- [x] **Phase 7** — bench-driven perf optimizations (R39 gaps): ~~hash_cache~~ **deferred (R40)**, **stable_sort_buffered ✓ (R41)**, **array_trivial_push ✓ (R42)**, **default_alloc_fastpath ✓ (R43)**, **introsort_tune ✓ (R44)** ✅
- [x] **Phase 8** — design-audit hardening (R45–R47) ✓: oom_predictable, hardcap_verify (R46), pq_reserve, fill_memset, hash_forceinline (R47, SwissTable insert −25%), swiss_tombstone, ordered_transparent (Set/Map default Less<>), map_single_descent, polish *(9/9; **debug 520/520, release 515/515 — fully clean**)*
- [ ] **Phase 9** — NEXT ITERATION (R48): string & expensive keys — P9-hash_bytes_mixer, P9-hash_string (blocked on a `String` type), P9-hash_cache (un-defers R40); float hashing R20 separately deferred *(queued; not started)*
- [x] **Phase 10 COMPLETE** — cleanup/housekeeping (R49): **P10-builtin_wrap ✓** (new `worse.core.intrinsics`, 25 sites, release codegen-neutral), **P10-style_audit ✓** (confirmed clang-format clean, conventions hold), **P10-doxygen_setup ✓** (Doxygen + Awesome CSS v2.4.2; cppm-module filter verified end-to-end, doxygen 1.17.0, 0 warnings, 196 pages), **P10-doc_pass ✓** (focused `/** \brief */` across all 37 modules; 6 per-group commits; comments-only verified; **debug 520/520**; **doxygen 0 warnings**) *(4/4 done)*. **Docs follow-ups:** output + vendored theme relocated to a committed top-level `docs/` (tx `P10-doc_relocate`); **curated guide authored** on top of the reference (tx `P10-doc_guide`) — `\mainpage`, topic groups (`\defgroup`), container/algorithm/core guides with master comparison tables + per-type deep sections (memory/complexity/stability/when-to-use/examples). Regenerated: **284 pages, 0 warnings**; entry `docs/html/index.html`.
- [x] **Phase 11 COMPLETE** — CI gate for container/algorithm changes (R50) *(4/4)*: **P11-bench_machine_output ✓** (`WCB_OUT=path` → nanobench `templates::csv()`; renders after EACH suite because `Bench::title()` clears `mResults`; verified 50 rows, dup name across suites → (title,name) key), **P11-bench_compare ✓** (`scripts/bench_compare.py`, stdlib-only, (title,name) match, `elapsed` median, 15% tol, markdown report, exit 1 on regress/missing, **empty master → no-baseline pass**; all 5 cases verified), **P11-ci_workflow ✓** (`.github/workflows/container-ci.yml`; **no `on.paths`** → `changes` detect + `test`(macos-15 Debug, full `wcoro_tests`) + `bench`(macos-15, same-runner PR-vs-master, tolerant of no-baseline) + **always-run `gate`** = sole required check; CI overrides preset's local abs-paths via `$VCPKG_INSTALLATION_ROOT` + `$(brew --prefix llvm)`; YAML + scope-regex validated locally, true E2E needs a real PR), **P11-branch_protection ✓** (`scripts/apply_branch_protection.sh`; `gate` required on `master`, strict=false, enforce_admins=false; classic-protection + rulesets both 403'd on private/Free → user made repo **public**, then applied; read-back confirms `contexts:["gate"]`). ⚠️ commits are local on `feature/container` — push needed for the workflow to exist on the remote.

## How to resume in a new session
1. `bash .tracker/track.sh status` (the SessionStart hook also prints this).
2. If an interrupted transaction is listed → `git stash apply <wip>` if shown, finish it,
   then `track.sh done <id>`; or `track.sh abort <id> --yes` to roll back to its baseline.
3. Read `PITFALLS.md` before touching modules you hit before.
