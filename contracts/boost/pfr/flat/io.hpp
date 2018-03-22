// Copyright (c) 2016-2017 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_FLAT_IO_HPP
#define BOOST_PFR_FLAT_IO_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <type_traits>
#include <utility>      // metaprogramming stuff

#include <boost/pfr/detail/sequence_tuple.hpp>
#include <boost/pfr/detail/io.hpp>
#include <boost/pfr/detail/core14.hpp>
#include <boost/pfr/flat/tuple_size.hpp>

namespace boost { namespace pfr {

/// \brief Writes \flattening{flattened} POD `value` to `out`
///
/// \rcast14
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     my_struct s{12, 13};
///     flat_write(std::cout, s); // outputs '{12, 13}'
/// \endcode
template <class Char, class Traits, class T>
void flat_write(std::basic_ostream<Char, Traits>& out, const T& value) {
    out << '{';
    detail::print_impl<0, flat_tuple_size_v<T> >::print(out, detail::tie_as_flat_tuple(value));
    out << '}';
}

/// Reads \flattening{flattened} POD `value` from stream `in`
///
/// \rcast14
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     my_struct s;
///     std::stringstream ss;
///     ss << "{ 12, 13 }";
///     ss >> s;
///     assert(s.i == 12);
///     assert(s.i == 13);
/// \endcode
template <class Char, class Traits, class T>
void flat_read(std::basic_istream<Char, Traits>& in, T& value) {
    const auto prev_exceptions = in.exceptions();
    in.exceptions( typename std::basic_istream<Char, Traits>::iostate(0) );
    const auto prev_flags = in.flags( typename std::basic_istream<Char, Traits>::fmtflags(0) );

    char parenthis = {};
    in >> parenthis;
    if (parenthis != '{') in.setstate(std::basic_istream<Char, Traits>::failbit);

    auto tuple = detail::tie_as_flat_tuple(value);
    detail::read_impl<0, flat_tuple_size_v<T> >::read(in, tuple);

    in >> parenthis;
    if (parenthis != '}') in.setstate(std::basic_istream<Char, Traits>::failbit);

    in.flags(prev_flags);
    in.exceptions(prev_exceptions);
}

}} // namespace boost::pfr

#endif // BOOST_PFR_FLAT_IO_HPP
