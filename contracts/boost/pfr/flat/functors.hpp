// Copyright (c) 2016-2017 Antony Polukhin
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_PFR_FLAT_FUNCTORS_HPP
#define BOOST_PFR_FLAT_FUNCTORS_HPP
#pragma once

#include <boost/pfr/detail/config.hpp>

#include <boost/pfr/detail/functional.hpp>
#include <boost/pfr/flat/core.hpp>

/// \file boost/pfr/functors.hpp
/// Contains functors that can work with PODs and are close to the Standard Library ones.
/// Each functor \flattening{flattens} the POD type and iterates over its fields.
///
/// \rcast

namespace boost { namespace pfr {
///////////////////// Comparisons

/// \brief std::equal_to like flattening comparator
template <class T = void> struct flat_equal_to {
    /// \return \b true if each field of \b x equals the field with same index of \b y.
    ///
    /// \rcast
    bool operator()(const T& x, const T& y) const noexcept {
        return detail::equal_impl<0, flat_tuple_size_v<T> >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const noexcept;
#endif
};

/// @cond
template <> struct flat_equal_to<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const noexcept {
        return detail::equal_impl<
            0,
            detail::min_size(flat_tuple_size_v<T>, flat_tuple_size_v<U>)
        >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

};
/// @endcond

/// \brief std::not_equal like flattening comparator
template <class T = void> struct flat_not_equal {
    /// \return \b true if at least one field \b x not equals the field with same index of \b y.
    ///
    /// \rcast
    bool operator()(const T& x, const T& y) const noexcept {
        return detail::not_equal_impl<0, flat_tuple_size_v<T> >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const noexcept;
#endif
};

/// @cond
template <> struct flat_not_equal<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const noexcept {
        return detail::not_equal_impl<0, flat_tuple_size_v<T> >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

    typedef std::true_type is_transparent;
};
/// @endcond

/// \brief std::greater like flattening comparator
template <class T = void> struct flat_greater {
    /// \return \b true if field of \b x greater than the field with same index of \b y and all previous fields of \b x equal to the same fields of \b y.
    ///
    /// \rcast
    bool operator()(const T& x, const T& y) const noexcept {
        return detail::greater_impl<0, flat_tuple_size_v<T> >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const noexcept;
#endif
};

/// @cond
template <> struct flat_greater<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const noexcept {
        return detail::greater_impl<
            0,
            detail::min_size(flat_tuple_size_v<T>, flat_tuple_size_v<U>)
        >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

    typedef std::true_type is_transparent;
};
/// @endcond

/// \brief std::less like flattening comparator
template <class T = void> struct flat_less {
    /// \return \b true if field of \b x less than the field with same index of \b y and all previous fields of \b x equal to the same fields of \b y.
    ///
    /// \rcast
    bool operator()(const T& x, const T& y) const noexcept {
        return detail::less_impl<0, flat_tuple_size_v<T> >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const noexcept;
#endif
};

/// @cond
template <> struct flat_less<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const noexcept {
        return detail::less_impl<
            0,
            detail::min_size(flat_tuple_size_v<T>, flat_tuple_size_v<U>)
        >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

    typedef std::true_type is_transparent;
};
/// @endcond

/// \brief std::greater_equal like flattening comparator
template <class T = void> struct flat_greater_equal {
    /// \return \b true if field of \b x greater than the field with same index of \b y and all previous fields of \b x equal to the same fields of \b y;
    /// or if each field of \b x equals the field with same index of \b y.
    ///
    /// \rcast
    bool operator()(const T& x, const T& y) const noexcept {
        return detail::greater_equal_impl<0, flat_tuple_size_v<T> >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const noexcept;
#endif
};

/// @cond
template <> struct flat_greater_equal<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const noexcept {
        return detail::greater_equal_impl<
            0,
            detail::min_size(flat_tuple_size_v<T>, flat_tuple_size_v<U>)
        >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

    typedef std::true_type is_transparent;
};
/// @endcond

/// \brief std::less_equal like flattening comparator
template <class T = void> struct flat_less_equal {
    /// \return \b true if field of \b x less than the field with same index of \b y and all previous fields of \b x equal to the same fields of \b y;
    /// or if each field of \b x equals the field with same index of \b y.
    ///
    /// \rcast
    bool operator()(const T& x, const T& y) const noexcept {
        return detail::less_equal_impl<0, flat_tuple_size_v<T> >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

#ifdef BOOST_PFR_DOXYGEN_INVOKED
    /// This typedef exists only if T \b is void
    typedef std::true_type is_transparent;

    /// This operator allows comparison of \b x and \b y that have different type.
    /// \pre Exists only if T \b is void.
    template <class V, class U> bool operator()(const V& x, const U& y) const noexcept;
#endif
};

/// @cond
template <> struct flat_less_equal<void> {
    template <class T, class U>
    bool operator()(const T& x, const U& y) const noexcept {
        return detail::less_equal_impl<
            0,
            detail::min_size(flat_tuple_size_v<T>, flat_tuple_size_v<U>)
        >::cmp(detail::tie_as_flat_tuple(x), detail::tie_as_flat_tuple(y));
    }

    typedef std::true_type is_transparent;
};
/// @endcond


/// \brief std::hash like flattening functor
template <class T> struct flat_hash {
    /// \return hash value of \b x.
    ///
    /// \rcast
    std::size_t operator()(const T& x) const noexcept {
        return detail::hash_impl<0, flat_tuple_size_v<T> >::compute(detail::tie_as_flat_tuple(x));
    }
};

}} // namespace boost::pfr

#endif // BOOST_PFR_FLAT_FUNCTORS_HPP
