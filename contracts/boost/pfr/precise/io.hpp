// Copyright (c) 2016-2017 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_PFR_PRECISE_IO_HPP
#define BOOST_PFR_PRECISE_IO_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <type_traits>
#include <utility>      // metaprogramming stuff

#include <boost/pfr/detail/sequence_tuple.hpp>
#include <boost/pfr/detail/io.hpp>
#include <boost/pfr/precise/tuple_size.hpp>

#if BOOST_PFR_USE_CPP17
#   include <boost/pfr/detail/core17.hpp>
#else
#   include <boost/pfr/detail/core14.hpp>
#endif

namespace boost { namespace pfr {

/// \brief Writes aggregate `value` to `out`
///
/// \b Requires: C++17 or \constexprinit{C++14 constexpr aggregate intializable type}.
///
/// \rcast14
///
/// \b Example:
/// \code
///     struct my_struct { int i, short s; };
///     my_struct s{12, 13};
///     write(std::cout, s); // outputs '{12, 13}'
/// \endcode
template <class Char, class Traits, class T>
void write(std::basic_ostream<Char, Traits>& out, const T& value) {
    constexpr std::size_t fields_count_val = boost::pfr::detail::fields_count<std::remove_reference_t<T>>();
    out << '{';
#if BOOST_PFR_USE_CPP17
    detail::print_impl<0, fields_count_val>::print(out, detail::tie_as_tuple(value));
#else
    ::boost::pfr::detail::for_each_field_dispatcher(
        value,
        [&out](const auto& val) {
            detail::print_impl<0, fields_count_val>::print(out, val);
        },
        std::make_index_sequence<fields_count_val>{}
    );
#endif
    out << '}';
}

/// Reads aggregate `value` from stream `in`
///
/// \b Requires: C++17 or \constexprinit{C++14 constexpr aggregate intializable type}.
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
void read(std::basic_istream<Char, Traits>& in, T& value) {
    constexpr std::size_t fields_count_val = boost::pfr::detail::fields_count<std::remove_reference_t<T>>();

    const auto prev_exceptions = in.exceptions();
    in.exceptions( typename std::basic_istream<Char, Traits>::iostate(0) );
    const auto prev_flags = in.flags( typename std::basic_istream<Char, Traits>::fmtflags(0) );

    char parenthis = {};
    in >> parenthis;
    if (parenthis != '{') in.setstate(std::basic_istream<Char, Traits>::failbit);

#if BOOST_PFR_USE_CPP17
    detail::read_impl<0, fields_count_val>::read(in, detail::tie_as_tuple(value));
#else
    ::boost::pfr::detail::for_each_field_dispatcher(
        value,
        [&in](const auto& val) {
            detail::read_impl<0, fields_count_val>::read(in, val);
        },
        std::make_index_sequence<fields_count_val>{}
    );
#endif

    in >> parenthis;
    if (parenthis != '}') in.setstate(std::basic_istream<Char, Traits>::failbit);

    in.flags(prev_flags);
    in.exceptions(prev_exceptions);
}

}} // namespace boost::pfr

#endif // BOOST_PFR_PRECISE_IO_HPP
