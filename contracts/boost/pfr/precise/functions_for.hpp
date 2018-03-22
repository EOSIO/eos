// Copyright (c) 2016-2017 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_PRECISE_FUNCTIONS_FOR_HPP
#define BOOST_PFR_PRECISE_FUNCTIONS_FOR_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <boost/pfr/precise/functors.hpp>
#include <boost/pfr/precise/io.hpp>

/// \def BOOST_PFR_PRECISE_FUNCTIONS_FOR(T)
/// Defines comparison operators and stream operators for T.
/// If type T is comparable or streamable using it's own operator (but not it's conversion operator), then the original operator is used.
///
/// \b Example:
/// \code
///     #include <boost/pfr/precise/functions_for.hpp>
///     struct comparable_struct {      // No operators defined for that structure
///         int i; short s; char data[7]; bool bl; int a,b,c,d,e,f;
///     };
///     BOOST_PFR_PRECISE_FUNCTIONS_FOR(comparable_struct)
///     // ...
///
///     comparable_struct s1 {0, 1, "Hello", false, 6,7,8,9,10,11};
///     comparable_struct s2 {0, 1, "Hello", false, 6,7,8,9,10,11111};
///     assert(s1 < s2);
///     std::cout << s1 << std::endl; // Outputs: {0, 1, H, e, l, l, o, , , 0, 6, 7, 8, 9, 10, 11}
/// \endcode
///
/// \rcast14
///
/// \podops for other ways to define operators and more details.
///
/// \b Defines \b following \b for \b T:
/// \code
/// bool operator==(const T& lhs, const T& rhs);
/// bool operator!=(const T& lhs, const T& rhs);
/// bool operator< (const T& lhs, const T& rhs);
/// bool operator> (const T& lhs, const T& rhs);
/// bool operator<=(const T& lhs, const T& rhs);
/// bool operator>=(const T& lhs, const T& rhs);
///
/// template <class Char, class Traits>
/// std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, const T& value);
///
/// template <class Char, class Traits>
/// std::basic_istream<Char, Traits>& operator>>(std::basic_istream<Char, Traits>& in, T& value);
///
/// // helper function for Boost unordered containers and boost::hash<>.
/// std::size_t hash_value(const T& value);
/// \endcode

#define BOOST_PFR_PRECISE_FUNCTIONS_FOR(T)                                                                              \
    static inline bool operator==(const T& lhs, const T& rhs) { return ::boost::pfr::equal_to<T>{}(lhs, rhs);      }    \
    static inline bool operator!=(const T& lhs, const T& rhs) { return ::boost::pfr::not_equal<T>{}(lhs, rhs);     }    \
    static inline bool operator< (const T& lhs, const T& rhs) { return ::boost::pfr::less<T>{}(lhs, rhs);          }    \
    static inline bool operator> (const T& lhs, const T& rhs) { return ::boost::pfr::greater<T>{}(lhs, rhs);       }    \
    static inline bool operator<=(const T& lhs, const T& rhs) { return ::boost::pfr::less_equal<T>{}(lhs, rhs);    }    \
    static inline bool operator>=(const T& lhs, const T& rhs) { return ::boost::pfr::greater_equal<T>{}(lhs, rhs); }    \
    template <class Char, class Traits>                                                                                 \
    static ::std::basic_ostream<Char, Traits>& operator<<(::std::basic_ostream<Char, Traits>& out, const T& value) {    \
        ::boost::pfr::write(out, value);                                                                                \
        return out;                                                                                                     \
    }                                                                                                                   \
    template <class Char, class Traits>                                                                                 \
    static ::std::basic_istream<Char, Traits>& operator>>(::std::basic_istream<Char, Traits>& in, T& value) {           \
        ::boost::pfr::read(in, value);                                                                                  \
        return in;                                                                                                      \
    }                                                                                                                   \
    static inline std::size_t hash_value(const T& v) {                                                                  \
        return ::boost::pfr::hash<T>{}(v);                                                                              \
    }                                                                                                                   \
/**/

#endif // BOOST_PFR_PRECISE_FUNCTIONS_FOR_HPP


