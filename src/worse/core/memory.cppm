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

    export void* allocate(usize sizeBytes, usize alignment, AllocInfo const& allocInfo)
    {
        return ::operator new(sizeBytes, std::align_val_t{alignment});
    }
    export void deallocate(void* p, usize sizeBytes, usize alignment)
    {
        ::operator delete(p, std::align_val_t{alignment});
    }

    export WE_NORETURN void handleAllocationFailure(usize sizeBytes, usize alignment)
    {
        unreachable();
    }

} // namespace worse::core::memory