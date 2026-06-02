# worse::core::container — STATUS

> Dashboard. Updated as tasks change state. Full breakdown in `TASKS.md`,
> rationale in `DECISIONS.md`, gotchas in `PITFALLS.md`.

| Field | Value |
|---|---|
| **Current phase** | **Phase 6 complete** — all planned containers/algorithms done; only Array-vectorize deferred (R35) |
| **Active task** | (none) |
| **Next up** | (review) — effort essentially complete; optional: Array trivial-push vectorization (R35, deferred), SwissTable string-key benches / default-swap |
| **Scope this iteration** | Phase 5c ✓ (rb_tree + Set/Map), Phase 6 ✓ (umbrella, benches+perf R33–R36, stableSort, range overloads, SwissTable R38) |
| **Overall progress** | branch `feature/container`; **Phase S ✓ (incl. S2 hooks), 0–5 ✓, 5b ✓, 5c ✓, 6 ✓**; **507 tests green** (debug); release/NDEBUG clean (2 pre-existing death tests aside); bench: node lists ≈ std, hash/flat/SwissTable › std, fixed lists 17-24× std |
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
- [x] *(checkpoint — Phase 4 complete: 405 tests debug, release/NDEBUG clean + 401 non-death green)*
- [x] **Phase 5** — list (doubly-linked), forward_list (singly-linked) — game-perf EASTL/Unreal shape ✓
- [x] *(checkpoint — Phase 5 complete: 448 tests debug, release/NDEBUG clean + 442 non-death green)*
- [x] **Phase 5b** — fixed_list, fixed_slist (inline zero-heap node pool, hard-cap) ✓
- [x] *(checkpoint — Phase 5b complete: 471 tests debug, release/NDEBUG clean + 463 non-death green)*
- [~] **Phase 6 (in progress)** — container.cppm umbrella ✓; bench suites + perf pass ✓ (R33/R34/R35: ≥ STL); SwissTable / stableSort / range overloads pending ← **HERE**
- [ ] Phase 5c — rb_tree + ordered Set/Map
- [ ] Phase 6 cont. — SwissTable / stableSort / range overloads / Array trivial-push vectorization (R35)

## How to resume in a new session
1. `bash .tracker/track.sh status` (the SessionStart hook also prints this).
2. If an interrupted transaction is listed → `git stash apply <wip>` if shown, finish it,
   then `track.sh done <id>`; or `track.sh abort <id> --yes` to roll back to its baseline.
3. Read `PITFALLS.md` before touching modules you hit before.
