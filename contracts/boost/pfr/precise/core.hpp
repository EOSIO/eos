// Copyright (c) 2016-2017 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_PRECISE_CORE_HPP
#define BOOST_PFR_PRECISE_CORE_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <type_traits>
#include <utility>      // metaprogramming stuff

#include <boost/pfr/detail/sequence_tuple.hpp>
#include <boost/pfr/detail/stdtuple.hpp>
#include <boost/pfr/detail/for_each_field_impl.hpp>

#include <boost/pfr/precise/tuple_size.hpp>
#if BOOST_PFR_USE_CPP17
#   include <boost/pfr/detail/core17.hpp>
#else
#   include <boost/pfr/detail/core14.hpp>
#endif

namespace boost { namespace pfr {

/// \brief Returns reference or const reference to a field with index `I` in aggregate T.
///
/// \b Requires: C++17 or \flatpod{C++14 flat POD or C++14 with not disabled Loophole}.
///
/// \rcast14
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     my_struct s {10, 11};
///     assert(boost::pfr::get<0>(s) == 10);
///     boost::pfr::get<1>(s) = 0;
/// \endcode
template <std::size_t I, class T>
constexpr decltype(auto) get(const T& val) noexcept {
    return detail::sequence_tuple::get<I>( detail::tie_as_tuple(val) );
}


/// \overload get
template <std::size_t I, class T>
constexpr decltype(auto) get(T& val) noexcept {
    return detail::sequence_tuple::get<I>( detail::tie_as_tuple(val) );
}


/// \brief `tuple_element` has a `typedef type-of-a-field-with-index-I-in-aggregate-T type;`
///
/// \b Requires: C++17 or \flatpod{C++14 flat POD or C++14 with not disabled Loophole}.
///
/// \b Example:
/// \code
///     std::vector<  boost::pfr::tuple_element<0, my_structure>::type  > v;
/// \endcode
template <std::size_t I, class T>
using tuple_element = detail::sequence_tuple::tuple_element<I, decltype( ::boost::pfr::detail::tie_as_tuple(std::declval<T&>()) ) >;


/// \brief Type of a field with index `I` in aggregate `T`.
///
/// \b Requires: C++17 or \flatpod{C++14 flat POD or C++14 with not disabled Loophole}.
///
/// \b Example:
/// \code
///     std::vector<  boost::pfr::tuple_element_t<0, my_structure>  > v;
/// \endcode
template <std::size_t I, class T>
using tuple_element_t = typename tuple_element<I, T>::type;


/// \brief Creates an `std::tuple` from an aggregate T.
///
/// \b Requires: C++17 or \flatpod{C++14 flat POD or C++14 with not disabled Loophole}.
///
/// \rcast14
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     my_struct s {10, 11};
///     std::tuple<int, short> t = make_tuple(s);
///     assert(get<0>(t) == 10);
/// \endcode
template <class T>
constexpr auto structure_to_tuple(const T& val) noexcept {
    return detail::make_stdtuple_from_tietuple(
        detail::tie_as_tuple(val),
        std::make_index_sequence< tuple_size_v<T> >()
    );
}


/// \brief Creates an `std::tuple` with lvalue references to fields of an aggregate T.
///
/// \b Requires: C++17 or \flatpod{C++14 flat POD or C++14 with not disabled Loophole}.
///
/// \rcast14
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     my_struct s;
///     tie(s) = std::tuple<int, short>{10, 11};
///     assert(s.s == 11);
/// \endcode
template <class T>
constexpr auto structure_tie(T& val) noexcept {
    return detail::make_stdtiedtuple_from_tietuple(
        detail::tie_as_tuple(val),
        std::make_index_sequence< tuple_size_v<T> >()
    );
}

/// Calls `func` for each field of a `value`.
///
/// \b Requires: C++17 or \constexprinit{C++14 constexpr aggregate intializable type}.
///
/// \param func must have one of the following signatures:
///     * any_return_type func(U&& field)                // field of value is perfect forwarded to function
///     * any_return_type func(U&& field, std::size_t i)
///     * any_return_type func(U&& value, I i)  // Here I is an `std::integral_constant<size_t, field_index>`
///
/// \param value To each field of this variable will be the `func` applied.
///
/// \rcast14
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     int sum = 0;
///     for_each_field(my_struct{20, 22}, [&sum](const auto& field) { sum += field; });
///     assert(sum == 42);
/// \endcode
template <class T, class F>
void for_each_field(T&& value, F&& func) {
    constexpr std::size_t fields_count_val = boost::pfr::detail::fields_count<std::remove_reference_t<T>>();

    ::boost::pfr::detail::for_each_field_dispatcher(
        value,
        [f = std::forward<F>(func)](auto&& t) mutable {
            // MSVC related workaround. It's lambdas do not capture constexprs.
            constexpr std::size_t fields_count_val_in_lambda
                = boost::pfr::detail::fields_count<std::remove_reference_t<T>>();

            ::boost::pfr::detail::for_each_field_impl(
                t,
                std::forward<F>(f),
                std::make_index_sequence<fields_count_val_in_lambda>{},
                std::is_rvalue_reference<T&&>{}
            );
        },
        std::make_index_sequence<fields_count_val>{}
    );
}

}} // namespace boost::pfr

#endif // BOOST_PFR_PRECISE_CORE_HPP
