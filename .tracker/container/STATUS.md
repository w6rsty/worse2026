# worse::core::container — STATUS

> Dashboard. Updated as tasks change state. Full breakdown in `TASKS.md`,
> rationale in `DECISIONS.md`, gotchas in `PITFALLS.md`.

| Field | Value |
|---|---|
| **Current phase** | **CHECKPOINT** — Phase 4 complete, awaiting review |
| **Active task** | (none) |
| **Next up** | (review) then Phase 5 — list, forward_list; (deferred) rb_tree + set/map |
| **Scope this iteration** | Phase 4 ✓ — hash, hash_table (Robin Hood), unordered_set, unordered_map (each: module + test, atomic commit) |
| **Overall progress** | branch `feature/container`; **Phase S ✓, 0 ✓, 1 ✓, 2 ✓, 3 ✓, 4 ✓**; **405 tests green** (debug); release/NDEBUG build clean + 401 non-death tests green |
| **Open items** | hooks JSON (user to paste into `.claude/settings.json`) — PostToolUse not active, so each tx uses `track.sh add <files>` before `done` |

## Phase ledger
- [ ] **Phase S** — tracker + hooks + seeded docs *(in progress)*
- [x] **Phase 0** — shared infra (type_traits, utility, iterator, memory_util) ✓
- [x] **Phase 1** — algorithms (heap, sort, binary_search, nonmodifying, modifying, umbrella) ✓
- [x] **Phase 2** — contiguous containers (Array finish, StaticArray, FixedArray) + bench target ✓
- [x] *(checkpoint — Phase S/0/1/2 reviewed green: 317 tests, release build clean)*
- [x] **Phase 3** — priority_queue, flat_set, flat_map, intrusive_list ✓
- [x] *(checkpoint — Phase 3 reviewed green: 359 tests debug, release/NDEBUG clean)*
- [x] **Phase 4** — hash, hash_table (Robin Hood), unordered_set, unordered_map ✓
- [x] *(checkpoint — Phase 4 complete: 405 tests debug, release/NDEBUG clean + 401 non-death green)* ← **HERE**
- [ ] Phase 5 — list, forward_list; (deferred) rb_tree + set/map
- [ ] Phase 6 — SwissTable / stableSort / range overloads / aggregators

## How to resume in a new session
1. `bash .tracker/track.sh status` (the SessionStart hook also prints this).
2. If an interrupted transaction is listed → `git stash apply <wip>` if shown, finish it,
   then `track.sh done <id>`; or `track.sh abort <id> --yes` to roll back to its baseline.
3. Read `PITFALLS.md` before touching modules you hit before.
