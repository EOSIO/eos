// Copyright (c) 2016-2017 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_FLAT_TUPLE_SIZE_HPP
#define BOOST_PFR_FLAT_TUPLE_SIZE_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <utility>      // metaprogramming stuff

#include <boost/pfr/detail/sequence_tuple.hpp>
#include <boost/pfr/detail/core14.hpp>

namespace boost { namespace pfr {

/// \brief Has a static const member variable `value` that contains fields count in a \flattening{flattened} T.
///
/// \b Example:
/// \code
///     std::array<int, boost::pfr::flat_tuple_size<my_structure>::value > a;
/// \endcode
template <class T>
using flat_tuple_size = boost::pfr::detail::size_t_<decltype(boost::pfr::detail::tie_as_flat_tuple(std::declval<T&>()))::size_v>;


/// \brief `flat_tuple_size_v` is a template variable that contains fields count in a \flattening{flattened} T.
///
/// \b Example:
/// \code
///     std::array<int, boost::pfr::flat_tuple_size_v<my_structure> > a;
/// \endcode
template <class T>
constexpr std::size_t flat_tuple_size_v = flat_tuple_size<T>::value;

}} // namespace boost::pfr

#endif // BOOST_PFR_FLAT_TUPLE_SIZE_HPP
