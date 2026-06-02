# worse::core::container — STATUS

> Dashboard. Updated as tasks change state. Full breakdown in `TASKS.md`,
> rationale in `DECISIONS.md`, gotchas in `PITFALLS.md`.

| Field | Value |
|---|---|
| **Current phase** | **Phase 8 COMPLETE** — design-audit hardening (R45–R47) all 9 tasks done; Phase 7 ✓ |
| **Active task** | (none) |
| **Next up** | (Phase 8 done) — all 9 dep-sorted tasks complete: oom_predictable, hardcap_verify (R46), pq_reserve, fill_memset, hash_forceinline (R47), swiss_tombstone, ordered_transparent, map_single_descent, polish. Hash-cache still deferred (R40, needs `Hash<String>`). No queued work. |
| **Scope this iteration** | Phase 5c ✓, Phase 6 ✓ (R33–R39), Phase 7 ✓ (R40–R44), **Phase 8 design audit done → 9 tasks queued (R45)** |
| **Overall progress** | branch `feature/container`; **Phase S ✓, 0–5 ✓, 5b ✓, 5c ✓, 6 ✓, 7 ✓, 8 ✓**; **debug 520/520 green; release/NDEBUG 515/515 fully clean** (P8-polish guarded the debug-only death tests); bench: Array push = std/eastl (R42), List push = std / ForwardList › std·eastl (R43), stableSort › eastl (R41), **SwissTable insert −25% (R47)**, hash/flat › std, fixed lists 16-24× std |
| **Open items** | (none blocking) — deferred: hash_cache (R40, needs `Hash<String>`). hooks JSON optional (PostToolUse IS active this session; each tx still `add`s its source files before `done` for safety). |

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

## How to resume in a new session
1. `bash .tracker/track.sh status` (the SessionStart hook also prints this).
2. If an interrupted transaction is listed → `git stash apply <wip>` if shown, finish it,
   then `track.sh done <id>`; or `track.sh abort <id> --yes` to roll back to its baseline.
3. Read `PITFALLS.md` before touching modules you hit before.
