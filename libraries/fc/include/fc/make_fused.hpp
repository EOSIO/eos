#pragma once
#include <fc/tuple.hpp>
#include <fc/function.hpp>

namespace fc {
   template<typename R>
   std::function<R, fc::tuple<> > make_fused( const std::function<R>& f ) {
      return [=]( fc::tuple<> ){ return f(); };
   }
   template<typename R,typename A>
   std::function<R(fc::tuple<A>) > make_fused( const std::function<R(A)>& f ) {
      return [f]( fc::tuple<A> t){ return f(t.a); };
   }
   template<typename R,typename A,typename B>
   std::function<R(fc::tuple<A,B>) > make_fused( const std::function<R(A,B)>& f ) {
      return [f]( fc::tuple<A,B> t){ return f(t.a,t.b); };
   }
   template<typename R,typename A,typename B,typename C>
   std::function<R(fc::tuple<A,B,C>) > make_fused( const std::function<R(A,B,C)>& f ) {
      return [f]( fc::tuple<A,B,C> t){ return f(t.a,t.b,t.c); };
   }
   template<typename R,typename A,typename B,typename C,typename D>
   std::function<R(fc::tuple<A,B,C,D>) > make_fused( const std::function<R(A,B,C,D)>& f ) {
      return [f]( fc::tuple<A,B,C> t){ return f(t.a,t.b,t.c,t.d); };
   }
}
