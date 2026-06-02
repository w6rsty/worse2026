# worse::core::container — TASKS (source of truth)

States: `pending` · `in_progress` · `done` · `blocked`.
Each task ≈ one `track.sh` transaction (`begin <id>` → … → `done <id>`).
Task id = the table's `ID` column.

## Phase S — tracker setup
| ID | Task | State |
|---|---|---|
| S1 | `.tracker/` dir + `track.sh` + seeded STATUS/TASKS/DECISIONS/PITFALLS | done |
| S2 | Wire SessionStart/Stop/PostToolUse hooks into `.claude/settings.json` | done (user-applied; PostToolUse now auto-records tx files) |
| S3 | Distilled tracking-workflow memory file + DECISIONS memory | done |
| S4 | Verify tracker round-trips (begin→checkpoint→resume→abort verified; commit path pending branch decision) | done |

## Phase 0 — shared infrastructure
| ID | Task | State |
|---|---|---|
| P0-type_traits | `core/type_traits.cppm` + test (trait truth tables, IsTriviallyRelocatable) | done (eb6c65f) |
| P0-utility | `core/utility.cppm` (Pair, CompressedPair EBO, Less/Greater/EqualTo, move/swap) + test | done |
| P0-iterator | flesh out `container/iterator.cppm` (tags, concepts, ReverseIterator, distance/advance) + test | done |
| P0-memory_util | `container/memory_util.cppm` (uninitialized*, destroyRange, relocate) + test | done |

## Phase 1 — algorithms
| ID | Task | State |
|---|---|---|
| P1-heap | `algorithm/heap.cppm` (makeHeap/pushHeap/popHeap/sortHeap/isHeap) + test | done |
| P1-binary_search | `algorithm/binary_search.cppm` (lowerBound/upperBound/binarySearch/equalRange) + test | done |
| P1-sort | `algorithm/sort.cppm` (insertionSort, introsort, partialSort, nthElement, isSorted) + test | done |
| P1-nonmodifying | `algorithm/nonmodifying.cppm` (find/count/forEach/allOf/equal/mismatch) + test | done |
| P1-modifying | `algorithm/modifying.cppm` (copy/move/fill/rotate/reverse/remove/unique/replace) + test | done |
| P1-umbrella | `algorithm/algorithm.cppm` umbrella re-export | done |

## Phase 2 — contiguous containers
| ID | Task | State |
|---|---|---|
| P2-array | finish `container/array.cppm` (relocate grow, full API, eraseUnsorted) + test | done |
| P2-static_array | `container/static_array.cppm` (T mData[N], constexpr) + test | done |
| P2-fixed_array | `container/fixed_array.cppm` (inline buffer + usize mSize, hard-cap) + test | done |
| P2-bench | `bench/` tree + `wcontainer_bench` target (nanobench, WORSE_BUILD_BENCH OFF) + first benches | done |

## Phase 3 — associative (flat) + adapter + intrusive containers
| ID | Task | State |
|---|---|---|
| P3-priority_queue | `container/priority_queue.cppm` (binary max-heap adapter over Array; push/pop/top/emplace; heap algos) + test | done (8831532) |
| P3-flat_set | `container/flat_set.cppm` (sorted unique `Array<Key>` + binary search; insert/erase/find/lowerBound; transparent lookup) + test | done (8ff9f8f) |
| P3-flat_map | `container/flat_map.cppm` (sorted `Array<Pair<Key,T>>` keyed by `.first`; operator[]/at/insert/find; PairCompare wrapper) + test | done (572c736) |
| P3-intrusive_list | `container/intrusive_list.cppm` (embedded-node circular doubly-linked list, no allocation; splice/erase/remove) + test | done (e9888ab) |

## Phase 4 — hash family (open-addressing Robin Hood)
| ID | Task | State |
|---|---|---|
| P4-hash | `container/hash.cppm` (Hash<T> for integral/enum/pointer; hashFinalize/hashCombine/hashBytes) + test | done |
| P4-hash_table | `container/hash_table.cppm` (Robin Hood engine: Base/Derived, u16 DIB info, backward-shift erase, rehash, forward iterator) + test | done |
| P4-unordered_set | `container/unordered_set.cppm` (adapter, identity extractor, const iterators) + test | done |
| P4-unordered_map | `container/unordered_map.cppm` (adapter, .first extractor, mutable .second; operator[]/at/insertOrAssign/tryEmplace) + test | done |

## Phase 5 — node containers (allocating linked lists)
Game-perf direction: EASTL/Unreal-flavored, not std clones (see DECISIONS R25–R31).
| ID | Task | State |
|---|---|---|
| P5-list | `container/list.cppm` (allocating doubly-linked: Base/Derived owns nodes, embedded circular sentinel, O(1) size, full API + splice/remove/unique/merge/sort/reverse, alloc-free in-place binned merge sort) + test | done (46c4646) |
| P5-forward_list | `container/forward_list.cppm` (allocating singly-linked, EASTL slist shape: O(1) size, `*After` API, beforeBegin/insertAfter/eraseAfter/spliceAfter, merge/sort/reverse, no back/pushBack/tail) + test | done (6d745a1) |

## Phase 5b — fixed-capacity inline node lists (zero-heap)
Inline node pool + free-list, hard-cap (R6); reuse exported List/ForwardList node + iterator types (R32).
| ID | Task | State |
|---|---|---|
| P5b-fixed_list | `container/fixed_list.cppm` (inline doubly-linked node pool, zero-heap free-list, hard-cap; full in-place sort/merge/remove/unique/reverse) + test | done (f462082) |
| P5b-fixed_slist | `container/fixed_slist.cppm` (inline singly-linked node pool, zero-heap, `*After` API) + test | done (0195f77) |

## Phase 6 — aggregation, algorithms, perf validation
| ID | Task | State |
|---|---|---|
| P6-container_umbrella | `container/container.cppm` umbrella re-export + test | done (5621b24) |
| P6-bench_node_lists | bench suites restructure + node-list benches vs std; perf fixes R33/R34 | done (99dad57) |
| P6-bench_assoc | Array/UnorderedMap/UnorderedSet/FlatMap benches vs std (R35) | done (d2f6e1f) |
| P6-stable_sort | `algorithm/sort.cppm` `stableSort` (alloc-free in-place merge, R36) | done (88f45ddc) |
| P6-swisstable | `container/swiss_table.cppm` SwissTable engine (portable SWAR control-byte groups, R38) + stress test + bench | done |
| P6-range_overloads | `algorithm/algorithm.cppm` Range concept + container-range overloads (sort/stableSort/reverse/find/findIf/count/forEach/allOf/anyOf/noneOf) + test | done |
| P6-bench_eastl | bench: add EASTL 3.27.1 → 3-way worse/std/EASTL comparison (list/slist/vector/hash/map/sort/stableSort) (R39) | done (b43e9c5) |
| P6-array_vectorize | `Array<trivial>` push vectorization parity (R35) | superseded by **P7-array_trivial_push** (now an active perf task) |

## Phase 5c — ordered associative (red-black tree)
| ID | Task | State |
|---|---|---|
| P5c-rb_tree | `container/rb_tree.cppm` red-black engine (SGI rebalance, KeyOfValue, checkInvariant) + randomized stress test vs std::set | done (fff28c5) |
| P5c-set_map | `container/set.cppm` + `container/map.cppm` ordered adapters over rb_tree + tests + umbrella | done |

## Phase 7 — performance optimizations (bench-driven; gaps from R39)
Selection criterion (user): currently slower than std, OR ≥15% behind EASTL. ns/op @ N=4096, arm64
release/NDEBUG, lower=faster. Each task must re-bench to confirm it closed the gap without regressions.
| ID | Task | Measured gap | State |
|---|---|---|---|
| P7-hash_cache | ~~Cache the hash code per slot in `hash_table` + `swiss_table` so lookup skips key recompare.~~ **DEFERRED (R40)** — identity-hash experiment proved the int-key gap is NOT hash/key-recompare (free hash left Robin Hood unchanged); caching only helps EXPENSIVE keys (`String`, not yet shipped). Revisit alongside `Hash<String>`. | **hash LOOKUP ≥15% behind EASTL** (also ~slightly behind std for map): UnorderedMap 7.3k / std 7.0k / **eastl 3.1k**; UnorderedSet 6.3k / std 9.7k / **eastl 3.5k**; SwissTable 10.5k / **eastl 3.5k** (~2× gap, structural — open-addressing vs cache-warm chaining) | **deferred** |
| P7-stable_sort_buffered | Buffered O(n log n) `stableSort` (allocator scratch) alongside the alloc-free in-place one (reopens R13/R36); pick buffered when an allocator is available. | **~12× behind std, ~3× behind EASTL**: worse 192–223k / **std 18k** / eastl 74k → **DONE (R41): worse ~72k (now BEATS eastl 74k; ~3× faster), std stays 17.7k.** Buffer-the-left-half merge at runtime; in-place rotation merge kept as constexpr/no-alloc fallback. | **done** |
| P7-array_trivial_push | ~~Vectorizable trivial-type push fast path for `Array`.~~ **DONE (R42)** — not vectorization (asm proved neither loop vectorizes): the out-of-line grow call pinned `mpEnd`/`mpCapacity` in memory (per-element spill/reload). Fix = `WE_FORCEINLINE` `emplaceBack`/`pushBack` with grow inline → members promote to registers like std::vector. Type-agnostic, no seam change. | **~5–8× behind std/eastl**: Array ~5.8k / **std 1.1k / eastl 1.1k** → **DONE: Array ~1.13k = parity** (std 1.11k, eastl 1.11k); no-reserve 6.1k→1.7k | **done** |
| P7-default_alloc_fastpath | ~~Trim the default `Allocator` per-node path~~ **DONE (R43)** — asm showed `Allocator::allocate`/`deallocate` were out-of-line per node (tracking not DCE'd, call frame each). Fix = `WE_FORCEINLINE` both → bare operator new/delete like ThinAlloc. Seam intact. | **behind std**: List push +9%, ForwardList push +11% → **DONE: List ~65.2k (= std/ThinAlloc), ForwardList ~50.2k (now BEATS std 51.4k & eastl 51.1k)**; sort-rebuild within noise | **done** |
| P7-introsort_tune | ~~Tune introsort to match `std::sort`.~~ **DONE (R44)** — added branchless Lomuto partition for trivially-copyable types (pdqsort lever), Hoare kept for heavy types. NO net change on Apple Silicon (strong branch predictor; threshold 16 already optimal) but a real x86 win → kept per user as the cross-platform choice. +stress tests. | **~13% behind std**: worse 32.6k / **std 28.9k** / eastl ~33–43k (beats eastl) → **DONE: branchless kept (x86-targeted); Apple-Silicon residual is a libc++ outlier like stableSort (R41)** | **done** |

> Excluded as noise / no clear lever: `List` iterate-sum +6% vs std (pure pointer-chase, identical node layout; within cross-run variance).

## Phase 8 — design-audit hardening (predictability + game-perf; audit-driven)
Source: full design re-audit of all 24 container + 6 algorithm modules against the two locked
principles — **game-performance-first** and **predictability** (DECISIONS **R45**). Five parallel
specialist reviews + direct source verification of every load-bearing claim. The architecture honors
both principles; gaps cluster in two cross-cutting **release-build predictability** themes, plus a few
unrealized perf levers and missing seams/docs. One alleged correctness BLOCKER (Robin Hood
duplicate-insert) was **investigated and DISPROVED** — see the note below and R45.

**Verified root facts (R45):** `WE_ASSERT`/`WE_ASSERT_MSG` = `((void)0)` under `NDEBUG`
(macro.hpp:57,63); `memory::allocate` uses **throwing** `::operator new` (memory.cppm:40,42);
`handleAllocationFailure` is `unreachable()` = UB (memory.cppm:58); `fill`/`fillN` are scalar loops
(modifying.cppm:148-163).

### Tier 1 — predictability correctness (do first)
| ID | Task | Evidence (file:line) | State |
|---|---|---|---|
| P8-oom_predictable | Make OOM deterministic. `memory::allocate` → `::operator new(n, std::nothrow)` (+ aligned nothrow) so the `p == nullptr` checks every container already has become live; `handleAllocationFailure` → deterministic `std::abort()` (keep `[[noreturn]]`), NOT `unreachable()`/UB. Revives `stableSort`'s documented alloc-fail in-place fallback (sort.cppm:397, currently dead). Pure win, no API change. **DONE:** memory.cppm only (2 edits); sized delete still pairs (nothrow draws same pool). Debug **508/508 green**; release/NDEBUG unchanged (4 pre-existing `WE_ASSERT`-in-release death-test failures, none from this task). | memory.cppm:40-42,56-58; sort.cppm:393-404 (now reachable) | **done** |
| P8-hardcap_verify | **Contract DECIDED — R46** (three-layer hard cap). **(1) fail-closed:** promote capacity guards `WE_ASSERT` → `WE_VERIFY` on mutating paths so overflow aborts in *every* build (FixedArray `emplace*`/ctor, FixedList/FixedSList `allocSlot` + ctor/`merge`, Array `doAllocate` `kMaxElements`; clamp `getNewCapacity` vs `SizeType` wrap; hash `u16` wrap). **(2) escape hatch:** add `try*` non-aborting variants — `FixedArray::tryEmplaceBack`→Pointer + `tryPushBack`→bool; `FixedList::tryEmplaceFront`/`tryEmplaceBack`; `FixedSList::tryEmplaceFront`/`tryEmplaceAfter`. **(3) query:** add `remaining()=N-mSize` (`full()`/`capacity()`/`size()`/`empty()` already exist). Boundary: InputIterator bulk relies on the per-node `WE_VERIFY` backstop; no `try`-bulk. Death-test: fill→N+1 aborts under NDEBUG, `try*` at N returns nullptr/false. | fixed_array.cppm:209,52,72,229,274,314,138-141; fixed_list.cppm:595,481,211-213; fixed_slist.cppm:559,441,185-187; array.cppm:84,102-105; hash_table.cppm:602 | pending |

### Tier 2 — game-perf levers (measured)
| ID | Task | Evidence (file:line) | State |
|---|---|---|---|
| P8-fill_memset | Add a `memset`/trivial-fill fast path to `fill`/`fillN`, mirroring copy/move's existing memmove specialization: `if constexpr (IsPointer<It> && IsTriviallyCopyable<Value>) && !__builtin_is_constant_evaluated()` → `__builtin_memset` for `sizeof(Value)==1`, byte-splat-detect otherwise. Per-frame buffer clears / SoA-column resets / resize-grow init are the target. Re-bench to confirm the codegen flip (u8*/i32* fill → memset, not a byte loop). | modifying.cppm:146-164 (scalar today); copy/move pattern at :45-136 | pending |
| P8-pq_reserve | Forward `reserve`/`capacity`/`getAllocator` from `PriorityQueue` to its backing Array so push is bounded/pre-sizable. The most per-frame-likely adapter (pathfinding open-set, timers, event queue) currently cannot pre-allocate → push is not amortized-predictable. Two-line forwards; aligns with `flat_*`. | priority_queue.cppm:79-83 (none exposed) | pending |
| P8-hash_forceinline | **Measured experiment** (honor R40/R42 "measure the lever first"): `WE_FORCEINLINE` the hash hot-path leaves — SWAR `match`/`maskEmpty`/`lowestMatch`/`ctrlIsFull`/`h2Of`, `findIndex`, `insertNoGrow`. `WE_FORCEINLINE` appears **0×** in the hash family today, despite the measured R42/R43 "out-of-line pins members" precedent. Re-run release bench ([[container-bench]]); **KEEP ONLY** annotations that move the number (avoid icache bloat on cold paths). | hash_table.cppm:535,565; swiss_table.cppm:39,56-64,68,379,400,470 | pending |

### Tier 3 — predictability semantics + docs (batchable)
| ID | Task | Evidence (file:line) | State |
|---|---|---|---|
| P8-swiss_tombstone | SwissTable: make `reserve(n)` **reclaim tombstones** (force a cleaning rehash when `mSize + mDeleted` would exceed maxLoad even if `mSize` alone fits), document the erase→same-capacity-rehash coupling, optional `wouldRehashOnInsert()` so frame code can pre-pay at a safe point; header note steering per-frame churn maps toward the tombstone-free Robin Hood engine. Closes the "surprise rehash mid-frame" gap — the headline predictability issue for game hash maps. | swiss_table.cppm:338-344,451,542-553 | pending |
| P8-ordered_transparent | Transparent (heterogeneous) lookup for ordered `Set`/`Map`: template `findNode`/`lowerBoundNode`/`upperBoundNode` + adapter `find/contains/count/lowerBound/upperBound/erase(key)` on a query type `K`, gated on a transparent comparator (`is_transparent`). Brings tree containers to `flat_*` parity; avoids temp `Key` construction (e.g. `Map<String,V>::find("lit")`). Comparator is a single call site in the engine. Consistent with R21's gating rationale. | rb_tree.cppm:703-714,895-900; flat_* already transparent | pending |
| P8-map_single_descent | Eliminate the double tree-walk in `Map::operator[]`/`tryEmplace`/`insertOrAssign` by exposing a hint/insert-slot from the RB engine (it already computes `y`+`insertLeft` inside `insertUniqueImpl`). `flat_map` already does single-probe insert; bring `Map` to parity → halves the O(log n) walks on the common insert-or-update idiom. | map.cppm:102-169; rb_tree.cppm insertUniqueImpl | pending |
| P8-polish | Batch of small predictability/robustness items: **(a)** debug range-asserts on `erase()` (`pos ∈ [begin,end)`) across array/fixed_array/flat_set/flat_map; `intrusive_list::erase` assert `pos != end()` (silent `mSize` corruption today, intrusive_list.cppm:227). **(b)** document `PriorityQueue` equal-priority pop = unspecified order (cross-run/platform determinism footgun) + tiebreaker guidance. **(c)** document the strict-weak-ordering contract on `flat_*` (+ optional debug adjacent-pair check after sort/bulk build). **(d)** `reserve` before the append loop in `flat_*::bulkAppendSortUnique` for ForwardIterator+ inputs (flat_set.cppm:222, flat_map.cppm:358). **(e)** `Allocator::mpName` default-member-initializer (allocator.cppm:103). **(f)** hash iteration-order "unspecified, changes on rehash" disclaimer on both engines + adapters. | intrusive_list.cppm:227; priority_queue.cppm; flat_set.cppm:222; flat_map.cppm:358; allocator.cppm:103 | pending |

> **NOT a bug (investigated, R45):** the alleged Robin Hood duplicate-insert (hash_table.cppm:586)
> is **correct** — the dup guard `info == dist && keyEqual` mirrors `findIndex` (:554), and the Robin
> Hood invariant guarantees a present key is reached *before* any `info < dist` steal, so the guard
> stays armed (`insertedIndex == kInvalidIndex`) until it fires. Cheap insurance only: add an "every
> key appears once" pass to the hash stress test.
>
> **Deferred (consistent with R40):** per-slot hash-cache stays deferred until `Hash<String>` /
> expensive keys land.
