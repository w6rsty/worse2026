# worse::core::container — PITFALLS

Accumulated gotchas. Append with `track.sh pitfall "<text>"`. Distill the high-signal
ones into persistent-memory files (`~/.claude/projects/-Users-w6rsty-dev-worse/memory/`)
so they auto-load every session; this file is the full log.

## Known up-front (design-time)
- **FixedArray must store `usize mSize`, never a `T* mpEnd` into its own buffer** — a
  self-pointer dangles after a `memcpy` trivial-relocation move, silently corrupting the
  moved-to object. (DECISIONS R7.)
- **C++20 module scan order is dependency-driven, not file order** — a cycle in the import
  DAG fails the build cryptically. Keep the DAG acyclic (see plan §DAG). `memory_util`
  imports `type_traits`+`utility`+`allocator_traits`; nothing imports back up.
- **Open-addressing hash invalidates ALL refs/iterators on rehash/erase** — document on
  every mutating method; never hand out long-lived element pointers. (DECISIONS D2.)
- **No exceptions anywhere** — `-fno-exceptions` is a target. No `throw`, no
  `std::vector`-style strong-guarantee that relies on catch. Use `WE_ASSERT`/`WE_VERIFY` +
  abort + sentinel/`Optional` returns.
- **Route ALL allocation/lifetime through `AllocatorTraits`** — never raw `new`/
  `construct_at`/`destroy_at` in a container; the traits layer honors member
  construct/destroy + source_location threading and is the only sanctioned path.

<!-- runtime gotchas get appended below by track.sh -->
- 2026-06-01 — Repo has a pre-commit clang-format hook: it auto-reformats C++ files and FAILS the first commit; must re-stage the reformatted files and commit again. track.sh done may need a second attempt, or pre-format first.
- 2026-06-01 — utility ADL clash: in-house move/forward/swap live in worse::core and collide with std::move/forward/swap. An UNQUALIFIED call (e.g. forward<T>(x)) whose argument is a std type pulls std's overload in via ADL -> 'call is ambiguous'. Fix: fully-qualify every internal call as worse::core::move/forward/swap (qualification suppresses ADL). Applies to all future container/algorithm code operating on generic T.
- 2026-06-01 — IteratorTraits primary template must be EMPTY for non-iterators. A primary that writes 'using ValueType = typename It::ValueType;' hard-errors (NOT SFINAE) when IteratorTraits<int> is merely NAMED inside a requires-expression, because instantiating the class instantiates every member alias. Fix: empty primary + VoidT<nested types...>-detected partial specialization (std::iterator_traits does the same). Reuse this idiom for any trait that reads nested typedefs.
- 2026-06-01 — __builtin_memcpy / memcpy on a trivially-RELOCATABLE-but-not-trivially-copyable type triggers clang -Wnontrivial-memcall (e.g. relocate<Reloc> where Reloc has a user move ctor+dtor but is opted into IsTriviallyRelocatable). This is the INTENDED use of the relocation fast path. Silence by casting args: __builtin_memcpy(static_cast<void*>(dst), static_cast<void const*>(src), n*sizeof(T)). Recurs in every container grow/rehash that memcpy-relocates.
