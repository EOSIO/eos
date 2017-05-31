#pragma once

namespace fc {
   /**
    *  @brief Used to forward declare value types and break circular dependencies or use heap allocation
    *
    *  A smart reference is heap allocated, move-aware, and is gauranteed to never be null (except after a move)
    */
   template<typename T>
   class smart_ref {
     public:
       template<typename U> smart_ref( U&& u );
       template<typename U, typename V> smart_ref( U&& u, V&& v );
       template<typename U, typename V, typename X, typename Y> smart_ref( U&& u, V&& v, X&&, Y&& );
       smart_ref();

       smart_ref( const smart_ref& f );
       smart_ref( smart_ref&& f );

       operator const T&()const;
       operator T&();

       T& operator*();
       const T& operator*()const;
       const T* operator->()const;

       T* operator->();
       bool operator !()const;

       template<typename U>
       T& operator = ( U&& u );

       T& operator = ( smart_ref&& u );
       T& operator = ( const smart_ref& u );

       ~smart_ref();
       
     private:
       T* impl;
   };
} 
