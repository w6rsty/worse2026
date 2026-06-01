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

## Non-negotiable conventions (from the existing tree)
- C++20 modules, one `.cppm` per unit, module name mirrors path.
- camelCase methods, PascalCase types, `m`/`mp` members, `k`/PascalCase static constants, `WE_*` macros.
- All allocation + element lifetime via `AllocatorTraits<Allocator>`; byte-based allocator (not templated on T); `WE_NO_UNIQUE_ADDRESS` EBO.
- No exceptions: `WE_ASSERT`/`WE_VERIFY` + abort + sentinel returns.
- Base/Derived split for heap-owning containers (storage+RAII in `XBase`, API in `X`).
- Math `min/max/clamp/abs` already exist — reuse, never duplicate.
