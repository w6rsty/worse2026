export module worse.core.platform;

/**
 * \file
 * \brief Compile-time platform/architecture detection: the `SystemPlatform`/`SystemArch`
 *        enums and the `gSystemPlatform`/`gSystemArch` constants resolved from build macros.
 */
namespace worse::core::platform
{

    /** \brief Host operating-system family. */
    export enum class SystemPlatform {
        Unknown = 0,
        Windows,
        macOS,
    };

    /** \brief The host platform, resolved from the build's `WORSE_PLATFORM_*` macros. */
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

    /** \brief Host CPU architecture. */
    export enum SystemArch {
        Unknown,
        AMD64,
        AARCH64,
    };

    /** \brief The host architecture, resolved from the build's `WE_ARCH_*` macros. */
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