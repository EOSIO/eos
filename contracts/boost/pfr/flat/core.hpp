// Copyright (c) 2016-2017 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_FLAT_CORE_HPP
#define BOOST_PFR_FLAT_CORE_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <type_traits>
#include <utility>      // metaprogramming stuff

#include <boost/pfr/detail/sequence_tuple.hpp>
#include <boost/pfr/detail/stdtuple.hpp>
#include <boost/pfr/detail/core14.hpp>
#include <boost/pfr/detail/for_each_field_impl.hpp>
#include <boost/pfr/flat/tuple_size.hpp>

namespace boost { namespace pfr {

/// \brief Returns reference or const reference to a field with index `I` in \flattening{flattened} T.
///
/// \rcast
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     my_struct s {10, 11};
///     assert(boost::pfr::flat_get<0>(s) == 10);
///     boost::pfr::flat_get<1>(s) = 0;
/// \endcode
template <std::size_t I, class T>
decltype(auto) flat_get(const T& val) noexcept {
    return boost::pfr::detail::sequence_tuple::get<I>( boost::pfr::detail::tie_as_flat_tuple(val) );
}


/// \overload flat_get
template <std::size_t I, class T>
decltype(auto) flat_get(T& val /* @cond */, std::enable_if_t< std::is_trivially_assignable<T, T>::value>* = 0/* @endcond */ ) noexcept {
    return boost::pfr::detail::sequence_tuple::get<I>( boost::pfr::detail::tie_as_flat_tuple(val) );
}


/// \brief `flat_tuple_element` has a `typedef type-of-the-field-with-index-I-in-\flattening{flattened}-T type;`
///
/// \b Example:
/// \code
///     std::vector<  boost::pfr::flat_tuple_element<0, my_structure>::type  > v;
/// \endcode
template <std::size_t I, class T>
using flat_tuple_element = std::remove_reference<
        typename boost::pfr::detail::sequence_tuple::tuple_element<I, decltype(boost::pfr::detail::tie_as_flat_tuple(std::declval<T&>())) >::type
    >;


/// \brief Type of a field with index `I` in \flattening{flattened} `T`
///
/// \b Example:
/// \code
///     std::vector<  boost::pfr::flat_tuple_element_t<0, my_structure>  > v;
/// \endcode
template <std::size_t I, class T>
using flat_tuple_element_t = typename flat_tuple_element<I, T>::type;


/// \brief Creates an `std::tuple` from a \flattening{flattened} T.
///
/// \rcast
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     my_struct s {10, 11};
///     std::tuple<int, short> t = flat_make_tuple(s);
///     assert(flat_get<0>(t) == 10);
/// \endcode
template <class T>
auto flat_structure_to_tuple(const T& val) noexcept {
    return detail::make_stdtuple_from_tietuple(
        detail::tie_as_flat_tuple(val),
        std::make_index_sequence< flat_tuple_size_v<T> >()
    );
}


/// \brief Creates an `std::tuple` with lvalue references to fields of a \flattening{flattened} T.
///
/// \rcast
///
/// \b Requires: `T` must not have const fields.
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     my_struct s;
///     flat_structure_tie(s) = std::tuple<int, short>{10, 11};
///     assert(s.s == 11);
/// \endcode
template <class T>
auto flat_structure_tie(T& val /* @cond */, std::enable_if_t< std::is_trivially_assignable<T, T>::value>* = 0 /* @endcond */) noexcept {
    return detail::make_stdtiedtuple_from_tietuple(
        detail::tie_as_flat_tuple(val),
        std::make_index_sequence< flat_tuple_size_v<T> >()
    );
}

/// Calls `func` for each field of a \flattening{flattened} POD `value`.
///
/// \rcast
///
/// \param func must have one of the following signatures:
///     * any_return_type func(U&& field)                // field of value is perfect forwarded to function
///     * any_return_type func(U&& field, std::size_t i)
///     * any_return_type func(U&& value, I i)  // Here I is an `std::integral_constant<size_t, field_index>`
///
/// \param value After \flattening{flattening} to each field of this variable will be the `func` applied.
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     int sum = 0;
///     for_each_field(my_struct{20, 22}, [&sum](const auto& field) { sum += field; });
///     assert(sum == 42);
/// \endcode
template <class T, class F>
void flat_for_each_field(T&& value, F&& func) {
    auto tup = detail::tie_as_flat_tuple(value);
    ::boost::pfr::detail::for_each_field_impl(
        tup,
        std::forward<F>(func),
        std::make_index_sequence< flat_tuple_size_v<T> >{},
        std::is_rvalue_reference<T&&>{}
    );
}

}} // namespace boost::pfr

#endif // BOOST_PFR_FLAT_CORE_HPP
