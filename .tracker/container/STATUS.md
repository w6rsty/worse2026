# worse::core::container — STATUS

> Dashboard. Updated as tasks change state. Full breakdown in `TASKS.md`,
> rationale in `DECISIONS.md`, gotchas in `PITFALLS.md`.

| Field | Value |
|---|---|
| **Current phase** | Phase S — tracker setup (S1/S3/S4 done; S2 hooks awaiting user OK) |
| **Active task** | (none — ready to begin Phase 0) |
| **Next up** | Phase 0 — `type_traits`, `utility`, `iterator`, `memory_util` |
| **Scope this iteration** | Phase S → 0 → 1 → 2, then checkpoint for review |
| **Overall progress** | tracker live; 0 / 3 build phases started |
| **Open questions** | hooks authorization; branch/commit strategy (master vs feature branch) |

## Phase ledger
- [ ] **Phase S** — tracker + hooks + seeded docs *(in progress)*
- [ ] **Phase 0** — shared infra (type_traits, utility, iterator, memory_util)
- [ ] **Phase 1** — algorithms (heap, sort, binary_search, nonmodifying, modifying)
- [ ] **Phase 2** — contiguous containers (Array finish, StaticArray, FixedArray) + bench target
- [ ] *(checkpoint — review before continuing)*
- [ ] Phase 3 — intrusive_list, flat_set/flat_map, priority_queue
- [ ] Phase 4 — hash (Robin Hood): hash, hash_table, unordered_set/map
- [ ] Phase 5 — list, forward_list; (deferred) rb_tree + set/map
- [ ] Phase 6 — SwissTable / stableSort / range overloads / aggregators

## How to resume in a new session
1. `bash .tracker/track.sh status` (the SessionStart hook also prints this).
2. If an interrupted transaction is listed → `git stash apply <wip>` if shown, finish it,
   then `track.sh done <id>`; or `track.sh abort <id> --yes` to roll back to its baseline.
3. Read `PITFALLS.md` before touching modules you hit before.
