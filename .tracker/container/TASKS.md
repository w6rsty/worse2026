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

## Later phases (expanded as each is reached)
| ID | Task | State |
|---|---|---|
| P5-* | list, forward_list; (deferred) rb_tree + set/map | pending |
| P6-* | SwissTable, stableSort, range overloads, container.cppm aggregator | pending |
