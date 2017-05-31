#pragma once
#include <fc/utility.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/preprocessor/repeat_from_to.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>

namespace fc {

  /**
   *  Provides a fast-compiling tuple that doesn't use fancy meta-programming
   *  techniques.  It is limited to 4 parameters which is sufficient for most 
   *  methods argument lists which is the primary use case for this tuple. Methods
   *  that require more than 4 parameters are probably better served by defining
   *  a struct.
   *
   *  The members of the tuple are easily visited with a simple visitor functor 
   *  of the form:
   *  @code
   *  struct visitor {
   *    template<typename MemberType>
   *    void operator()( MemberType& m );
   *
   *    template<typename MemberType>
   *    void operator()( const MemberType& m );
   *  };
   *  @endcode
  template<typename A=void, typename B=void,typename C=void, typename D=void>
  struct tuple { 
    tuple(){}
    enum size_enum { size = 4 };

    template<typename AA, typename BB, typename CC, typename DD>
    tuple( AA&& aa, BB&& bb, CC&& cc, DD&& dd )
    :a( fc::forward<AA>(aa) ),
     b( fc::forward<BB>(bb) ),
     c( fc::forward<CC>(cc) ),
     d( fc::forward<DD>(dd) )
     {}

    template<typename V>
    void visit( V&& v ) { v(a); v(b); v(c); v(d); }
    template<typename V>
    void visit( V&& v )const { v(a); v(b); v(c); v(d); }

    A a; 
    B b; 
    C c; 
    D d; 
  }; 
   */
        template<BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(9, typename A, void)> struct tuple{};

        template<>
        struct tuple<> {
          enum size_enum { size = 0 };
          template<typename V> 
          void visit( V&& v)const{};
        };
        template<typename Functor> 
        auto call_fused( Functor f, const tuple<>& t )  -> decltype( f( ) ) {
          return f();
        } 

        inline tuple<> make_tuple(){ return tuple<>(); }

        template<typename T>
        struct is_tuple {
          typedef fc::false_type type;
        };

        #define RREF_PARAMS(z,n,data) BOOST_PP_CAT(AA,n)&& BOOST_PP_CAT(p,n)
        #define ILIST_PARAMS(z,n,data) BOOST_PP_CAT(a,n)( fc::forward<BOOST_PP_CAT(AA,n)>( BOOST_PP_CAT(p,n) ) )
        #define ILIST_PARAMS_COPY(z,n,data) BOOST_PP_CAT(a,n)( t.BOOST_PP_CAT(a,n)  )
        #define VISIT_PARAMS(z,n,data) v(BOOST_PP_CAT(a,n));
        #define LIST_MEMBERS_ON(z,n,data) data.BOOST_PP_CAT(a,n)
        #define DEDUCE_MEMBERS(z,n,data) typename fc::deduce<BOOST_PP_CAT(AA,n)>::type
        #define FORWARD_PARAMS(z,n,data) fc::forward<BOOST_PP_CAT(AA,n)>(BOOST_PP_CAT(p,n))
        #define MEM_PARAMS(z,n,data) BOOST_PP_CAT(A,n) BOOST_PP_CAT(a,n);
        #define TUPLE(z,n,unused) \
        template<BOOST_PP_ENUM_PARAMS( n, typename A)> \
        struct tuple<BOOST_PP_ENUM_PARAMS(n,A)> { \
            enum size_enum { size = n }; \
            template<BOOST_PP_ENUM_PARAMS( n, typename AA)> \
            explicit tuple( BOOST_PP_ENUM(n, RREF_PARAMS, unused ) )BOOST_PP_IF(n,:,BOOST_PP_EMPTY())BOOST_PP_ENUM( n, ILIST_PARAMS,unused){} \
            tuple( const tuple& t )BOOST_PP_IF(n,:,BOOST_PP_EMPTY())BOOST_PP_ENUM( n, ILIST_PARAMS_COPY,unused){} \
            tuple( tuple&& t )BOOST_PP_IF(n,:,BOOST_PP_EMPTY())BOOST_PP_ENUM( n, ILIST_PARAMS_COPY,unused){} \
            tuple(){}\
            template<typename V>\
            void visit( V&& v ) { BOOST_PP_REPEAT(n,VISIT_PARAMS,a) }\
            template<typename V>\
            void visit( V&& v )const { BOOST_PP_REPEAT(n,VISIT_PARAMS,a) }\
            BOOST_PP_REPEAT(n,MEM_PARAMS,a) \
        };  \
        template<BOOST_PP_ENUM_PARAMS( n, typename AA)> \
        tuple<BOOST_PP_ENUM_PARAMS(n,AA)> make_tuple( BOOST_PP_ENUM( n, RREF_PARAMS, unused) ) { \
          return tuple<BOOST_PP_ENUM_PARAMS(n,AA)>( BOOST_PP_ENUM( n, FORWARD_PARAMS,unused ) );  \
        } \
        template<typename Functor, BOOST_PP_ENUM_PARAMS(n,typename AA)> \
        auto call_fused( Functor f, tuple<BOOST_PP_ENUM_PARAMS(n,AA)>& t )  \
          -> decltype( f( BOOST_PP_ENUM( n, LIST_MEMBERS_ON, t) ) ) { \
          return f( BOOST_PP_ENUM( n, LIST_MEMBERS_ON, t) ); \
        } \
        template<typename Functor, BOOST_PP_ENUM_PARAMS(n,typename AA)> \
        auto call_fused( Functor f, const tuple<BOOST_PP_ENUM_PARAMS(n,AA)>& t )  \
          -> decltype( f( BOOST_PP_ENUM( n, LIST_MEMBERS_ON, t) ) ) { \
          return f( BOOST_PP_ENUM( n, LIST_MEMBERS_ON, t) ); \
        } \
        template<BOOST_PP_ENUM_PARAMS( n, typename AA)> \
        struct is_tuple<fc::tuple<BOOST_PP_ENUM_PARAMS(n,AA)> > { \
          typedef fc::true_type type; \
        }; \
        template<BOOST_PP_ENUM_PARAMS( n, typename AA)> \
        struct deduce<fc::tuple<BOOST_PP_ENUM_PARAMS(n,AA)> > { \
          typedef fc::tuple<BOOST_PP_ENUM( n, DEDUCE_MEMBERS,unused)>  type; \
        }; 

        BOOST_PP_REPEAT_FROM_TO( 1, 5, TUPLE, unused )


        #undef FORWARD_PARAMS
        #undef DEDUCE_MEMBERS
        #undef RREF_PARAMS
        #undef LIST_MEMBERS_ON
        #undef ILIST_PARAMS
        #undef ILIST_PARAMS_COPY
        #undef VISIT_PARAMS
        #undef MEM_PARAMS
        #undef TUPLE


}

