module;

#include "worse/core/macro.hpp"

#include <concepts>
#include <memory> // std::construct_at, std::destroy_at
#include <source_location>
#include <type_traits>
#include <utility> // std::forward

export module worse.core.container.allocator_traits;
import worse.core.basic_type;

namespace worse::core::container
{
    // AllocatorTraits is the in-house equivalent of std::allocator_traits for the
    // EASTL-style (byte-based, non-T-templated) allocator contract used here:
    //
    //     void* allocate(bytes, align [, flags] [, source_location]);
    //     void* allocate(bytes, align, offset [, flags] [, source_location]);
    //     void  deallocate(void* p, bytes, align);
    //
    // It exists because std::allocator_traits<A> requires a standard allocator
    // (A::value_type, T* allocate(n), deallocate(p, n)), which this model does
    // not provide. Like std::allocator_traits, this is a purely COMPILE-TIME
    // layer -- every function inlines to a direct allocator call, so it adds no
    // runtime cost. It centralizes:
    //   - raw allocate/deallocate (with graceful source_location threading),
    //   - element construct/destroy (allocator never participates -> construct_at),
    //   - the propagation/equality policy a container must honour.
    //
    // The allocator opts into policy via nested types (all optional):
    //   using IsAlwaysEqual                        = std::true_type;
    //   using PropagateOnContainerCopyAssignment   = std::true_type;
    //   using PropagateOnContainerMoveAssignment   = std::true_type;
    //   using PropagateOnContainerSwap             = std::true_type;
    // Absent ones fall back to standard defaults (propagate = false,
    // is-always-equal = is_empty<A>).

    // --- capability detection (graceful: works with leaner allocators too) ---
    template <typename Allocator>
    concept AllocatesWithDebugInfo =
        requires(Allocator& allocator, usize n, usize alignment, u32 flags, std::source_location location) {
            { allocator.allocate(n, alignment, flags, location) } -> std::convertible_to<void*>;
        };

    template <typename Allocator>
    concept AllocatesWithOffsetDebugInfo =
        requires(Allocator& allocator, usize n, usize alignment, usize offset, u32 flags, std::source_location location) {
            { allocator.allocate(n, alignment, offset, flags, location) } -> std::convertible_to<void*>;
        };

    template <typename Allocator>
    concept AllocatesWithOffset =
        requires(Allocator& allocator, usize n, usize alignment, usize offset) {
            { allocator.allocate(n, alignment, offset) } -> std::convertible_to<void*>;
        };

    template <typename Allocator, typename T, typename... Args>
    concept HasMemberConstruct =
        requires(Allocator& allocator, T* p, Args&&... args) {
            allocator.construct(p, std::forward<Args>(args)...);
        };

    template <typename Allocator, typename T>
    concept HasMemberDestroy = requires(Allocator& allocator, T* p) { allocator.destroy(p); };

    template <typename Allocator>
    concept HasSelectOnCopy =
        requires(Allocator const& allocator) {
            { allocator.selectOnContainerCopyConstruction() } -> std::convertible_to<Allocator>;
        };

    // --- optional nested-trait extraction with standard defaults ---
    template <typename Allocator, typename = void>
    struct AlwaysEqual : std::bool_constant<std::is_empty_v<Allocator>>
    {
    };
    template <typename Allocator>
    struct AlwaysEqual<Allocator, std::void_t<typename Allocator::IsAlwaysEqual>> : Allocator::IsAlwaysEqual
    {
    };

    template <typename Allocator, typename = void>
    struct PropCopy : std::false_type
    {
    };
    template <typename Allocator>
    struct PropCopy<Allocator, std::void_t<typename Allocator::PropagateOnContainerCopyAssignment>>
        : Allocator::PropagateOnContainerCopyAssignment
    {
    };

    template <typename Allocator, typename = void>
    struct PropMove : std::false_type
    {
    };
    template <typename Allocator>
    struct PropMove<Allocator, std::void_t<typename Allocator::PropagateOnContainerMoveAssignment>>
        : Allocator::PropagateOnContainerMoveAssignment
    {
    };

    template <typename Allocator, typename = void>
    struct PropSwap : std::false_type
    {
    };
    template <typename Allocator>
    struct PropSwap<Allocator, std::void_t<typename Allocator::PropagateOnContainerSwap>>
        : Allocator::PropagateOnContainerSwap
    {
    };

    export template <typename Allocator>
    struct AllocatorTraits
    {
        using AllocatorType  = Allocator;
        using SizeType       = usize;
        using DifferenceType = isize;

        // --- policy (compile-time constants a container branches on) ---
        static constexpr bool isAlwaysEqual                      = AlwaysEqual<Allocator>::value;
        static constexpr bool propagateOnContainerCopyAssignment = PropCopy<Allocator>::value;
        static constexpr bool propagateOnContainerMoveAssignment = PropMove<Allocator>::value;
        static constexpr bool propagateOnContainerSwap           = PropSwap<Allocator>::value;

        // --- raw memory ---------------------------------------------------
        // The default source_location argument is evaluated at the CALL SITE
        // (i.e. inside the container), not here, so memory-debug info points at
        // the container that requested the allocation rather than this header.
        WE_NODISCARD static void* allocate(
            Allocator& allocator,
            usize bytes,
            usize alignment,
            std::source_location location = std::source_location::current()) noexcept
        {
            if constexpr (AllocatesWithDebugInfo<Allocator>)
            {
                return allocator.allocate(bytes, alignment, 0u, location);
            }
            else
            {
                return (void)location, allocator.allocate(bytes, alignment);
            }
        }

        WE_NODISCARD static void* allocate(
            Allocator& allocator,
            usize bytes,
            usize alignment,
            usize offset,
            std::source_location location = std::source_location::current()) noexcept
        {
            if constexpr (AllocatesWithOffsetDebugInfo<Allocator>)
            {
                return allocator.allocate(bytes, alignment, offset, 0u, location);
            }
            else if constexpr (AllocatesWithOffset<Allocator>)
            {
                return (void)location, allocator.allocate(bytes, alignment, offset);
            }
            else
            {
                return (void)location, (void)offset, allocator.allocate(bytes, alignment);
            }
        }

        static void deallocate(Allocator& allocator, void* p, usize bytes, usize alignment) noexcept
        {
            allocator.deallocate(p, bytes, alignment);
        }

        // --- element lifetime ---------------------------------------------
        // The allocator does not participate in element construction in this
        // model; we go straight to construct_at/destroy_at (both constexpr).
        // A member construct/destroy is still honoured if an allocator provides
        // one, mirroring std::allocator_traits.
        template <typename T, typename... Args>
        static constexpr T* construct(WE_MAYBE_UNUSED Allocator& allocator, T* p, Args&&... args)
        {
            if constexpr (HasMemberConstruct<Allocator, T, Args...>)
            {
                allocator.construct(p, std::forward<Args>(args)...);
            }
            else
            {
                std::construct_at(p, std::forward<Args>(args)...);
            }
            return p;
        }

        template <typename T>
        static constexpr void destroy(WE_MAYBE_UNUSED Allocator& allocator, T* p) noexcept
        {
            if constexpr (HasMemberDestroy<Allocator, T>)
            {
                allocator.destroy(p);
            }
            else
            {
                std::destroy_at(p);
            }
        }

        // --- policy operations --------------------------------------------
        // Runtime equality: if the allocator is always-equal we skip the call
        // entirely (lets containers take the cheap move/swap path).
        WE_NODISCARD static constexpr bool equal(Allocator const& lhs, Allocator const& rhs) noexcept
        {
            if constexpr (isAlwaysEqual)
            {
                return (void)lhs, (void)rhs, true;
            }
            else
            {
                return lhs == rhs;
            }
        }

        WE_NODISCARD static constexpr Allocator selectOnContainerCopyConstruction(Allocator const& allocator)
        {
            if constexpr (HasSelectOnCopy<Allocator>)
            {
                return allocator.selectOnContainerCopyConstruction();
            }
            else
            {
                return allocator; // standard default: copy the allocator
            }
        }
    };

} // namespace worse::core::container
