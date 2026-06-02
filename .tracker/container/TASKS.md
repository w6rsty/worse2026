# worse::core::container — TASKS (source of truth)

States: `pending` · `in_progress` · `done` · `blocked`.
Each task ≈ one `track.sh` transaction (`begin <id>` → … → `done <id>`).
Task id = the table's `ID` column.

## Phase S — tracker setup
| ID | Task | State |
|---|---|---|
| S1 | `.tracker/` dir + `track.sh` + seeded STATUS/TASKS/DECISIONS/PITFALLS | done |
| S2 | Wire SessionStart/Stop/PostToolUse hooks into `.claude/settings.json` | blocked (needs user authorization — agent-config edit) |
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
| P6-swisstable | SwissTable hash engine behind the R8 facade (game-perf upgrade; Robin Hood already › std) | pending |
| P6-range_overloads | container-range convenience overloads over the iterator-pair algorithms | pending |
| P6-array_vectorize | allocator-seam trivial-store fast path so `Array<trivial>` push matches std::vector (R35) | pending |

## Deferred
| ID | Task | State |
|---|---|---|
| S2 | Wire SessionStart/Stop/PostToolUse hooks into `.claude/settings.json` | blocked (needs user authorization — agent-config edit) |
| P5c-rb_tree | `rb_tree` engine + ordered `Set`/`Map` adapters (low game-priority — flat/hash preferred; R3) | pending |
