module;

#include "worse/core/macro.hpp"

#include <new>

export module worse.core.memory;
import worse.core.basic_type;

namespace worse::core::memory
{
    export constexpr char const* WE_MEMORY_DEFAULT_NAME = "worse";

    export struct AllocInfo
    {
        usize alignmentOffset = 0;
        char const* pName     = WE_MEMORY_DEFAULT_NAME;
        u32 flags             = 0;
        u32 debugFlags        = 0;
        char const* pFile     = nullptr;
        u32 line              = 0;
    };

    // Only route through the ALIGNED new/delete overloads when the request actually
    // over-aligns past the default new alignment (16 on most ABIs). The aligned overload is a
    // slower path on libc++/libstdc++ (posix_memalign / aligned_alloc) than plain operator
    // new, so a node container allocating e.g. a 24-byte 8-aligned node would needlessly pay
    // it on every node -- this keeps the common path on par with std::list / std::allocator.
    // (allocInfo is reserved for the engine's tracking allocator; a no-op in this build.)
    // `inline` so the body is emitted into importers (the definition lives in this module
    // interface): at -O the optimizer can then inline the whole allocate chain and elide the
    // unused `allocInfo` / source_location threading, collapsing the fast path to a bare
    // `operator new` -- on par with std::allocator. (Reinstate a real out-of-line definition
    // here when the tracking allocator lands; the seam stays the same.)
    export inline void* allocate(usize sizeBytes, usize alignment, AllocInfo const& allocInfo)
    {
        (void)allocInfo;
        // nothrow form so OOM is observable as a null return (not a thrown bad_alloc):
        // callers (`stableSort`'s scratch, container node alloc) branch on null to either
        // fall back or `handleAllocationFailure` -> deterministic abort. Keeps the
        // no-exceptions contract; the matching sized delete below is valid for nothrow-new
        // memory (same pool as the throwing form).
        if (alignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__)
        {
            return ::operator new(sizeBytes, std::nothrow);
        }
        return ::operator new(sizeBytes, std::align_val_t{alignment}, std::nothrow);
    }
    export inline void deallocate(void* p, usize sizeBytes, usize alignment)
    {
        if (alignment <= __STDCPP_DEFAULT_NEW_ALIGNMENT__)
        {
            ::operator delete(p, sizeBytes); // sized delete -- the fast path
        }
        else
        {
            ::operator delete(p, sizeBytes, std::align_val_t{alignment});
        }
    }

    export WE_NORETURN void handleAllocationFailure(usize sizeBytes, usize alignment)
    {
        // Unrecoverable: a container that needs this node/buffer cannot proceed. Abort
        // deterministically (no exceptions) -- NOT unreachable()/UB. Now actually
        // reachable since `allocate` returns null on OOM (nothrow new).
        (void)sizeBytes;
        (void)alignment;
        std::abort();
    }

} // namespace worse::core::memory