export module worse.core.platform;

/**
 * \file
 * \brief Compile-time platform/architecture detection: the `ESystemPlatform`/`ESystemArch`
 *        enums and the `gSystemPlatform`/`gSystemArch` constants resolved from build macros.
 */
namespace worse::core::platform
{

    /** \brief Host operating-system family. */
    export enum class ESystemPlatform {
        Unknown = 0,
        Windows,
        macOS,
    };

    /** \brief The host platform, resolved from the build's `WE_PLATFORM_*` macros. */
    export constexpr ESystemPlatform gSystemPlatform =
#if defined(WE_PLATFORM_WINDOWS)
        ESystemPlatform::Windows
#elif defined(WE_PLATFORM_MACOS)
        ESystemPlatform::macOS
#else
        ESystemPlatform::Unknown
#endif
        ;

    static_assert(gSystemPlatform != ESystemPlatform::Unknown);

    /** \brief Host CPU architecture. */
    export enum ESystemArch {
        Unknown,
        AMD64,
        AARCH64,
    };

    /** \brief The host architecture, resolved from the build's `WE_ARCH_*` macros. */
    export constexpr ESystemArch gSystemArch =
#if defined(WE_ARCH_AMD64)
        ESystemArch::AMD64
#elif defined(WE_ARCH_AARCH64)
        ESystemArch::AARCH64
#else
        ESystemArch::Unknown
#endif
        ;

    static_assert(gSystemArch != ESystemArch::Unknown);

} // namespace worse::core::platform