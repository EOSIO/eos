#pragma once
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/stringize.hpp>

namespace arisen {
  template<typename T>
  struct reflector {
     typedef std::false_type is_reflected;
     typedef std::false_type is_enum;
  };

} /// eosio



#define RSNLIB_REFLECT_VISIT_BASE(r, visitor, base) \
  arisen::reflector<base>::visit( visitor );

#define RSNLIB_REFLECT_VISIT2_BASE(r, visitor, base) \
  arisen::reflector<base>::visit( t, forward<Visitor>(visitor) );


#define RSNLIB_REFLECT_VISIT_MEMBER( r, visitor, elem ) \
{ typedef decltype((static_cast<type*>(nullptr))->elem) member_type;  \
   visitor( &type::elem ); \
}

#define RSNLIB_REFLECT_VISIT2_MEMBER( r, visitor, elem ) \
{ typedef decltype((static_cast<type*>(nullptr))->elem) member_type;  \
   visitor( t.elem ); \
}


#define RSNLIB_REFLECT_BASE_MEMBER_COUNT( r, OP, elem ) \
  OP arisen::reflector<elem>::total_member_count

#define RSNLIB_REFLECT_MEMBER_COUNT( r, OP, elem ) \
  OP 1

#define RSNLIB_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
template<typename Visitor>\
static inline void visit( Visitor&& v ) { \
    BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_VISIT_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_VISIT_MEMBER, v, MEMBERS ) \
} \
template<typename Visitor>\
static inline void visit( const type& t, Visitor&& v ) { \
    BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_VISIT2_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_VISIT2_MEMBER, v, MEMBERS ) \
} \
template<typename Visitor>\
static inline void visit( type& t, Visitor&& v ) { \
    BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_VISIT2_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_VISIT2_MEMBER, v, MEMBERS ) \
}

#define RSNLIB_REFLECT_DERIVED_IMPL_EXT( TYPE, INHERITS, MEMBERS ) \
template<typename Visitor>\
void arisen::reflector<TYPE>::visit( Visitor&& v ) { \
    BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_VISIT_BASE, v, INHERITS ) \
    BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_VISIT_MEMBER, v, MEMBERS ) \
}


/**
 * @addtogroup serializecpp
 * @{
 */

/**
 *  Perform class reflection
 *  
 *  @brief Specializes arisen::reflector for TYPE
 *  @param TYPE - the class template to be reflected
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 *
 *  @see RSNLIB_REFLECT_DERIVED
 */
#define RSNLIB_REFLECT( TYPE, MEMBERS ) \
    RSNLIB_REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

/**
 *  Perform class template reflection
 *  
 *  @brief Perform class template reflection
 *  @param TEMPLATE_ARGS - a sequence of template args. (args1)(args2)(args3)
 *  @param TYPE - the class template to be reflected
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define RSNLIB_REFLECT_TEMPLATE( TEMPLATE_ARGS, TYPE, MEMBERS ) \
    RSNLIB_REFLECT_DERIVED_TEMPLATE( TEMPLATE_ARGS, TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

/**
 *  Perform class reflection on empty class
 *  
 *  @brief Perform class reflection on empty class
 *  @param TYPE - the class to be reflected
 */
#define RSNLIB_REFLECT_EMPTY( TYPE ) \
    RSNLIB_REFLECT_DERIVED( TYPE, BOOST_PP_SEQ_NIL, BOOST_PP_SEQ_NIL )

/**
 *  Perform forward declaration of class reflection
 *  
 *  @brief Perform forward declaration of class reflection
 *  @param TYPE - the class to be reflected
 */
#define RSNLIB_REFLECT_FWD( TYPE ) \
namespace arisen { \
  template<> struct reflector<TYPE> {\
       typedef TYPE type; \
       typedef arisen::true_type is_reflected; \
       enum  member_count_enum {  \
         local_member_count = BOOST_PP_SEQ_SIZE(MEMBERS), \
         total_member_count = local_member_count BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_BASE_MEMBER_COUNT, +, INHERITS )\
       }; \
       template<typename Visitor> static void visit( Visitor&& v ); \
       template<typename Visitor> static void visit( const type& t, Visitor&& v ); \
       template<typename Visitor> static void visit( type& t, Visitor&& v ); \
  }; }

///@}

#define RSNLIB_REFLECT_DERIVED_IMPL( TYPE, MEMBERS ) \
    RSNLIB_REFLECT_IMPL_DERIVED_EXT( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )

#define RSNLIB_REFLECT_IMPL( TYPE, MEMBERS ) \
    RSNLIB_REFLECT_DERIVED_IMPL_EXT( TYPE, BOOST_PP_SEQ_NIL, MEMBERS )


/**
 * @addtogroup serializecpp
 * @{
 */

/**
 *  Perform class reflection where TYPE inherits other reflected classes
 *
 *  @brief Specializes arisen::reflector for TYPE where
 *         type inherits other reflected classes
 * 
 *  @param TYPE - the class to be reflected
 *  @param INHERITS - a sequence of base class names (basea)(baseb)(basec)
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define RSNLIB_REFLECT_DERIVED( TYPE, INHERITS, MEMBERS ) \
namespace arisen {  \
template<> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef arisen::true_type  is_reflected; \
    typedef arisen::false_type is_enum; \
    enum  member_count_enum {  \
      local_member_count = 0  BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_MEMBER_COUNT, +, MEMBERS ),\
      total_member_count = local_member_count BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_BASE_MEMBER_COUNT, +, INHERITS )\
    }; \
    RSNLIB_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; }

/**
 *  Perform class template reflection where TYPE inherits other reflected classes
 *
 *  @brief Perform class template reflection where TYPE inherits other reflected classes
 *  
 *  @param TEMPLATE_ARGS - a sequence of template args. (args1)(args2)(args3)
 *  @param TYPE - the class to be reflected
 *  @param INHERITS - a sequence of base class names (basea)(baseb)(basec)
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define RSNLIB_REFLECT_DERIVED_TEMPLATE( TEMPLATE_ARGS, TYPE, INHERITS, MEMBERS ) \
namespace arisen {  \
template<BOOST_PP_SEQ_ENUM(TEMPLATE_ARGS)> struct reflector<TYPE> {\
    typedef TYPE type; \
    typedef arisen::true_type  is_defined; \
    typedef arisen::false_type is_enum; \
    enum  member_count_enum {  \
      local_member_count = 0  BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_MEMBER_COUNT, +, MEMBERS ),\
      total_member_count = local_member_count BOOST_PP_SEQ_FOR_EACH( RSNLIB_REFLECT_BASE_MEMBER_COUNT, +, INHERITS )\
    }; \
    RSNLIB_REFLECT_DERIVED_IMPL_INLINE( TYPE, INHERITS, MEMBERS ) \
}; }


///@}
