module;

#include "worse/core/macro.hpp"

#include <concepts>
#include <type_traits>

export module worse.core.type_traits;
import worse.core.basic_type;

/**
 * \file
 * \brief In-house trait/concept layer for the container + algorithm library: a thin
 *        PascalCase skin over `<type_traits>`/`<concepts>` plus the trivial-relocation
 *        trait the standard lacks and games need most.
 * \note The skin is deliberately small (no point reinventing the compiler intrinsics);
 *       the one genuinely new thing is `IsTriviallyRelocatable`, which lets containers
 *       grow/erase/rehash with `memcpy`.
 * \note Lives in `worse::core` (not a `::type_traits` sub-namespace) so this vocabulary
 *       is visible unqualified throughout `worse::core::*`, matching `unreachable()`.
 */
export namespace worse::core
{
    // --- type transformations (PascalCase alias templates) --------------------
    template <typename T>
    using RemoveReference = std::remove_reference_t<T>;
    template <typename T>
    using RemoveConst = std::remove_const_t<T>;
    template <typename T>
    using RemoveCv = std::remove_cv_t<T>;
    template <typename T>
    using RemoveCvRef = std::remove_cvref_t<T>;
    template <typename T>
    using RemovePointer = std::remove_pointer_t<T>;
    template <typename T>
    using RemoveExtent = std::remove_extent_t<T>;
    template <typename T>
    using AddPointer = std::add_pointer_t<T>;
    template <typename T>
    using AddConst = std::add_const_t<T>;
    template <typename T>
    using Decay = std::decay_t<T>;

    /** \brief SFINAE gate: names `T` only when `B` is true, removing the overload otherwise. */
    template <bool B, typename T = void>
    using EnableIf = std::enable_if_t<B, T>;
    /** \brief Compile-time type selection: `T` when `B` is true, else `F`. */
    template <bool B, typename T, typename F>
    using Conditional = std::conditional_t<B, T, F>;
    template <typename T>
    using TypeIdentity = std::type_identity_t<T>;

    // --- category trait values (PascalCase bool variable templates) -----------
    template <typename T>
    inline constexpr bool IsVoid = std::is_void_v<T>;
    template <typename T>
    inline constexpr bool IsIntegral = std::is_integral_v<T>;
    template <typename T>
    inline constexpr bool IsFloating = std::is_floating_point_v<T>;
    template <typename T>
    inline constexpr bool IsPointer = std::is_pointer_v<T>;
    template <typename T>
    inline constexpr bool IsEnum = std::is_enum_v<T>;
    template <typename T>
    inline constexpr bool IsConst = std::is_const_v<T>;
    template <typename T>
    inline constexpr bool IsVolatile = std::is_volatile_v<T>;
    template <typename T>
    inline constexpr bool IsReference = std::is_reference_v<T>;
    template <typename T>
    inline constexpr bool IsEmpty = std::is_empty_v<T>;
    template <typename A, typename B>
    inline constexpr bool IsSame = std::is_same_v<A, B>;

    // --- lifetime / triviality trait values -----------------------------------
    template <typename T>
    inline constexpr bool IsTriviallyCopyable = std::is_trivially_copyable_v<T>;
    template <typename T>
    inline constexpr bool IsTriviallyDestructible = std::is_trivially_destructible_v<T>;
    template <typename T>
    inline constexpr bool IsTriviallyDefaultConstructible = std::is_trivially_default_constructible_v<T>;
    template <typename T>
    inline constexpr bool IsTriviallyMoveConstructible = std::is_trivially_move_constructible_v<T>;
    template <typename T>
    inline constexpr bool IsTriviallyCopyConstructible = std::is_trivially_copy_constructible_v<T>;

    template <typename T>
    inline constexpr bool IsDefaultConstructible = std::is_default_constructible_v<T>;
    template <typename T>
    inline constexpr bool IsMoveConstructible = std::is_move_constructible_v<T>;
    template <typename T>
    inline constexpr bool IsCopyConstructible = std::is_copy_constructible_v<T>;
    template <typename T>
    inline constexpr bool IsMoveAssignable = std::is_move_assignable_v<T>;
    template <typename T>
    inline constexpr bool IsCopyAssignable = std::is_copy_assignable_v<T>;

    template <typename T>
    inline constexpr bool IsNothrowDefaultConstructible = std::is_nothrow_default_constructible_v<T>;
    template <typename T>
    inline constexpr bool IsNothrowMoveConstructible = std::is_nothrow_move_constructible_v<T>;
    template <typename T>
    inline constexpr bool IsNothrowMoveAssignable = std::is_nothrow_move_assignable_v<T>;
    template <typename To, typename From>
    inline constexpr bool IsNothrowAssignable = std::is_nothrow_assignable_v<To, From>;
    template <typename T>
    inline constexpr bool IsNothrowDestructible = std::is_nothrow_destructible_v<T>;

    template <typename T, typename... Args>
    inline constexpr bool IsConstructible = std::is_constructible_v<T, Args...>;
    template <typename From, typename To>
    inline constexpr bool IsConvertible = std::is_convertible_v<From, To>;

    // --- trivial relocation: the centerpiece game-perf trait ------------------

    /**
     * \brief Opt-in hook declaring `T` trivially relocatable; specialize to `value == true`.
     * \note The standard has no relocation trait. Default is `false`; an author opts a type
     *       in via WE_DECLARE_TRIVIALLY_RELOCATABLE (macro.hpp), which specializes this hook.
     */
    template <typename T>
    struct WeIsTriviallyRelocatable
    {
        static constexpr bool value = false;
    };

    /**
     * \brief True when "move-construct at dst then destroy src" is observably a raw byte copy.
     * \note Trivially relocatable types let containers `memcpy` the live range on
     *       grow / erase-shift / rehash instead of looping move+destroy — the game-perf win.
     * \note Trivially-copyable types qualify automatically (covers most POD game data);
     *       anything else is `false` until declared via the `WeIsTriviallyRelocatable` hook.
     */
    template <typename T>
    inline constexpr bool IsTriviallyRelocatable =
        std::is_trivially_copyable_v<T> || WeIsTriviallyRelocatable<RemoveCvRef<T>>::value;

    // --- concepts (PascalCase) — used to constrain containers/algorithms ------
    template <typename T>
    concept TriviallyRelocatable = IsTriviallyRelocatable<T>;

    template <typename T>
    concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

    template <typename T>
    concept EqualityComparable = requires(T const& a, T const& b) {
        { a == b } -> std::convertible_to<bool>;
        { a != b } -> std::convertible_to<bool>;
    };

    template <typename T>
    concept LessThanComparable = requires(T const& a, T const& b) {
        { a < b } -> std::convertible_to<bool>;
    };

    template <typename T>
    concept Swappable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;

    /** \brief A callable usable as a hash: `h(value)` yields something convertible to `usize`. */
    template <typename Hash, typename T>
    concept HashFor = requires(Hash const& h, T const& value) {
        { h(value) } -> std::convertible_to<usize>;
    };

    /** \brief A callable usable as a strict-weak comparator: `cmp(a, b) -> bool`. */
    template <typename Compare, typename T>
    concept CompareFor = requires(Compare const& cmp, T const& a, T const& b) {
        { cmp(a, b) } -> std::convertible_to<bool>;
    };

    /** \brief A unary predicate over `T`: `pred(value) -> bool`. */
    template <typename Pred, typename T>
    concept PredicateFor = requires(Pred const& pred, T const& value) {
        { pred(value) } -> std::convertible_to<bool>;
    };
} // namespace worse::core
