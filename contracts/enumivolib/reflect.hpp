#pragma once
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/stringize.hpp>

namespace eosio {
  template<typename T>
  struct reflector {
     typedef std::false_type is_reflected;
     typedef std::false_type is_enum;
  };

} /// eosio



#define ENULIB_REFLECT_VISIT_BASE(r, visitor, base) \
  eosio::reflector<base>::visit( visitor );

#define ENULIB_REFLECT_VISIT2_BASE(r, visitor, base) \
  eosio::reflector<base>::visit( t, forward<Visitor>(visitor) );


#define ENULIB_REFLECT_VISIT_MEMBER( r, visitor, elem ) \
{ typedef decltype((static_cast<type*>(nullptr))->elem) member_type;  \
   visitor( &type::elem ); \
}

#define ENULIB_REFLECT_VISIT2_MEMBER( r, visitor, elem ) \
{ typedef decltype((static_cast<type*>(nullptr))->elem) member_type;  \
   visitor( t.elem ); \
}


#define ENULIB_REFLECT_BASE_MEMBER_COUNT( r, OP, elem ) \
  OP eosio::reflector<elem>::total_member_count

#define ENULIB_REFLECT_MEMBER_COUNT( r, OP, elem ) \
  OP 1

#define ENULIB_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
template<typename Visitor>\
static inline void visit( Visitor&& v ) { \
    BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_VISIT_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_VISIT_MEMBER, v, MEMBERS ) \
} \
template<typename Visitor>\
static inline void visit( const type& t, Visitor&& v ) { \
    BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_VISIT2_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_VISIT2_MEMBER, v, MEMBERS ) \
} \
template<typename Visitor>\
static inline void visit( type& t, Visitor&& v ) { \
    BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_VISIT2_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_VISIT2_MEMBER, v, MEMBERS ) \
}

#define ENULIB_REFLECT_DERIVED_IMPL_EXT( TYPE, INHERITS, MEMBERS ) \
template<typename Visitor>\
void eosio::reflector<TYPE>::visit( Visitor&& v ) { \
    BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_VISIT_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_VISIT_MEMBER, v, MEMBERS ) \
}


/**
 *  @def ENULIB_REFLECT(TYPE,MEMBERS)
 *  @brief Specializes eosio::reflector for TYPE
 *
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 *
 *  @see ENULIB_REFLECT_DERIVED
 */
#define ENULIB_REFLECT( TYPE, MEMBERS ) \
    ENULIB_REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

#define ENULIB_REFLECT_TEMPLATE( TEMPLATE_ARGS, TYPE, MEMBERS ) \
    ENULIB_REFLECT_DERIVED_TEMPLATE( TEMPLATE_ARGS, TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

#define ENULIB_REFLECT_EMPTY( TYPE ) \
    ENULIB_REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, BOOST_PP_SEQ_NIL )


#define ENULIB_REFLECT_FWD( TYPE ) \
namespace eosio { \
  template<> struct reflector<TYPE> {\
       typedef TYPE type; \
       typedef eosio::true_type is_reflected; \
       enum  member_count_enum {  \
         local_member_count = BOOST_PP_SEQ_SIZE(MEMBERS), \
         total_member_count = local_member_count BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_BASE_MEMBER_COUNT, +, INHERITS )\
       }; \
       template<typename Visitor> static void visit( Visitor&& v ); \
       template<typename Visitor> static void visit( const type& t, Visitor&& v ); \
       template<typename Visitor> static void visit( type& t, Visitor&& v ); \
  }; }


#define ENULIB_REFLECT_DERIVED_IMPL( TYPE, MEMBERS ) \
    ENULIB_REFLECT_IMPL_DERIVED_EXT( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

#define ENULIB_REFLECT_IMPL( TYPE, MEMBERS ) \
    ENULIB_REFLECT_DERIVED_IMPL_EXT( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )



/**
 *  @def ENULIB_REFLECT_DERIVED(TYPE,INHERITS,MEMBERS)
 *
 *  @brief Specializes eosio::reflector for TYPE where
 *         type inherits other reflected classes
 *
 *  @param INHERITS - a sequence of base class names (basea)(baseb)(basec)
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define ENULIB_REFLECT_DERIVED( TYPE, INHERITS, MEMBERS ) \
namespace eosio {  \
template<> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef eosio::true_type  is_reflected; \
    typedef eosio::false_type is_enum; \
    enum  member_count_enum {  \
      local_member_count = 0  BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_MEMBER_COUNT, +, MEMBERS ),\
      total_member_count = local_member_count BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_BASE_MEMBER_COUNT, +, INHERITS )\
    }; \
    ENULIB_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; }
#define ENULIB_REFLECT_DERIVED_TEMPLATE( TEMPLATE_ARGS, TYPE, INHERITS, MEMBERS ) \
namespace eosio {  \
template<BOOST_PP_SEQ_ENUM(TEMPLATE_ARGS)> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef eosio::true_type  is_defined; \
    typedef eosio::false_type is_enum; \
    enum  member_count_enum {  \
      local_member_count = 0  BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_MEMBER_COUNT, +, MEMBERS ),\
      total_member_count = local_member_count BOOST_PP_SEQ_FOR_EACH( ENULIB_REFLECT_BASE_MEMBER_COUNT, +, INHERITS )\
    }; \
    ENULIB_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; }
