# worse::core::container — DECISIONS (ADR)

Locked-in choices for the container library. Confirmed with the user on 2026-06-01.
Change only with a new dated entry below + the user's sign-off.

## User-confirmed (2026-06-01)
| # | Decision | Choice | Why |
|---|---|---|---|
| D1 | Benchmark framework | **nanobench** (single-header) | runs inside a gtest `TEST`, no extra link surface; separate optional `wcontainer_bench` target gated by `WORSE_BUILD_BENCH OFF`. |
| D2 | Hash container architecture | **open-addressing Robin Hood** | flat/contiguous, cache-friendly, power-of-two + bitmask. Trade-off: refs/iterators invalidate on rehash/erase — documented contract. |
| D3 | Ordered Set/Map | **Flat first** (`FlatSet`/`FlatMap` = sorted Array + binary search) | cache-friendly, mostly composition. Red-black `Set`/`Map` deferred to Phase 5. |
| D4 | Execution cadence | **Phase S → 0 → 1 → 2 with tests, then checkpoint** | de-risk the foundation before the big novel chunks (hash/lists). |

## Adopted defaults (agent-chosen, game-perf oriented)
| # | Decision | Choice | Why |
|---|---|---|---|
| R1 | `IsTriviallyRelocatable<T>` default | opt-in: `is_trivially_copyable` ⇒ true, else user-declares via `WE_DECLARE_TRIVIALLY_RELOCATABLE` | zero false positives; covers most POD game data. |
| R2 | `Pair` field names | public `first`/`second` | keeps structured bindings; the one STL muscle-memory worth honoring. |
| R3 | Strong-guarantee grow rollback | implement, compiled out when element move is `noexcept` / `-fno-exceptions` | no cost on the common path. |
| R4 | `at()` out-of-range | debug `WE_ASSERT` + abort; release UB (no throw) | matches engine's no-exceptions contract. |
| R5 | `StaticArray` storage | plain `T mData[N]` | constexpr-friendly, std::array-like; needs default-constructible T. |
| R6 | `FixedArray` overflow | hard-cap `WE_ASSERT` (no heap spill) | zero-alloc guarantee is the whole point; heap-spill is a later opt-in param. |
| R7 | `FixedArray` size repr | `usize mSize` (NOT a self-pointer) | a pointer-into-self would dangle after a memcpy relocation; an index relocates cleanly. |
| R8 | Hash probe scheme | Robin Hood + backward-shift delete (no tombstones) | see D2. SwissTable is a Phase 6 upgrade behind the same facade. |
| R9 | Hash slot layout | AoS `Pair` slots first | SoA is a later perf option that complicates iterators. |
| R12 | Algorithm API | iterator-pair, no `<ranges>` | avoid `<ranges>` codegen/compile cost in an engine lib. |
| R13 | `stableSort` | deferred | needs O(n) scratch (allocator dep on the algorithm module). |
| R14 | Debug iterator tripwire | include behind `!NDEBUG` | catches cross-thread/invalidation misuse, zero release cost. |
| R15 | `FlatMap` stored element | `Pair<Key, T>` with a **non-const** key | a contiguous backing `Array` must move/relocate elements on insert/erase, which a `const` key forbids. A const-key proxy facade (boost flat_map) fights relocation; deferred. |
| R16 | Flat-container iterator constness | `FlatSet` iterators const-only (keys immutable); `FlatMap` iterators mutable but **only `.second` may change** | mutating a key silently breaks the sorted invariant — documented contract, not enforced. Change a key via erase+insert. |
| R17 | `PriorityQueue` default order | `Less<>` ⇒ **max-heap** (`top()` is largest), `Greater<>` ⇒ min-heap | matches `std::priority_queue`; heap maintenance routes through the `heap.cppm` algos. |
| R18 | `IntrusiveList` shape | move-only, embedded circular sentinel, **O(1) `size()` counter**, nodes externally owned | size counter stays correct only because ALL mutation goes through the API (no self-unlink); move/swap re-seat boundary links to the new anchor. |

## Phase 4 additions — hash family (2026-06-01)
| # | Decision | Choice | Why |
|---|---|---|---|
| R19 | Default hash algorithm | integer keys: **murmur3 `fmix64`** finalizer; byte ranges: **FNV-1a** (`hashBytes`); multi-field: golden-ratio `hashCombine` | strong avalanche so `hash & mask` spreads sequential integer keys on power-of-two tables; cheap + branch-free. `Hash<T>` primary is declared-undefined so unsupported keys fail `HashFor` and users specialize. |
| R20 | **Float hashing deferred** (user-confirmed) | `Hash<T>` ships integral/enum/pointer/bool/char only; `f32`/`f64` deferred | floats need `-0.0`/NaN canonicalization before a bit hash is sound. Tracked in persistent memory `hash-float-deferred`. |
| R21 | Hash-container lookups | **exact `Key` (non-templated)** find/contains/count/erase (user-confirmed) | transparent/heterogeneous hashing needs a transparent `Hash`+`KeyEqual`; a templated `find<K>` under a non-transparent hash silently misses. Deferred to Phase 6 with string types. Side benefit: non-templated `erase(Key)` sidesteps the erase-overload trap. |
| R22 | Hash slot info type | **`u16` DIB** (displacement-from-ideal-bucket), `0`=empty, occupied=`disp+1` | a degenerate hash (all keys colliding) grows displacement with element count — a `u8` (max 254) corrupts under heavy-collision workloads; `u16` keeps them correct at negligible metadata cost, overflow aborts (no-exceptions). Refines the plan's tentative `u8`. No cached hash in v1. |
| R23 | Hash table tuning | `kMinCapacity = 16`, **max load 7/8 (0.875)** compile-time constant (no runtime setter), power-of-two + bitmask, wrap-around probing, **two allocations** (slots + info), lazy (0 capacity until first insert) | game-perf defaults: fewer early rehashes on populate-then-read; 7/8 balances speed/memory for Robin Hood; single-block slots+info layout is a later cache optimization. |
| R24 | Hash table erase | **backward-shift, no tombstones** (per R8); copy preserves exact RH layout; move steals buffers (noexcept, allocator always-equal) | tombstones degrade open-addressing over time; backward-shift keeps probe runs tight. Engine carries a `checkRobinHoodInvariant()` test hook. |

## Phase 5 additions — node lists (2026-06-01)
Game-perf direction (user-confirmed): design for games, reference **EASTL / Unreal**, do NOT blindly
mimic the STL. `list` + `forward_list` this phase; `fixed_list`/`fixed_slist` (inline zero-heap node
pool) deferred to **P5b**; rb_tree + set/map still deferred.
| # | Decision | Choice | Why |
|---|---|---|---|
| R25 | Node-list Base/Derived split | the destroy-all-nodes loop lives in `ListBase`/`ForwardListBase` **destructor** (value-destroy + free per node), NOT the derived dtor as Array does | a node is inseparably storage + a live value, freed in one step, and both need the allocator the base owns; the derived needs no dtor. Deliberate, documented deviation from `ArrayBase`'s raw-free-in-base / element-destroy-in-derived split (there is no raw-vs-live split per node). |
| R26 | Range-splice count accounting | cross-list range `splice`/`spliceAfter` counts the range with `distance` (O(range)); same-list relinking (sort/merge internals) leaves `mSize` untouched (O(1)) | keeps the O(1) `size()` correct; matches counted lists in EASTL/Unreal and the std cross-container wording. |
| R27 | `remove`/`removeIf`/`unique` return type | return `SizeType` (count removed) | game-useful, avoids a re-count; matches C++20 std::list. |
| R28 | `forward_list` shape | **NOT the std-crippled shape**: minimal single-pointer node + the correct `*After` API (beforeBegin/insertAfter/emplaceAfter/eraseAfter/spliceAfter) **plus O(1) `size()`** and the full algorithm surface. Front-ops only — **no `back`/`pushBack`/tail** | the reason to exist over `List` is the smaller node + minimal object; a size counter is what std dropped on purpose but games want it (R30). A tail pointer would change the storage shape — that's a different (queue) structure. |
| R29 | Node-list trivial relocatability | NOT declared trivially relocatable | `List`'s embedded sentinel is the target of self-referential boundary links — a `memcpy` of the container corrupts the ring; kept consistent for `forward_list`. |
| R30 | O(1) cached `size()` on both | `usize mSize` counter | game-ergonomic (Unreal `GetCount` O(1)); accepted tradeoff is R26. The conscious EASTL/Unreal divergence from `std::forward_list`. |
| R31 | `list::sort` / `forward_list::sort` | allocation-free, **stable, bottom-up binned merge sort** (SGI/EASTL), O(log n) stack bins, relinks only | the whole point vs `Array::sort` is zero allocation + stable; `mSize` invariant under sort. |

## Non-negotiable conventions (from the existing tree)
- C++20 modules, one `.cppm` per unit, module name mirrors path.
- camelCase methods, PascalCase types, `m`/`mp` members, `k`/PascalCase static constants, `WE_*` macros.
- All allocation + element lifetime via `AllocatorTraits<Allocator>`; byte-based allocator (not templated on T); `WE_NO_UNIQUE_ADDRESS` EBO.
- No exceptions: `WE_ASSERT`/`WE_VERIFY` + abort + sentinel returns.
- Base/Derived split for heap-owning containers (storage+RAII in `XBase`, API in `X`).
- Math `min/max/clamp/abs` already exist — reuse, never duplicate.
- **In-house trait vocabulary only**: use the `worse::core` `IsXxx`/`RemoveXxx` wrappers from
  `type_traits.cppm`, not raw `std::` trait queries, in module code. Add a wrapper when one is
  missing rather than reaching for `std::`. (Pure metaprogramming primitives with no wrapper —
  `void_t`, `true_type`/`false_type`, `bool_constant` — may stay `std::`.)
- **No `*_detail` sub-namespaces**: internal helpers go in the module's MAIN namespace
  (e.g. `worse::core`), simply left out of the `export` block. Module linkage already hides
  non-exported names from importers with no cross-module clashes (see `allocator_traits.cppm`,
  `heap.cppm`, `sort.cppm`).
