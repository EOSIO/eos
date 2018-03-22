// Copyright (c) 2016-2017 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_FLAT_GLOBAL_OPS_HPP
#define BOOST_PFR_FLAT_GLOBAL_OPS_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <boost/pfr/flat/functors.hpp>
#include <boost/pfr/flat/core.hpp>
#include <boost/pfr/flat/io.hpp>

/// \file boost/pfr/flat/global_ops.hpp
/// Contains comparison operators and stream operators for any POD types that do not have their own operators.
/// If POD is comparable or streamable using it's own operator (but not it's conversion operator), then the original operator is used.
///
/// \b Example:
/// \code
///     #include <boost/pfr/flat/global_ops.hpp>
///     struct comparable_struct {      // No operators defined for that structure
///         int i; short s; char data[7]; bool bl; int a,b,c,d,e,f;
///     };
///     // ...
///
///     comparable_struct s1 {0, 1, "Hello", false, 6,7,8,9,10,11};
///     comparable_struct s2 {0, 1, "Hello", false, 6,7,8,9,10,11111};
///     assert(s1 < s2);
///     std::cout << s1 << std::endl; // Outputs: {0, 1, H, e, l, l, o, , , 0, 6, 7, 8, 9, 10, 11}
/// \endcode
///
/// \rcast
///
/// \podops for other ways to define operators and more details.
///
/// \b This \b header \b defines:
/// @cond
namespace boost { namespace pfr { namespace detail {

    template <class T, class U>
    using enable_flat_comparisons = std::enable_if_t<
        std::is_same<T, U>::value && std::is_pod<T>::value,
        bool
    >;

}}} // namespace boost::pfr::detail
/// @endcond

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    template <class T> bool operator==(const T& lhs, const T& rhs) noexcept;
    template <class T> bool operator!=(const T& lhs, const T& rhs) noexcept;
    template <class T> bool operator< (const T& lhs, const T& rhs) noexcept;
    template <class T> bool operator> (const T& lhs, const T& rhs) noexcept;
    template <class T> bool operator<=(const T& lhs, const T& rhs) noexcept;
    template <class T> bool operator>=(const T& lhs, const T& rhs) noexcept;

    template <class Char, class Traits, class T>
    std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, const T& value);

    template <class Char, class Traits, class T>
    std::basic_istream<Char, Traits>& operator>>(std::basic_istream<Char, Traits>& in, T& value);

    /// \brief helper function for Boost unordered containers and boost::hash<>.
    template <class T> std::size_t hash_value(const T& value) noexcept;
#else
    template <class T, class U>
    static boost::pfr::detail::enable_flat_comparisons<T, U> operator==(const T& lhs, const U& rhs) noexcept {
        return ::boost::pfr::flat_equal_to<T>{}(lhs, rhs);
    }

    template <class T, class U>
    static boost::pfr::detail::enable_flat_comparisons<T, U> operator!=(const T& lhs, const U& rhs) noexcept {
        return ::boost::pfr::flat_not_equal<T>{}(lhs, rhs);
    }

    template <class T, class U>
    static boost::pfr::detail::enable_flat_comparisons<T, U> operator<(const T& lhs, const U& rhs) noexcept {
        return ::boost::pfr::flat_less<T>{}(lhs, rhs);
    }

    template <class T, class U>
    static boost::pfr::detail::enable_flat_comparisons<T, U> operator>(const T& lhs, const U& rhs) noexcept {
        return ::boost::pfr::flat_greater<T>{}(lhs, rhs);
    }

    template <class T, class U>
    static boost::pfr::detail::enable_flat_comparisons<T, U> operator<=(const T& lhs, const U& rhs) noexcept {
        return ::boost::pfr::flat_less_equal<T>{}(lhs, rhs);
    }

    template <class T, class U>
    static boost::pfr::detail::enable_flat_comparisons<T, U> operator>=(const T& lhs, const U& rhs) noexcept {
        return ::boost::pfr::flat_greater_equal<T>{}(lhs, rhs);
    }

    template <class Char, class Traits, class T>
    static std::enable_if_t<std::is_pod<T>::value, std::basic_ostream<Char, Traits>&> operator<<(std::basic_ostream<Char, Traits>& out, const T& value) {
        ::boost::pfr::flat_write(out, value);
        return out;
    }

    template <class Char, class Traits, class T>
    static std::enable_if_t<std::is_pod<T>::value, std::basic_istream<Char, Traits>&> operator>>(std::basic_istream<Char, Traits>& in, T& value) {
        ::boost::pfr::flat_read(in, value);
        return in;
    }

    template <class T>
    static std::enable_if_t<std::is_pod<T>::value, std::size_t> hash_value(const T& value) noexcept {
        return ::boost::pfr::flat_hash<T>{}(value);
    }
#endif

#endif // BOOST_PFR_FLAT_GLOBAL_OPS_HPP
