/** This source adapted from https://github.com/kmicklas/variadic-static_variant
 *
 * Copyright (C) 2013 Kenneth Micklas
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **/
#pragma once
#include <stdexcept>
#include <typeinfo>
#include <fc/exception/exception.hpp>
#include <boost/core/typeinfo.hpp>

namespace fc {

// Implementation details, the user should not import this:
namespace impl {

template<typename R, int N, typename... Ts>
struct storage_ops;

template<typename X, typename... Ts>
struct position;

template<int Pos, typename... Ts>
struct type_at;

template<typename... Ts>
struct type_info;

template< typename Visitor, typename ...Ts>
struct const_result_type_info;

template< typename Visitor, typename ...Ts>
struct result_type_info;

template< typename Visitor, typename ...Ts>
struct rvalue_ref_result_type_info;

template<typename StaticVariant>
struct copy_construct
{
   StaticVariant& sv;
   copy_construct( StaticVariant& s ):sv(s){}
   template<typename T>
   void operator()( const T& v )const
   {
      sv.init(v);
   }
};

template<typename StaticVariant>
struct move_construct
{
   StaticVariant& sv;
   move_construct( StaticVariant& s ):sv(s){}
   template<typename T>
   void operator()( T& v )const
   {
      sv.init( std::move(v) );
   }
};

template<typename R, int N, typename T, typename... Ts>
struct storage_ops<R, N, T&, Ts...> {
    static void del(int n, void *data) {}
    static void con(int n, void *data) {}

    template<typename visitor>
    static R apply(int n, void *data, visitor&& v) {}

    template<typename visitor>
    static R apply(int n, const void *data, visitor&& v) {}

    template<typename visitor>
    static R apply_rvalue_ref(int n, void *data, visitor&& v) {}

    template<template<typename> class Op>
    static R apply_binary_operator(int n, const void *lhs, const void *rhs) {}
};

template<typename R, int N, typename T, typename... Ts>
struct storage_ops<R, N, T, Ts...> {
    static void del(int n, void *data) {
        if(n == N) reinterpret_cast<T*>(data)->~T();
        else storage_ops<R, N + 1, Ts...>::del(n, data);
    }
    static void con(int n, void *data) {
        if(n == N) new(reinterpret_cast<T*>(data)) T();
        else storage_ops<R, N + 1, Ts...>::con(n, data);
    }

    template<typename visitor>
    static R apply(int n, void *data, visitor&& v) {
        if(n == N) return v(*reinterpret_cast<T*>(data));
        else return storage_ops<R, N + 1, Ts...>::apply(n, data, std::forward<visitor>(v));
    }

    template<typename visitor>
    static R apply(int n, const void *data, visitor&& v) {
        if(n == N) return v(*reinterpret_cast<const T*>(data));
        else return storage_ops<R, N + 1, Ts...>::apply(n, data, std::forward<visitor>(v));
    }

    template<typename visitor>
    static R apply_rvalue_ref(int n, void *data, visitor&& v) {
        if(n == N) return v(std::move(*reinterpret_cast<T*>(data)));
        else return storage_ops<R, N + 1, Ts...>::apply(n, data, std::forward<visitor>(v));
    }

    template<template<typename> class Op>
    static R apply_binary_operator(int n, const void *lhs, const void *rhs) {
       if (n == N) return Op<void>()(*reinterpret_cast<const T*>(lhs), *reinterpret_cast<const T*>(rhs));
       else return storage_ops<R, N + 1, Ts...>::template apply_binary_operator<Op>(n, lhs, rhs);
    }
};

template<typename R, int N>
struct storage_ops<R, N> {
    static void del(int n, void *data) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid.");
    }
    static void con(int n, void *data) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }

    template<typename visitor>
    static R apply(int n, void *data, visitor&& v) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }
    template<typename visitor>
    static R apply(int n, const void *data, visitor&& v) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }

    template<typename visitor>
    static R apply_rvalue_ref(int n, void *data, visitor&& v) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }

    template<template<typename> class Op>
    static R apply_binary_operator(int n, const void *lhs, const void *rhs) {
       FC_THROW_EXCEPTION( fc::assert_exception, "Internal error: static_variant tag is invalid." );
    }
};

template<typename visitor, int N, typename... Ts>
using storage_ops_auto = storage_ops<typename result_type_info<visitor, Ts...>::result_type, N, Ts...>;

template<typename visitor, int N, typename... Ts>
using storage_ops_const_auto = storage_ops<typename const_result_type_info<visitor, Ts...>::result_type, N, Ts...>;

template<typename visitor, int N, typename... Ts>
using storage_ops_revalue_ref_auto = storage_ops<typename rvalue_ref_result_type_info<visitor, Ts...>::result_type, N, Ts...>;

template<typename X>
struct position<X> {
    static const int pos = -1;
};

template<typename X, typename... Ts>
struct position<X, X, Ts...> {
    static const int pos = 0;
};

template<typename X, typename T, typename... Ts>
struct position<X, T, Ts...> {
    static const int pos = position<X, Ts...>::pos != -1 ? position<X, Ts...>::pos + 1 : -1;
};

template<typename T, typename... Ts>
struct type_at<0, T, Ts...> {
   using type = T;
};

template<int Pos, typename T, typename... Ts>
struct type_at<Pos, T, Ts...> {
   using type = typename type_at<Pos - 1, Ts...>::type;
};

template<template<typename> class Op, typename T>
std::is_convertible<std::invoke_result_t<Op<void>, const T&, const T&>, bool> can_invoke_operator_test(int);

template<template<typename> class Op, typename T>
std::false_type can_invoke_operator_test(...);

template<template<typename> class Op, typename T>
using can_invoke_operator = decltype(can_invoke_operator_test<Op, T>(0));

template<typename T, typename... Ts>
struct type_info<T&, Ts...> {
    static const bool no_reference_types = false;
    static const bool no_duplicates = position<T, Ts...>::pos == -1 && type_info<Ts...>::no_duplicates;
    static const size_t size = type_info<Ts...>::size > sizeof(T&) ? type_info<Ts...>::size : sizeof(T&);
    static const size_t count = 1 + type_info<Ts...>::count;

    static const bool has_equal_to = can_invoke_operator<std::equal_to, std::remove_reference_t<T>>::value && type_info<Ts...>::has_equal_to;
    static const bool has_not_equal_to = can_invoke_operator<std::not_equal_to, std::remove_reference_t<T>>::value && type_info<Ts...>::has_not_equal_to;
    static const bool has_less = can_invoke_operator<std::less, std::remove_reference_t<T>>::value && type_info<Ts...>::has_less;
    static const bool has_less_equal = can_invoke_operator<std::less_equal, std::remove_reference_t<T>>::value && type_info<Ts...>::has_less_equal;
    static const bool has_greater = can_invoke_operator<std::greater, std::remove_reference_t<T>>::value && type_info<Ts...>::has_greater;
    static const bool has_greater_equal = can_invoke_operator<std::greater_equal, std::remove_reference_t<T>>::value && type_info<Ts...>::has_greater_equal;
};

template<typename T, typename... Ts>
struct type_info<T, Ts...> {
    static const bool no_reference_types = type_info<Ts...>::no_reference_types;
    static const bool no_duplicates = position<T, Ts...>::pos == -1 && type_info<Ts...>::no_duplicates;
    static const size_t size = type_info<Ts...>::size > sizeof(T) ? type_info<Ts...>::size : sizeof(T&);
    static const size_t count = 1 + type_info<Ts...>::count;

    static const bool has_equal_to = can_invoke_operator<std::equal_to, T>::value && type_info<Ts...>::has_equal_to;
    static const bool has_not_equal_to = can_invoke_operator<std::not_equal_to, T>::value && type_info<Ts...>::has_not_equal_to;
    static const bool has_less = can_invoke_operator<std::less, T>::value && type_info<Ts...>::has_less;
    static const bool has_less_equal = can_invoke_operator<std::less_equal, T>::value && type_info<Ts...>::has_less_equal;
    static const bool has_greater = can_invoke_operator<std::greater, T>::value && type_info<Ts...>::has_greater;
    static const bool has_greater_equal = can_invoke_operator<std::greater_equal, T>::value && type_info<Ts...>::has_greater_equal;
};

template<>
struct type_info<> {
    static const bool no_reference_types = true;
    static const bool no_duplicates = true;
    static const size_t count = 0;
    static const size_t size = 0;
    static const bool has_equal_to = true;
    static const bool has_not_equal_to = true;
    static const bool has_less = true;
    static const bool has_less_equal = true;
    static const bool has_greater = true;
    static const bool has_greater_equal = true;
};

template<typename Visitor, typename T>
struct const_result_type_info<Visitor, T> {
   using result_type = decltype(std::declval<std::decay_t<Visitor>>()(std::declval<const T&>()));
};

template<typename Visitor, typename T, typename ... Ts>
struct const_result_type_info<Visitor, T, Ts...> {
   using result_type = typename const_result_type_info<Visitor,T>::result_type;

   static_assert(
      std::is_same_v<result_type, typename const_result_type_info<Visitor,Ts...>::result_type >,
      "Varying result types are not supported from visitors"
   );
};

template<typename Visitor, typename T>
struct result_type_info<Visitor, T> {
   using result_type = decltype(std::declval<std::decay_t<Visitor>>()(std::declval<T&>()));
};


template<typename Visitor, typename T, typename ... Ts>
struct result_type_info<Visitor, T, Ts...> {
   using result_type = typename result_type_info<Visitor,T>::result_type;

   static_assert(
      std::is_same_v<result_type, typename result_type_info<Visitor,Ts...>::result_type >,
      "Varying result types are not supported from visitors"
   );
};

template<typename Visitor, typename T>
struct rvalue_ref_result_type_info<Visitor, T> {
   using result_type = decltype(std::declval<std::decay_t<Visitor>>()(std::declval<T&&>()));
};

template<typename Visitor, typename T, typename ... Ts>
struct rvalue_ref_result_type_info<Visitor, T, Ts...> {
   using result_type = typename rvalue_ref_result_type_info<Visitor,T>::result_type;

   static_assert(
      std::is_same_v<result_type, typename rvalue_ref_result_type_info<Visitor,Ts...>::result_type >,
      "Varying result types are not supported from visitors"
   );
};


} // namespace impl

template<typename... Types>
class static_variant {
    using type_info = impl::type_info<Types...>;
    static_assert(type_info::no_reference_types, "Reference types are not permitted in static_variant.");
    static_assert(type_info::no_duplicates, "static_variant type arguments contain duplicate types.");

    alignas(Types...) char storage[impl::type_info<Types...>::size];
    int _tag;

    template<typename X>
    void init(X&& x) {
        _tag = impl::position<std::decay_t<X>, Types...>::pos;
        new(storage) std::decay_t<X>( std::forward<X>(x) );
    }

    template<typename StaticVariant>
    friend struct impl::copy_construct;
    template<typename StaticVariant>
    friend struct impl::move_construct;
public:
    template<typename X>
    struct tag
    {
       static_assert(
         impl::position<X, Types...>::pos != -1,
         "Type not in static_variant."
       );
       static const int value = impl::position<X, Types...>::pos;
    };

    static_variant()
    {
       _tag = 0;
       impl::storage_ops<void, 0, Types...>::con(0, storage);
    }

    template<typename... Other>
    static_variant( const static_variant<Other...>& cpy )
    {
       cpy.visit( impl::copy_construct<static_variant>(*this) );
    }

    static_variant( const static_variant& cpy )
    {
       cpy.visit( impl::copy_construct<static_variant>(*this) );
    }

    static_variant( static_variant& cpy )
    : static_variant( const_cast<const static_variant&>(cpy) )
    {}

    static_variant( static_variant&& mv )
    {
       mv.visit( impl::move_construct<static_variant>(*this) );
    }

    template<typename X>
    static_variant(X&& v) {
        static_assert(
            impl::position<std::decay_t<X>, Types...>::pos != -1,
            "Type not in static_variant."
        );
        init(std::forward<X>(v));
    }

   ~static_variant() {
       impl::storage_ops<void, 0, Types...>::del(_tag, storage);
    }


    template<typename X>
    static_variant& operator=(const X& v) {
        static_assert(
            impl::position<X, Types...>::pos != -1,
            "Type not in static_variant."
        );
        this->~static_variant();
        init(v);
        return *this;
    }
    static_variant& operator=( const static_variant& v )
    {
       if( this == &v ) return *this;
       this->~static_variant();
       v.visit( impl::copy_construct<static_variant>(*this) );
       return *this;
    }
    static_variant& operator=( static_variant&& v )
    {
       if( this == &v ) return *this;
       this->~static_variant();
       v.visit( impl::move_construct<static_variant>(*this) );
       return *this;
    }

    template <typename Bool = bool>
    friend std::enable_if_t<type_info::has_equal_to, Bool> operator == ( const static_variant& a, const static_variant& b )
    {
       if (a.which() != b.which()) {
          return false;
       }

       return impl::storage_ops<bool, 0, Types...>::template apply_binary_operator<std::equal_to>(a._tag, a.storage, b.storage);
    }

    template <typename Bool = bool>
    friend std::enable_if_t<type_info::has_not_equal_to, Bool> operator != ( const static_variant& a, const static_variant& b )
    {
       if (a.which() != b.which()) {
          return true;
       }

       return impl::storage_ops<bool, 0, Types...>::template apply_binary_operator<std::not_equal_to>(a._tag, a.storage, b.storage);
    }

    template <typename Bool = bool>
    friend std::enable_if_t<type_info::has_less, Bool> operator < ( const static_variant& a, const static_variant& b )
    {
       if (a.which() > b.which()) {
          return false;
       }

       if (a.which() < b.which()) {
          return true;
       }

       return impl::storage_ops<bool, 0, Types...>::template apply_binary_operator<std::less>(a._tag, a.storage, b.storage);
    }

    template <typename Bool = bool>
    friend std::enable_if_t<type_info::has_less_equal, Bool> operator <= ( const static_variant& a, const static_variant& b )
    {
       if (a.which() > b.which()) {
          return false;
       }

       if (a.which() < b.which()) {
          return true;
       }

       return impl::storage_ops<bool, 0, Types...>::template apply_binary_operator<std::less_equal>(a._tag, a.storage, b.storage);
    }

    template <typename Bool = bool>
    friend std::enable_if_t<type_info::has_greater, Bool> operator > ( const static_variant& a, const static_variant& b )
    {
       if (a.which() < b.which()) {
          return false;
       }

       if (a.which() > b.which()) {
          return true;
       }

       return impl::storage_ops<bool, 0, Types...>::template apply_binary_operator<std::greater>(a._tag, a.storage, b.storage);
    }

    template <typename Bool = bool>
    friend std::enable_if_t<type_info::has_greater_equal, Bool> operator >= ( const static_variant& a, const static_variant& b )
    {
       if (a.which() < b.which()) {
          return false;
       }

       if (a.which() > b.which()) {
          return true;
       }
       
       return impl::storage_ops<bool, 0, Types...>::template apply_binary_operator<std::greater_equal>(a._tag, a.storage, b.storage);
    }

    template<typename X>
    X& get() & {
        static_assert(
            impl::position<X, Types...>::pos != -1,
            "Type not in static_variant."
        );
        if(_tag == impl::position<X, Types...>::pos) {
            void* tmp(storage);
            return *reinterpret_cast<X*>(tmp);
        } else {
            FC_THROW_EXCEPTION( fc::assert_exception, "static_variant does not contain a value of type ${t}", ("t",fc::get_typename<X>::name()) );
           //     std::string("static_variant does not contain value of type ") + typeid(X).name()
           // );
        }
    }
    template<typename X>
    const X& get() const & {
        static_assert(
            impl::position<X, Types...>::pos != -1,
            "Type not in static_variant."
        );
        if(_tag == impl::position<X, Types...>::pos) {
            const void* tmp(storage);
            return *reinterpret_cast<const X*>(tmp);
        } else {
            FC_THROW_EXCEPTION( fc::assert_exception, "static_variant does not contain a value of type ${t}", ("t",fc::get_typename<X>::name()) );
        }
    }
    template<typename X>
    X&& get() && {
        static_assert(
            impl::position<X, Types...>::pos != -1,
            "Type not in static_variant."
        );
        if(_tag == impl::position<X, Types...>::pos) {
            void* tmp(storage);
            return std::move(*reinterpret_cast<X*>(tmp));
        } else {
            FC_THROW_EXCEPTION( fc::assert_exception, "static_variant does not contain a value of type ${t}", ("t",fc::get_typename<X>::name()) );
        }
    }
    template<typename visitor>
    auto visit(visitor&& v) & {
        return impl::storage_ops_auto<visitor, 0, Types...>::apply(_tag, storage, std::forward<visitor>(v));
    }


    template<typename visitor>
    auto visit(visitor&& v) const & {
        return impl::storage_ops_const_auto<visitor, 0, Types...>::apply(_tag, storage, std::forward<visitor>(v));
    }

    template<typename visitor>
    auto visit(visitor&& v) && {
        return impl::storage_ops_revalue_ref_auto<visitor, 0, Types...>::apply_rvalue_ref(_tag, storage, std::forward<visitor>(v));
    }

    template<typename R, typename visitor>
    auto visit(visitor&& v) & {
        return impl::storage_ops<R, 0, Types...>::apply(_tag, storage, std::forward<visitor>(v));
    }


    template<typename R, typename visitor>
    auto visit(visitor&& v) const & {
        return impl::storage_ops<R, 0, Types...>::apply(_tag, storage, std::forward<visitor>(v));
    }

    template<typename R, typename visitor>
    auto visit(visitor&& v) && {
        return impl::storage_ops<R, 0, Types...>::apply_rvalue_ref(_tag, storage, std::forward<visitor>(v));
    }

    static uint32_t count() { return type_info::count; }
    void set_which( uint32_t w ) {
      FC_ASSERT( w < count()  );
      this->~static_variant();
      try {
         _tag = w;
         impl::storage_ops<void, 0, Types...>::con(_tag, storage);
      } catch ( ... ) {
         _tag = 0;
         impl::storage_ops<void, 0, Types...>::con(_tag, storage);
      }
    }

    int which() const {return _tag;}

    template<typename X>
    bool contains() const { return which() == tag<X>::value; }

    template<typename X>
    static constexpr int position() { return impl::position<X, Types...>::pos; }

    template<int Pos, std::enable_if_t<Pos < type_info::size,int> = 1>
    using type_at = typename impl::type_at<Pos, Types...>::type;
};

template<typename Result>
struct visitor {
};

   struct from_static_variant
   {
      variant& var;
      from_static_variant( variant& dv ):var(dv){}

      template<typename T> void operator()( const T& v )const
      {
         to_variant( v, var );
      }
   };

   struct to_static_variant
   {
      const variant& var;
      to_static_variant( const variant& dv ):var(dv){}

      template<typename T> void operator()( T& v )const
      {
         from_variant( var, v );
      }
   };


   template<typename... T> void to_variant( const fc::static_variant<T...>& s, fc::variant& v )
   {
      variant tmp;
      variants vars(2);
      vars[0] = s.which();
      s.visit( from_static_variant(vars[1]) );
      v = std::move(vars);
   }
   template<typename... T> void from_variant( const fc::variant& v, fc::static_variant<T...>& s )
   {
      auto ar = v.get_array();
      if( ar.size() < 2 ) return;
      s.set_which( ar[0].as_uint64() );
      s.visit( to_static_variant(ar[1]) );
   }

  template<typename... T> struct get_typename { static const char* name()   { return BOOST_CORE_TYPEID(static_variant<T...>).name();   } };
} // namespace fc
