export module worse.core.platform;

namespace worse::core::platform
{

    export enum class SystemPlatform {
        Unknown = 0,
        Windows,
        macOS,
    };

    export constexpr SystemPlatform gSystemPlatform =
#if defined(WORSE_PLATFORM_WINDOWS)
        SystemPlatform::Windows
#elif defined(WORSE_PLATFORM_MACOS)
        SystemPlatform::macOS
#else
        SystemPlatform::Unknown
#endif
        ;

    static_assert(gSystemPlatform != SystemPlatform::Unknown);

    export enum SystemArch {
        Unknown,
        AMD64,
        AARCH64,
    };

    export constexpr SystemArch gSystemArch =
#if defined(WE_ARCH_AMD64)
        SystemArch::AMD64
#elif defined(WE_ARCH_AARCH64)
        SystemArch::AARCH64
#else
        SystemArch::Unknown
#endif
        ;

    static_assert(gSystemArch != SystemArch::Unknown);

} // namespace worse::core::platform