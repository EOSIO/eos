#pragma once

#include <fc/utility.hpp>
#include <fc/smart_ref_fwd.hpp>
#include <new>

namespace fc {

    namespace detail {

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

    template<typename T, typename U>
    auto operator << ( U& u, const smart_ref<T>& f ) -> typename detail::insert_op<U,T>::type { return u << *f; }

    template<typename T, typename U>
    auto operator >> ( U& u, smart_ref<T>& f ) -> typename detail::extract_op<U,T>::type { return u >> *f; }

    template<typename T>
    bool smart_ref<T>::operator !()const { return !(**this); }

    template<typename T>
    template<typename U>
    smart_ref<T>::smart_ref( U&& u ) {
      impl = new (this) T( fc::forward<U>(u) );
    }

    template<typename T>
    template<typename U,typename V>
    smart_ref<T>::smart_ref( U&& u, V&& v ) {
      impl = new T( fc::forward<U>(u), fc::forward<V>(v) );
    }
    template<typename T>
    template<typename U,typename V,typename X,typename Y>
    smart_ref<T>::smart_ref( U&& u, V&& v, X&& x, Y&&  y ) {
      impl = new T( fc::forward<U>(u), fc::forward<V>(v), fc::forward<X>(x), fc::forward<Y>(y) );
    }

    template<typename T>
    smart_ref<T>::smart_ref() {
      impl = new T;
    }
    template<typename T>
    smart_ref<T>::smart_ref( const smart_ref<T>& f ){
      impl = new T( *f );
    }
    template<typename T>
    smart_ref<T>::smart_ref( smart_ref<T>&& f ){
      impl = new T( fc::move(*f) );
    }

    template<typename T>
    smart_ref<T>::operator  T&() { return *impl; }
    template<typename T>
    smart_ref<T>::operator const T&()const { return *impl; }

    template<typename T>
    T& smart_ref<T>::operator*() { return *impl; }
    template<typename T>
    const T& smart_ref<T>::operator*()const  { return *impl; }
    template<typename T>
    const T* smart_ref<T>::operator->()const { return impl; }

    template<typename T>
    T* smart_ref<T>::operator->(){ return impl; }

    template<typename T>
    smart_ref<T>::~smart_ref() {
       delete impl;
    }

    template<typename T>
    template<typename U>
    T& smart_ref<T>::operator = ( U&& u ) {
      return **this = fc::forward<U>(u);
    }

    template<typename T>
    T& smart_ref<T>::operator = ( smart_ref<T>&& u ) {
      if( &u == this ) return *impl;
      delete impl;
      impl = u.impl;
      u.impl = nullptr;
      return *impl;
    }

    template<typename T>
    T& smart_ref<T>::operator = ( const smart_ref<T>& u ) {
      if( &u == this ) return *impl;
      return **this = *u;
    }

} // namespace fc

