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
| P7-array_trivial_push | Vectorizable trivial-type push fast path for `Array` (e.g. `if constexpr` trivially-copyable + default-alloc → direct store, or a bulk uninitialized-fill `append`), without breaking the AllocatorTraits seam for the general case. Supersedes the deferred P6-array_vectorize (R35). | **~8× behind std AND EASTL**: Array 9.0k / **std 1.1k / eastl 1.1k** | pending |
| P7-default_alloc_fastpath | Trim the default `Allocator` per-node path (avoid `AllocInfo`/`source_location` threading on the hot path) so node-container push/rebuild matches std/EASTL. `List<ThinAlloc>` already ties EASTL → the residual is purely the default allocator. | **behind std**: List push 72k/69k (+4%), ForwardList push 59k/53k (+11%), ForwardList sort-rebuild 126k/115k (+10%); vs EASTL 13–15% | pending |
| P7-introsort_tune | Tune introsort (pivot selection / insertion-sort threshold; consider pdqsort-style) to match `std::sort`. | **~11% behind std**: worse 31.8k / **std 28.6k** / eastl 30.9k (<15% vs EASTL) | pending |

> Excluded as noise / no clear lever: `List` iterate-sum +6% vs std (pure pointer-chase, identical node layout; within cross-run variance).
