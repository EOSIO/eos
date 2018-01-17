/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/types.hpp>

namespace  eosio {

    /**
     * @brief Forwards lvalues as either lvalues or as rvalues, depending on T 
     * @details This is a helper function to allow perfect forwarding of arguments taken as rvalue references to deduced types, 
     *          preserving any potential move semantics involved.
     * @ingroup type_traits
     * @{
    */
    template<typename T, typename U>
    constexpr decltype(auto) forward(U && u) noexcept
    {
       return static_cast<T &&>(u);
    }

    /// @}

    /**
     * @brief Returns an rvalue reference to arg. 
     * @details This is a helper function to force move semantics on values, even if they have a name: Directly using the returned value 
     *           causes arg to be considered an rvalue. 
     * @ingroup type_traits
     * @{
    */
    template< class T >
    constexpr typename remove_reference<T>::type&& move( T&& t ) noexcept {
       return static_cast<typename remove_reference<decltype(t)>::type&&>(t);
    }

    /// @}

    /**
     * @brief This template is designed to provide compile-time constants as types. 
     * @details It is used by several parts of the standard library as the base class for trait types, 
     *          especially in their bool variant: see true_type and false_type. 
     * @ingroup type_traits
     * @{
    */
    template<class T, T v>
    struct integral_constant {
        static constexpr T value = v;
        typedef T value_type;
        typedef integral_constant type; // using injected-class-name
        constexpr operator value_type() const noexcept { return value; }
        constexpr value_type operator()() const noexcept { return value; } //since c++14
    };

    /// @}

    /**
    *  @defgroup type Builtin Types
    *  @ingroup type_traits
    *  @brief Specifies typedefs , aliases and SFINAE compile time helper utility classes 
    *
    *  @{
    */
    using true_type  = integral_constant<bool,true>;
    using false_type = integral_constant<bool,false>;


    template<class T, class U>
    struct is_same : false_type {};

    template<class T>
    struct is_same<T, T> : true_type {};


    template<class...> struct voidify { using type = void; };
    template<class... Ts> using void_t = typename voidify<Ts...>::type;

    template<class T, class = void>
    struct supports_arithmetic_operations : false_type {};

    //for no default ctor use eosio::declval<T>() +-/* eosio::declval<T>()
    template<class T>
    struct supports_arithmetic_operations<T,
            void_t<decltype(T() + T()),
                    decltype(T() - T()),
                    decltype(T() * T()),
                    decltype(T() / T())>>
            : true_type {};


    namespace detail {
        template<typename T,bool = supports_arithmetic_operations<T>::value>
        struct is_unsigned : integral_constant<bool, T(0) < T(-1)> {};

        template<typename T>
        struct is_unsigned<T,false> : false_type {};
    } // namespace detail

    template<typename T>
    struct is_unsigned : detail::is_unsigned<T>::type {};

    template<bool B, class T = void>
    struct enable_if {};

    template<class T>
    struct enable_if<true, T> { typedef T type; };

    /// @}

} // namespace eosio
