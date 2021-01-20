#pragma once

#include <fc/utility.hpp>
#include <fc/fwd.hpp>
#include <new>

namespace fc {

    namespace detail {
      template<typename A, typename U>
      struct add {
        typedef decltype( *((A*)0) + *((typename fc::remove_reference<U>::type*)0) ) type; 
      };
      template<typename A, typename U>
      struct add_eq {
        typedef decltype( *((A*)0) += *((typename fc::remove_reference<U>::type*)0) ) type; 
      };

      template<typename A, typename U>
      struct sub {
        typedef decltype( *((A*)0) - *((typename fc::remove_reference<U>::type*)0) ) type; 
      };

      template<typename A, typename U>
      struct sub_eq {
        typedef decltype( *((A*)0) -= *((typename fc::remove_reference<U>::type*)0) ) type; 
      };
      template<typename A, typename U>
      struct insert_op {
        typedef decltype( *((A*)0) << *((typename fc::remove_reference<U>::type*)0) ) type; 
      };
      template<typename A, typename U>
      struct extract_op {
        A* a;
        U* u;
        typedef decltype( *a >> *u ) type;
      };
    }


    template<typename T, unsigned int S, typename U, typename A>
    auto operator + ( const fwd<T,S,A>& x, U&& u ) -> typename detail::add<T,U>::type { return *x+fc::forward<U>(u); }

    template<typename T, unsigned int S, typename U, typename A>
    auto operator - ( const fwd<T,S,A>& x, U&& u ) -> typename detail::sub<T,U>::type { return *x-fc::forward<U>(u); }

    template<typename T, unsigned int S, typename U, typename A>
    auto operator << ( U& u, const fwd<T,S,A>& f ) -> typename detail::insert_op<U,T>::type { return u << *f; }

    template<typename T, unsigned int S, typename U, typename A>
    auto operator >> ( U& u, fwd<T,S,A>& f ) -> typename detail::extract_op<U,T>::type { return u >> *f; }

    template<typename T, unsigned int S, typename A>
    bool fwd<T,S,A>::operator !()const { return !(**this); }


    template<uint64_t RequiredSize, uint64_t ProvidedSize>
    void check_size() { static_assert( (ProvidedSize >= RequiredSize), "Failed to reserve enough space in fc::fwd<T,S>" ); }

    template<typename T,unsigned int S,typename A>
    template<typename U>
    fwd<T,S,A>::fwd( U&& u ) {
      check_size<sizeof(T),sizeof(_store)>();
      new (this) T( fc::forward<U>(u) );
    }

    template<typename T,unsigned int S,typename A>
    template<typename U,typename V>
    fwd<T,S,A>::fwd( U&& u, V&& v ) {
      check_size<sizeof(T),sizeof(_store)>();
      new (this) T( fc::forward<U>(u), fc::forward<V>(v) );
    }
    template<typename T,unsigned int S,typename A>
    template<typename U,typename V,typename X>
    fwd<T,S,A>::fwd( U&& u, V&& v, X&& x ) {
      check_size<sizeof(T),sizeof(_store)>();
      new (this) T( fc::forward<U>(u), fc::forward<V>(v), fc::forward<X>(x) );
    }
    template<typename T,unsigned int S,typename A>
    template<typename U,typename V,typename X,typename Y>
    fwd<T,S,A>::fwd( U&& u, V&& v, X&& x, Y&&  y ) {
      check_size<sizeof(T),sizeof(_store)>();
      new (this) T( fc::forward<U>(u), fc::forward<V>(v), fc::forward<X>(x), fc::forward<Y>(y) );
    }


    template<typename T,unsigned int S,typename A>
    fwd<T,S,A>::fwd() {
      check_size<sizeof(T),sizeof(_store)>();
      new (this) T;
    }
    template<typename T,unsigned int S,typename A>
    fwd<T,S,A>::fwd( const fwd<T,S,A>& f ){
      check_size<sizeof(T),sizeof(_store)>();
      new (this) T( *f );
    }
    template<typename T,unsigned int S,typename A>
    fwd<T,S,A>::fwd( fwd<T,S,A>&& f ){
      check_size<sizeof(T),sizeof(_store)>();
      new (this) T( fc::move(*f) );
    }



    template<typename T,unsigned int S, typename A>
    fwd<T,S,A>::operator  T&() { return *(( T*)this); }
    template<typename T,unsigned int S, typename A>
    fwd<T,S,A>::operator const T&()const { return *((const T*)this); }

    template<typename T,unsigned int S, typename A>
    T& fwd<T,S,A>::operator*() { return *((T*)this); }
    template<typename T,unsigned int S, typename A>
    const T& fwd<T,S,A>::operator*()const  { return *((const T*)this); }
    template<typename T,unsigned int S, typename A>
    const T* fwd<T,S,A>::operator->()const { return ((const T*)this); }

    template<typename T,unsigned int S, typename A>
    T* fwd<T,S,A>::operator->(){ return ((T*)this); }


    template<typename T,unsigned int S, typename A>
    fwd<T,S,A>::~fwd() {
      ((T*)this)->~T();
    }
    template<typename T,unsigned int S, typename A>
    template<typename U>
    T& fwd<T,S,A>::operator = ( U&& u ) {
      return **this = fc::forward<U>(u);
    }

    template<typename T,unsigned int S, typename A>
    T& fwd<T,S,A>::operator = ( fwd<T,S,A>&& u ) {
      return **this = fc::move(*u);
    }
    template<typename T,unsigned int S, typename A>
    T& fwd<T,S,A>::operator = ( const fwd<T,S,A>& u ) {
      return **this = *u;
    }

} // namespace fc

