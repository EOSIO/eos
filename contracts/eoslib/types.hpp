/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/types.h>

namespace  eosio {

   /**
    *  @brief Converts a base32 symbol into its binary representation, used by string_to_name()
    *
    *  @details Converts a base32 symbol into its binary representation, used by string_to_name()
    *  @ingroup types
    */
   static constexpr char char_to_symbol( char c ) {
      if( c >= 'a' && c <= 'z' )
         return (c - 'a') + 6;
      if( c >= '1' && c <= '5' )
         return (c - '1') + 1;
      return 0;
   }

   /**
    *  @brief Converts a base32 string to a uint64_t. 
    *
    *  @details Converts a base32 string to a uint64_t. This is a constexpr so that
    *  this method can be used in template arguments as well.
    *
    *  @ingroup types
    */
   static constexpr uint64_t string_to_name( const char* str ) {

      uint32_t len = 0;
      while( str[len] ) ++len;

      uint64_t value = 0;

      for( uint32_t i = 0; i <= 12; ++i ) {
         uint64_t c = 0;
         if( i < len && i <= 12 ) c = char_to_symbol( str[i] );

         if( i < 12 ) {
            c &= 0x1f;
            c <<= 64-5*(i+1);
         }
         else {
            c &= 0x0f;
         }

         value |= c;
      }

      return value;
   }

   /**
    * @brief used to generate a compile time uint64_t from the base32 encoded string interpretation of X
    * @ingroup types
    */
   #define N(X) ::eosio::string_to_name(#X)

   /**
    *  @brief wraps a uint64_t to ensure it is only passed to methods that expect a Name
    *  @details wraps a uint64_t to ensure it is only passed to methods that expect a Name and
    *         that no mathematical operations occur.  It also enables specialization of print
    *         so that it is printed as a base32 string.
    *
    *  @ingroup types
    *  @{
    */
   struct name {
      name( uint64_t v = 0 ): value(v) {}
      operator uint64_t()const { return value; }

      friend bool operator==( const name& a, const name& b ) { return a.value == b.value; }
      account_name value = 0;
   };

   /// @}

   /**
    * @ingroup types
    *
    * @{
    */
   template<typename T> struct remove_reference           { typedef T type; };
   template<typename T> struct remove_reference<T&>       { typedef T type; };
   template<typename T> struct remove_reference<const T&> { typedef T type; };
   ///@}

    /**
     * @ingroup types
     * needed for universal references since we build with --nostdlib and thus std::forward<T> is not available
     *  with forward<Args...>(args...) we always guarantee correctness of the calling code
    */
    template<typename T, typename U>
    constexpr decltype(auto) forward(U && u) noexcept
    {
       return static_cast<T &&>(u);
    }

    template< class T >
    constexpr typename remove_reference<T>::type&& move( T&& t ) noexcept {
       return static_cast<typename remove_reference<decltype(t)>::type&&>(t);
    }

    template<class T, T v>
    struct integral_constant {
        static constexpr T value = v;
        typedef T value_type;
        typedef integral_constant type; // using injected-class-name
        constexpr operator value_type() const noexcept { return value; }
        constexpr value_type operator()() const noexcept { return value; } //since c++14
    };
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

    //for no default ctor we'll  need decltype(eosio::declval<T>() +-/* eosio::declval<T>())
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

} // namespace eos
