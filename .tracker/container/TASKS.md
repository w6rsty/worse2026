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
| P7-hash_cache | ~~Cache the hash code per slot in `hash_table` + `swiss_table` so lookup skips key recompare.~~ **DEFERRED (R40)** — identity-hash experiment proved the int-key gap is NOT hash/key-recompare (free hash left Robin Hood unchanged); caching only helps EXPENSIVE keys (`String`, not yet shipped). Revisit alongside `Hash<String>` → **now scheduled as Phase 9 `P9-hash_cache` (R48)**. | **hash LOOKUP ≥15% behind EASTL** (also ~slightly behind std for map): UnorderedMap 7.3k / std 7.0k / **eastl 3.1k**; UnorderedSet 6.3k / std 9.7k / **eastl 3.5k**; SwissTable 10.5k / **eastl 3.5k** (~2× gap, structural — open-addressing vs cache-warm chaining) | **deferred** |
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
| P8-hardcap_verify | **Contract DECIDED — R46** (three-layer hard cap). **(1) fail-closed:** promote capacity guards `WE_ASSERT` → `WE_VERIFY` on mutating paths so overflow aborts in *every* build (FixedArray `emplace*`/ctor, FixedList/FixedSList `allocSlot` + ctor/`merge`, Array `doAllocate` `kMaxElements`; clamp `getNewCapacity` vs `SizeType` wrap; hash `u16` wrap). **(2) escape hatch:** add `try*` non-aborting variants — `FixedArray::tryEmplaceBack`→Pointer + `tryPushBack`→bool; `FixedList::tryEmplaceFront`/`tryEmplaceBack`; `FixedSList::tryEmplaceFront`/`tryEmplaceAfter`. **(3) query:** add `remaining()=N-mSize` (`full()`/`capacity()`/`size()`/`empty()` already exist). Boundary: InputIterator bulk relies on the per-node `WE_VERIFY` backstop; no `try`-bulk. Death-test: fill→N+1 aborts under NDEBUG, `try*` at N returns nullptr/false. **DONE:** guards promoted (fixed_array capacity asserts, fixed_list/slist `allocSlot`+`merge`, array `doAllocate`, hash `u16` wrap) + `getNewCapacity` clamp; `try*`+`remaining()` added to all 3 fixed types; header doc updated. Split each fixed-list death test into an **always-on** `OverflowHardCapAborts` + debug-only `EmptyAccessAborts`. **Debug 513/513**; **release 506/509** — all 3 fixed-type overflow death tests now abort in release too (was: FixedArray failed); the 3 residual release failures (FlatMap/UnorderedMap `at()`, MacrosTest) are pre-existing debug-only-assert death tests, unrelated → P8-polish to guard. | fixed_array.cppm; fixed_list.cppm; fixed_slist.cppm; array.cppm:84,102-114; hash_table.cppm:602; +3 test files | **done** |

### Tier 2 — game-perf levers (measured)
| ID | Task | Evidence (file:line) | State |
|---|---|---|---|
| P8-fill_memset | Add a `memset`/trivial-fill fast path to `fill`/`fillN`, mirroring copy/move's memmove specialization. **DONE:** `if constexpr (IsPointer && IsTriviallyCopyable<Value> && sizeof==1) && !is_constant_evaluated()` → byte = object-rep of `Value(value)` (via memcpy, robust for char/u8/bool/byte/enum) → `__builtin_memset`. Larger types keep the scalar loop (optimizer lowers 0/repeatable fills itself). Codegen flip is guaranteed (memset is a direct intrinsic, not a loop-recognition). +tests: byte path (u8/bool), `fillN` past-n untouched, constexpr fallback (`static_assert`). Debug 516/516. | modifying.cppm:144-200; copy pattern :45-136 | **done** |
| P8-pq_reserve | Forward `reserve`/`capacity`/`getAllocator` from `PriorityQueue` to its backing Array so push is bounded/pre-sizable. The most per-frame-likely adapter (pathfinding open-set, timers, event queue) currently cannot pre-allocate → push is not amortized-predictable. **DONE:** added `capacity()`/`reserve(n)` + `getAllocator()` (decltype(auto), lazy-instantiated so allocator-less Containers still work) forwarders; +`ReserveBoundsPush` test (reserve→64 pushes realloc-free). Debug 514/514. | priority_queue.cppm:81-90 | **done** |
| P8-hash_forceinline | **Measured experiment** (honor R40/R42 "measure the lever first"): `WE_FORCEINLINE` the hash hot-path leaves. **DONE (R47):** force-inlined the SwissTable SWAR leaves (`ctrlIsFull`/`match`/`maskEmpty`/`maskEmptyOrDeleted`/`lowestMatch`/`h2Of`/`findIndex`) → **SwissTable insert 70.3k→~52.6k (~25% faster, 3 runs)**; lookups unchanged (R40 structural gap confirmed). Robin Hood `findIndex` inline moved nothing → **reverted**. Kept only the measured win. Debug 516/516. | swiss_table.cppm:39,56-64,68,379,400 (kept); hash_table.cppm (reverted, no net change) | **done** |

### Tier 3 — predictability semantics + docs (batchable)
| ID | Task | Evidence (file:line) | State |
|---|---|---|---|
| P8-swiss_tombstone | SwissTable: make `reserve(n)` **reclaim tombstones** (force a cleaning rehash when `mSize + mDeleted` would exceed maxLoad even if `mSize` alone fits), document the erase→same-capacity-rehash coupling, optional `wouldRehashOnInsert()` so frame code can pre-pay at a safe point; header note steering per-frame churn maps toward the tombstone-free Robin Hood engine. **DONE:** `reserve(n)` now does in-place `resize(mCapacity)` when `mDeleted!=0 && n+mDeleted>maxLoad` (capacity fits n but tombstones would rehash first) → a `reserve` after batch-erase restores alloc-free steady state; added `wouldRehashOnInsert()` predicate; header + erase()/reserve() docs spell out the coupling and steer per-frame churn to the tombstone-free Robin Hood engine. +test (reserve→160-insert burst stays at one capacity). Debug 517/517. | swiss_table.cppm:16-30(doc),338-368,522-528 | **done** |
| P8-ordered_transparent | Transparent (heterogeneous) lookup for ordered `Set`/`Map`: template `findNode`/`lowerBoundNode`/`upperBoundNode` + adapter `find/contains/count/lowerBound/upperBound/erase(key)` on a query type `K`, gated on a transparent comparator (`is_transparent`). Brings tree containers to `flat_*` parity; avoids temp `Key` construction (e.g. `Map<String,V>::find("lit")`). **DONE:** templated the engine node-finders (`findNode`/`lowerBoundNode`/`upperBoundNode`) + RBTree/Set/Map `find`/`contains`/`count`/`lowerBound`/`upperBound`/`equalRange` on query type `K`; **default Compare changed `Less<Key>`→`Less<>`** (transparent, matches FlatSet/FlatMap) so it works out of the box. Mirrors the flat_* convention (template unconditionally, NOT gated on `is_transparent`). **`erase` deliberately NOT templated** — a transparent `erase(K)` hijacks `erase(ConstIterator)` for class-type tree iterators (K=Iterator is an exact match vs a conversion), exactly why std omits it pre-C++23. +transparent-lookup tests (Set/Map, heterogeneous int key). Debug 519/519. | rb_tree.cppm:701-730,857-901; set.cppm; map.cppm; +2 tests | **done** |
| P8-map_single_descent | Eliminate the double tree-walk in `Map::operator[]`/`tryEmplace`/`insertOrAssign`. **DONE:** added engine primitive `RBTree::findOrInsertWith(key, factory)` — one descent keyed on `key` (mirrors `insertUniqueImpl`), `factory()` invoked ONLY on the insert branch; routed all three Map sites through it (was find()+insertUnique() = two walks). `insertOrAssign` assigns on the found path / factory on insert (exactly one forward runs); `tryEmplace`/`operator[]` build the value only when inserting. +test locking "value built only on insert" via a ctor counter. Debug 520/520. | rb_tree.cppm:775-810 (findOrInsertWith); map.cppm:112-185; +1 test | **done** |
| P8-polish | Batch of small predictability/robustness items: **(a)** debug range-asserts on `erase()` (`pos ∈ [begin,end)`) across array/fixed_array/flat_set/flat_map; `intrusive_list::erase` assert `pos != end()` (silent `mSize` corruption today, intrusive_list.cppm:227). **(b)** document `PriorityQueue` equal-priority pop = unspecified order (cross-run/platform determinism footgun) + tiebreaker guidance. **(c)** document the strict-weak-ordering contract on `flat_*` (+ optional debug adjacent-pair check after sort/bulk build). **(d)** `reserve` before the append loop in `flat_*::bulkAppendSortUnique` for ForwardIterator+ inputs (flat_set.cppm:222, flat_map.cppm:358). **(e)** `Allocator::mpName` default-member-initializer (allocator.cppm:103). **(f)** hash iteration-order "unspecified, changes on rehash" disclaimer on both engines + adapters. **DONE:** (a) erase range-asserts on array/fixed_array/flat_set/flat_map + `intrusive_list::erase` `pos!=end()`; (b) PQ pop tie-order determinism note; (c) flat_set SWO-contract note; (d) flat_*::bulkAppendSortUnique `reserve` for ForwardIterator+ ranges; (e) `Allocator::mpName` default-init; (f) iteration-order disclaimer on UnorderedSet/Map; **+(g) guarded the 3 debug-only death tests (`MacrosTest`, FlatMap/UnorderedMap `at()`) under `#ifndef NDEBUG` → release test run now FULLY CLEAN (515/515).** Debug 520/520. | array/fixed_array/flat_set/flat_map/intrusive_list/allocator/priority_queue/unordered_set/unordered_map + 3 test files | **done** |

> **NOT a bug (investigated, R45):** the alleged Robin Hood duplicate-insert (hash_table.cppm:586)
> is **correct** — the dup guard `info == dist && keyEqual` mirrors `findIndex` (:554), and the Robin
> Hood invariant guarantees a present key is reached *before* any `info < dist` steal, so the guard
> stays armed (`insertedIndex == kInvalidIndex`) until it fires. Cheap insurance only: add an "every
> key appears once" pass to the hash stress test.
>
> **Deferred → now scheduled in Phase 9 (R48):** per-slot hash-cache (R40) moves to the next
> iteration alongside `Hash<String>`.

---

## Phase 9 — NEXT ITERATION: string & expensive keys  *(QUEUED — not started)*
Scope set 2026-06-02 (user, **R48**): iteration 1 (Phase S–8) is CLOSED; `Hash<String>` and the work
it unlocks live here. **Hard dependency:** the engine has **no `String`/`StringView` type yet**
(verified — `Hash` primary is declared-undefined, only integral/enum/pointer specializations ship;
`hashBytes` FNV-1a exists as the substrate). A `String` type must land (likely its own effort/tracker)
before `Hash<String>` can start. Each task re-benches string-keyed paths (see [[container-bench]]).

| ID | Task | Rationale / unlocks | State |
|---|---|---|---|
| P9-hash_bytes_mixer | Replace FNV-1a `hashBytes` (hash.cppm:55) with a word-at-a-time mixer (xxHash3 / wyhash / rapidhash-class), keeping the `hashBytes(void const*, len)` seam. | substrate for `Hash<String>`; FNV is per-byte (slow) + weak avalanche. R19/R45. Independent of the String type — can start anytime. | pending (next iter) |
| P9-hash_string | `Hash<String>` (+ `Hash<StringView>` if present), **transparent** so heterogeneous lookup by `StringView`/`char const*` needs no temporary `String`. | the anchor: makes `UnorderedMap<String,V>` / `Set<String>` first-class; reuses P9-hash_bytes_mixer. | pending (next iter — **blocked on a String type**) |
| P9-hash_cache | Un-defer **R40**: cache the hash per slot in `hash_table` + `swiss_table` so probe compares skip the key recompare. Re-bench string-keyed insert/lookup vs eastl. | proven a no-op for int keys (R40), but pays off for EXPENSIVE String compares — the architecture finally has a workload that justifies it. | pending (next iter — follows P9-hash_string) |

> **Also deferred, independent, unscheduled:** **float hashing** (R20) — `Hash<f32>`/`Hash<f64>` need
> `-0.0`/NaN canonicalization. Same "key extension" theme but NOT blocked on `String`; pull into Phase 9
> or a later iteration when a float-keyed container is actually needed. Tracked in memory [[hash-float-deferred]].

---

## Phase 10 — cleanup, intrinsics-wrapping & Doxygen docs  *(QUEUED — NOT started; record-only)*
Scope set 2026-06-02 (user, **R49**): housekeeping on the existing (iteration-1) code. **Independent of
Phase 9** (no `String` dependency) → the recommended next work whenever cleanup is scheduled, since Phase 9
is blocked. **Recon (verified):** style is ALREADY consistent (zero naming-convention violations;
`.clang-format` enforces formatting via the pre-commit hook); **no Doxygen** present (not installed, no
Doxyfile/docs); comments are universally `//` with ZERO Doxygen markup and ZERO TODO/FIXME scratch —
class-level docs exist everywhere but public *methods* are mostly undocumented; `R##` refs are durable
design pointers (keep). ~24 raw intrinsic sites to wrap (`addressOf` already wraps `__builtin_addressof`
at iterator.cppm:121 — the precedent). Order: builtin_wrap → style_audit → doxygen_setup → doc_pass.

**Decisions (R49):** intrinsics → typed fns in a NEW `worse.core.intrinsics` module; Doxygen comment style
→ `/** \brief … */` blocks; doc-pass depth → **Focused** (class docs + top-level public APIs; keep
R##/GAME-PERF notes; skip trivial getters).

| ID | Task | Notes | State |
|---|---|---|---|
| P10-builtin_wrap | New `src/worse/core/intrinsics.cppm` (`export module worse.core.intrinsics;`, imports basic_type only; glob-auto-registered) with typed wrappers: `memCopy`/`memMove`/`memSet`, `constexpr isConstantEvaluated()`, `countTrailingZeros64()` (precond x≠0). Replace the ~24 raw `__builtin_*` sites + `import` the module; keep the `if (!isConstantEvaluated())` guards exactly. Leave `addressOf` (iterator.cppm, already wrapped) + `__STDCPP_DEFAULT_NEW_ALIGNMENT__` (memory.cppm, std macro). **DONE:** new `intrinsics.cppm` (ns `worse::core::intrinsics`, `WE_FORCEINLINE` wrappers; `isConstantEvaluated` is `constexpr`; `countTrailingZeros64` carries a debug `WE_ASSERT(x!=0)`); **25 call-sites** migrated across the 6 modules (each `import`s it); call-site `static_cast<void*>` casts kept verbatim → `-Wnontrivial-memcall` suppression intact; `addressOf`/`__builtin_unreachable`(macro.hpp header)/`__STDCPP_DEFAULT_NEW_ALIGNMENT__` left. Debug **520/520**, release **515/515**; **release bench codegen-neutral** (Array push ±0%, SwissTable insert +0.4%, all within noise — verified via stash before/after). Pitfall logged: a module-exported inline's `WE_ASSERT` bakes at the module's NDEBUG state (debug-built lib carries it into the hot loop) → always bench from `build/release`. | sites: algorithm/modifying·sort, container/memory_util·swiss_table·hash_table·fixed_array | done |
| P10-style_audit | **Confirmation** (recon found no violations): verify camelCase/PascalCase/`m`·`mp`/`k`/`WE_`/in-house-traits/no-`_detail`; fix any stragglers (file-header consistency, `import` ordering); clang-format clean. **DONE — confirmed, no code diff:** clang-format `--dry-run --Werror` clean across all 46 `src/**` files (zero reformat diffs; an initial exit-1 was a `find -o`/word-split artifact, not a violation). New `intrinsics.cppm` adheres: `lowerCamelCase` fns, lowercase ns `intrinsics` (matches `memory`/`container`/`algorithm`), standard `module;`→`#include macro.hpp`→`export module`→`import` header, no `_detail`; `import worse.core.intrinsics;` placed with the core-level imports in all 6 consumers. | scope: 24 container + 6 algorithm + core | done |
| P10-doxygen_setup | `find_package(Doxygen)` + `option(WORSE_BUILD_DOCS)` + `docs` target in CMakeLists; `Doxyfile` (INPUT=src, RECURSIVE, `EXTENSION_MAPPING cppm=C++`, `FILE_PATTERNS += *.cppm`, EXTRACT_ALL=NO); vendor **Doxygen Awesome CSS** (jothepro) under `extern/`, set `HTML_EXTRA_STYLESHEET` + treeview; output gitignored. **Risk:** Doxygen's C++20-module support is limited — verify `.cppm` parsing (export/import/module may need PREDEFINED/filters). Needs `brew install doxygen`. **DONE — wired AND verified end-to-end:** `WORSE_BUILD_DOCS` option + `docs` custom target (gated on `find_package(Doxygen)`, graceful `message(WARNING)` if absent) in CMakeLists; root `Doxyfile` (INPUT=src, `EXTENSION_MAPPING cppm=C++`, `FILE_PATTERNS=*.cppm *.hpp`, EXTRACT_ALL=NO, `OUTPUT_DIRECTORY=build/docs` → gitignored). **C++20-module risk RETIRED:** a `FILTER_PATTERNS` sed strips `module;`/`export module`/`import` and rewrites `export namespace`→`namespace`, `export <decl>`→`<decl>`; `WE_*` macros `PREDEFINED=` (stripped from signatures). Vendored **Doxygen Awesome CSS v2.4.2** (pinned `d52eafe`) under `extern/doxygen-awesome-css/`, trimmed 6.2M→144K (css/js+LICENSE+VERSION); `HTML_EXTRA_STYLESHEET`+treeview+sidebar applied. **Doxygen turned out to be installed (1.17.0)** (recon was wrong) so ran it: **exit 0, ZERO warnings, 196 HTML files, ALL namespaces (worse/core/container/algorithm/intrinsics) + every container/algorithm class register, theme copied.** Member-level docs sparse until P10-doc_pass (EXTRACT_ALL=NO by design). | doxygen 1.17.0 present | done |
| P10-doc_pass | **Focused**, split per module group (contiguous / node-list / hash / ordered / algorithm / core): convert existing class/module `//` blocks → `/** \brief … */` (GAME-PERF → `\note`, keep `R##`); document top-level public APIs (`insert`/`erase`/`find`/`operator[]`/`emplace*`/`push`/`pop`/`reserve`/`sort`/`copy`/`fill`/`lowerBound`/…) with `\param`/`\tparam`/`\return`; trim genuinely verbose dev-narrative; add docs to undocumented exported APIs; skip trivial getters. **DONE — 6 per-group commits** (core `e459878`, contiguous `5ae1b67`, node-list `f817889`, hash `486a130`, ordered `19f4d05`, algorithm — this commit): all 37 modules' class/module `//` blocks → `/** \brief … */`, `GAME-PERF`→`\note`, every `R##` kept; top-level public APIs documented with `\param`/`\tparam`/`\return`/`\pre`/`\note`; trivial getters/aliases/`// --- sep ---`/private helpers left as-is. **Comments-only** — verified zero non-comment lines changed across all 37 files (tokenized git-diff filter, run after every batch). **debug 520/520**; release codegen unaffected (comments). **Doxygen 1.17.0: exit 0, 0 warnings, 261 HTML pages** (was 196 at setup). Two recovery fixes: completed the algorithm free-functions' `\param first/last` coverage to hold the 0-warning bar (Doxygen's all-or-nothing param rule), and finished `flat_set` (a network-interrupted agent had landed only its class brief). Method dispatch: 6 parallel doc-agents (comments-only spec + worked examples); 3 dropped their final socket but their edits were intact — re-ran the 6 untouched files. | follows doxygen_setup | done |

> **Follow-up (post-completion, tx `P10-doc_relocate`):** Doxygen output relocated from `build/docs` (transient, gitignored) to a **committed `docs/html/`**, and the vendored theme moved `extern/doxygen-awesome-css/` → `docs/doxygen-awesome-css/` (`extern/` removed, now empty). `Doxyfile` `OUTPUT_DIRECTORY`/`HTML_EXTRA_STYLESHEET` + the CMake `docs`-target comment updated; `WARN_LOGFILE` → `build/doxygen.warn` (kept transient). Regenerated clean: **0 warnings, 261 pages, theme applied**. Browse entry: `docs/html/index.html`. (To serve via GitHub Pages directly from `/docs`, flatten with `HTML_OUTPUT=.` — deferred unless wanted.)

> **Follow-up (post-completion, tx `P10-doc_guide`):** the auto-extracted reference alone left the user hunting ("收集了散落的注释，还得自己去搜索"). Authored a **curated guide** on top of it (user-chosen: Doxygen pages + topic groups, comprehensive depth): a `\mainpage` (design principles, family map, quickstart), **topic groups** (`\defgroup` containers/ctr_*/algorithms/algo_*/core_infra in `docs/pages/groups.dox`) so the reference tree is navigable by family, two overview pages with **master comparison tables** (`containers.dox`/`algorithms.dox`), a **per-type deep page per family** (`guide_contiguous`/`_nodelist`/`_hash`/`_ordered`/`_algorithms` — purpose, memory model, complexity, iterator/stability contract, when-to-use/avoid, `R##`, code example), and a `core_guide`. Each exported type tagged into its group via `\ingroup` (containers) / `@addtogroup` span (algorithms) — **comments-only** (verified; debug 520/520). Method: 5 parallel agents (1/family) reading the real source + authored connective tissue/tables. Regenerated: **0 warnings, 284 pages**; topic groups populate (verified member lists). New input dir `docs/pages/` (Doxyfile `INPUT`+`FILE_PATTERNS *.dox`). Two agent accuracy fixes folded in: `IntrusiveList` has no `splice`; SwissTable single-insert is `insertUnique`/`emplaceUnique`.

---

## Phase 11 — CI gate for container/algorithm changes  *(QUEUED — NOT started; record-only)*
Scope set 2026-06-02 (user, **R50**): a **path-scoped PR gate** — when files under the container/algorithm
dirs change, CI runs the relevant unit tests + bench, compares bench against `master`, posts a comparison
report, and **blocks the PR unless tests pass AND no benchmark regressed vs master**. Independent of Phases
9/10 (pure infra) — can run anytime. **Decisions (R50):** runner = **same-runner PR-vs-master** (GitHub-hosted;
build+run BOTH branches back-to-back in one job, compare relatively to absorb runner noise — no self-hosted
infra); tolerance = **15%** (fail if PR median > master median × 1.15). **Path scope (user-limited):** trigger
only on `src/worse/core/container/**`, `src/worse/core/algorithm/**` (+ `tests/core/{container,algorithm}/**`
and `bench/**`). ⚠️ **Caveat:** the shared core modules these depend on (`memory`/`utility`/`type_traits`/
`intrinsics`/`macro.hpp`) are OUT of scope per the instruction — a regression introduced THERE won't trip the
gate; revisit if that's not intended. **State of the world:** no CI exists; remote = `w6rsty/worse2026`; bench
currently prints only a human table (needs a machine-readable emit).

| ID | Task | Notes | State |
|---|---|---|---|
| P11-bench_machine_output | Add a machine-readable bench emit to `bench/bench_main.cpp` (nanobench `render()` → CSV/JSON to a path via env/arg, e.g. `WCB_OUT=bench.csv`), keeping the human table. Must expose bench NAME + median ns/op per row for matching. **DONE:** `WCB_OUT=path` → nanobench `templates::csv()` (one row/bench: `title;name;unit;batch;elapsed(median s);…`). **Gotcha:** `Bench::title()` does `mResults.clear()` on title change, so a single render at the end keeps only the LAST suite — must render **after each suite** and de-dup the repeated CSV header (kept first only). Verified: 1 header + **50 data rows** across all 4 suite titles; the cross-suite dup name `Array<int> push x4096 (reserve)` appears twice with different titles → confirms the compare must key on **(title, name)**. Human markdown table still prints to stdout. bench_main.cpp only. | nanobench supports CSV/JSON/custom templates; CSV median col = `elapsed` (seconds/op) | done |
| P11-bench_compare | Comparison script (`scripts/bench_compare.*`): parse PR + master CSVs, match by bench name, compute ratio, flag any `pr_median > master_median × 1.15`, emit a Markdown report (name / master / PR / Δ% / verdict), exit non-zero if any regressed or a bench went missing. **DONE:** `scripts/bench_compare.py` (stdlib only: `csv`/`argparse`/`pathlib`). Matches on **(title, name)** (name alone is ambiguous — repeats across suites); metric = nanobench `elapsed` (median s/op) by header-name lookup (no column-order assumption). Markdown report (Δ% / verdict / bench / suite, worst-first), exit 1 on REGRESS or MISSING, exit 0 otherwise. **No-baseline rule:** empty/absent master CSV → "gate passes" + exit 0 (master predates the bench — R50 caveat / Phase 11 plan pt 3). Verified all 5 cases: equal→0, +30% regress→1, missing→1, PR-only "new"→0, empty master→0. | tolerance 1.15 (R50); CSV median col = `elapsed` | done |
| P11-ci_workflow | `.github/workflows/container-ci.yml`, `on: pull_request` with `paths:` = the scoped dirs above (+ the workflow + CMake). **Test job (hard gate):** configure+build `wcoro_tests`, run container/algorithm suites. **Bench job:** build+run `wcontainer_bench` on PR → csv; `git checkout master`, build+run → csv (SAME runner); run P11-bench_compare; upload report + post to job summary / PR comment; fail on regression. vcpkg binary cache for gtest/nanobench/eastl. | release preset + `-DWORSE_BUILD_BENCH=ON -DVCPKG_MANIFEST_FEATURES=bench` | pending |
| P11-branch_protection | Document/apply the GitHub branch-protection making the test + bench checks REQUIRED to merge into `master` — the "同意PR" gate (a repo setting, not code). | `gh api repos/w6rsty/worse2026/branches/master/protection` | pending |
