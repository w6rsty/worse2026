module;

#include "worse/core/macro.hpp"
#include "worse/core/container/config.hpp"

#include <source_location>
#include <type_traits>

export module worse.core.container.allocator;
import worse.core.basic_type;
import worse.core.memory;

namespace worse::core::container
{

    // Default allocator, not templated on any type
    export class Allocator
    {
    public:
        // Policy advertised to AllocatorTraits. All instances compare equal
        // (operator== below always returns true), so the allocator is
        // always-equal: containers may steal storage on move-assignment and
        // swap pointers unconditionally, and those operations are noexcept.
        using IsAlwaysEqual = std::true_type;

        explicit Allocator(char const* pName = WE_ALLOCATOR_DEFAULT_NAME)
        {
            mpName = pName;
        }
        Allocator(Allocator const& other)
        {
            mpName = other.mpName;
        }
        Allocator(Allocator const& other, char const* pName)
        {
            mpName = pName;
        }

        Allocator& operator=(Allocator const& other)
        {
            mpName = other.mpName;
            return *this;
        }

        // WE_FORCEINLINE so the AllocInfo/source_location tracking provably DCEs to a bare
        // operator new on the hot path (memory::allocate ignores allocInfo in this build).
        // Without it the optimizer leaves Allocator::allocate out-of-line, pinning the tracking
        // + a call frame on every node push/free (~9% vs a thin ::operator new allocator) (R43).
        WE_FORCEINLINE void* allocate(
            usize sizeBytes,
            usize alignment,
            u32 flags                     = 0,
            std::source_location location = std::source_location::current()) noexcept
        {
            return memory::allocate(
                sizeBytes,
                alignment,
                {
                    .pName = mpName,
                    .flags = flags,
                    .pFile = location.file_name(),
                    .line  = location.line(),
                });
        }
        WE_FORCEINLINE void* allocate(
            usize sizeBytes,
            usize alignment,
            usize offset,
            u32 flags                     = 0,
            std::source_location location = std::source_location::current()) noexcept
        {
            return memory::allocate(
                sizeBytes,
                alignment,
                {
                    .alignmentOffset = offset,
                    .pName           = mpName,
                    .flags           = flags,
                    .pFile           = location.file_name(),
                    .line            = location.line(),
                });
        }
        WE_FORCEINLINE void deallocate(void* p, usize size, usize alignment) noexcept
        {
            memory::deallocate(p, size, alignment);
        }

        char const* getName() const noexcept
        {
            return mpName;
        }
        void setName(char const* pName) noexcept
        {
            mpName = pName;
        }

        friend bool operator==(Allocator const& lhs, Allocator const& rhs) noexcept
        {
            return true;
        }

    private:
        char const* mpName = WE_ALLOCATOR_DEFAULT_NAME; // default-init: robust if a future ctor omits it
    };

} // namespace worse::core::container
