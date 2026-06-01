# worse::core::container — STATUS

> Dashboard. Updated as tasks change state. Full breakdown in `TASKS.md`,
> rationale in `DECISIONS.md`, gotchas in `PITFALLS.md`.

| Field | Value |
|---|---|
| **Current phase** | Phase 2 — contiguous containers (2 / 4 done) |
| **Active task** | (none) |
| **Next up** | `P2-fixed_array` (FixedArray<T,N>, inline buffer + usize mSize, hard-cap) |
| **Scope this iteration** | Phase S → 0 → 1 → 2, then checkpoint for review |
| **Overall progress** | branch `feature/container`; Phase S ✓; **Phase 0 ✓**, **Phase 1 ✓** (heap, binary_search, sort, nonmodifying, modifying, umbrella) |
| **Open items** | hooks JSON (user to paste into `.claude/settings.json`) |

## Phase ledger
- [ ] **Phase S** — tracker + hooks + seeded docs *(in progress)*
- [x] **Phase 0** — shared infra (type_traits, utility, iterator, memory_util) ✓
- [x] **Phase 1** — algorithms (heap, sort, binary_search, nonmodifying, modifying, umbrella) ✓
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
