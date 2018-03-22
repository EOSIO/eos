// Copyright (c) 2016-2017 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_DETAIL_CONFIG_HPP
#define BOOST_PFR_DETAIL_CONFIG_HPP
#pragma once

// Reminder:
//  * MSVC++ 1?.? _MSC_VER >  1900 (Visual Studio 2017)
//  * MSVC++ 14.0 _MSC_VER == 1900 (Visual Studio 2015)
//  * MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)

#if defined(_MSC_VER)
#   if _MSC_VER <= 1900
#       error Boost.PFR library requires MSVC with c++17 support (Visual Studio 2017 or later).
#   endif
#elif __cplusplus < 201402L
#   error Boost.PFR library requires at least C++14.
#endif

#ifndef BOOST_PFR_USE_LOOPHOLE
#   define BOOST_PFR_USE_LOOPHOLE 1
#endif

#ifndef BOOST_PFR_USE_CPP17
#   ifdef __cpp_structured_bindings
#       define BOOST_PFR_USE_CPP17 1
#   elif defined(_MSC_VER)
#       warning PFR library supports MSVC compiler only with /std:c++latest or /std:c++17 flag. Assuming that you`ve used it. Define `BOOST_PFR_USE_CPP17` to 1 to suppress this warning.
#       define BOOST_PFR_USE_CPP17 1
#   else
#       define BOOST_PFR_USE_CPP17 0
#   endif
#endif

#endif // BOOST_PFR_DETAIL_CONFIG_HPP
