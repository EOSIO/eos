#pragma once

#include <stddef.h>                                   /* offsetof */
#include <type_traits>                                /* is_base_of */

#include <boost/hana/map.hpp>                         /* make_map */
#include <boost/hana/pair.hpp>                        /* make_pair */
#include <boost/hana/type.hpp>                        /* type_c */
#include <boost/hana/find.hpp>                        /* find */
#include <boost/preprocessor/seq/for_each.hpp>        /* BOOST_PP_SEQ_FOR_EACH */
#include <boost/preprocessor/seq/for_each_i.hpp>      /* BOOST_PP_SEQ_FOR_EACH_I */
#include <boost/preprocessor/seq/size.hpp>            /* BOOST_PP_SEQ_SIZE */
#include <boost/preprocessor/seq/elem.hpp>            /* BOOST_PP_SEQ_ELEM */
#include <boost/preprocessor/repetition/repeat.hpp>   /* BOOST_PP_REPEAT */

#include <eosio/chain/types.hpp>                      /* vector */

namespace bh = boost::hana;

namespace eosio { namespace chain {

/* 
 * currently OFFSETOF_IMPL is pointed to the offsetof builtin.
 * offsetof builtin has some limitation and if those will cause issues, 
 * check here for example compile-time implementation:
 * https://gist.github.com/graphitemaster/494f21190bb2c63c5516#gistcomment-2552129
 */
#define OFFSETOF_IMPL(C, M) offsetof(C, M)

#define CASE_PACK(r, CLASS, MEMBER)\
if (eosio::chain::field_id<&CLASS::MEMBER>() == entry.id){\
   fc::raw::pack(s, entry.config.MEMBER);\
   return s;\
}

#define RANGE_PACK_DERIVED(BASE, CLASS, MEMBERS)\
template<typename DataStream>\
inline DataStream& operator<<( DataStream& s, const eosio::chain::data_entry<CLASS, eosio::chain::config_entry_validator>& entry ) {\
   if (!entry.is_allowed()) return s;\
   BOOST_PP_SEQ_FOR_EACH(CASE_PACK, CLASS, MEMBERS)\
   if constexpr (std::is_base_of<BASE, CLASS>()) {\
      fc::raw::pack(s, eosio::chain::data_entry<BASE, eosio::chain::config_entry_validator>(entry));\
      return s;\
   }\
   FC_THROW_EXCEPTION(eosio::chain::config_parse_error, "DataStream& operator<<: no such id: ${id}", ("id", entry.id));\
}

#define RANGE_PACK(CLASS, MEMBERS)\
RANGE_PACK_DERIVED(std::false_type, CLASS, MEMBERS)

//TODO: add to CASE_UNPACK check for future protocol being active
#define CASE_UNPACK(r, CLASS, MEMBER)\
case eosio::chain::field_id<&CLASS::MEMBER>():\
   fc::raw::unpack(s, entry.config.MEMBER);\
   return s;

#define RANGE_UNPACK_DERIVED(BASE, CLASS, MEMBERS)\
template<typename DataStream>\
inline DataStream& operator>>( DataStream& s, eosio::chain::data_entry<CLASS, eosio::chain::config_entry_validator>& entry ) {\
   EOS_ASSERT(entry.is_allowed(), eosio::chain::unsupported_feature, "config id ${id} is no allowed", ("id", entry.id));\
   switch(entry.id){\
   BOOST_PP_SEQ_FOR_EACH(CASE_UNPACK, CLASS, MEMBERS)\
   }\
   if constexpr (std::is_base_of<BASE, CLASS>()) {\
      eosio::chain::data_entry<BASE, eosio::chain::config_entry_validator> base_entry(entry);\
      fc::raw::unpack(s, base_entry);\
      return s;\
   }\
   FC_THROW_EXCEPTION(eosio::chain::config_parse_error, "DataStream& operator<<: no such id: ${id}", ("id", entry.id));\
}

#define RANGE_UNPACK(CLASS, MEMBERS)\
RANGE_UNPACK_DERIVED(std::false_type, CLASS, MEMBERS)

#define ENUM_PAIR2(r, CLASS, ID, MEMBER, BASE)\
   bh::make_pair(bh::size_c<OFFSETOF_IMPL(CLASS, MEMBER)>,\
                 bh::size_c<enum_size<BASE>() + ID>),

#define ENUM_PAIR_DERIVED(r, BASE_DERIVED, ID, MEMBER)\
ENUM_PAIR2(r, BOOST_PP_SEQ_ELEM(1, BASE_DERIVED), ID, MEMBER, BOOST_PP_SEQ_ELEM(0, BASE_DERIVED))

#define DEFINE_FIELD_ID(r, CLASS, ID, MEMBER)\
template<>\
constexpr size_t field_id<&CLASS::MEMBER>(){\
   auto t = bh::size_c<OFFSETOF_IMPL(CLASS, MEMBER)>;\
   auto found = bh::find(BOOST_PP_CAT(CLASS, _MAP), t);\
   return *found;\
}

#define DEFINE_ENUM_DERIVED(BASE, DERIVED, MEMBERS)\
constexpr auto DERIVED##_MAP = bh::make_map(\
   BOOST_PP_SEQ_FOR_EACH_I(ENUM_PAIR_DERIVED, (BASE)(DERIVED), MEMBERS)\
   bh::make_pair(bh::size_c<sizeof(DERIVED)>, bh::size_c<BOOST_PP_SEQ_SIZE(MEMBERS)>)\
);\
BOOST_PP_SEQ_FOR_EACH_I(DEFINE_FIELD_ID, DERIVED, MEMBERS)\
template<>\
constexpr size_t enum_size<DERIVED>(){\
   auto t = bh::size_c<sizeof(DERIVED)>;\
   auto found = bh::find(DERIVED##_MAP, t);\
   return *found;\
}

#define DEFINE_ENUM(CLASS, MEMBERS)\
DEFINE_ENUM_DERIVED(void, CLASS, MEMBERS)

/**
 * Returns field in in the structure
 * This is enum-kind implementation.
 * See DEFINE_FIELD_ID macro for implementation
 */
template<auto T>
constexpr size_t field_id(){
   FC_THROW_EXCEPTION(config_parse_error, "only specializations of field_id can be used");
   return 0;
}

/**
 * Returns size of id/offset map
 * Map type is boost::hana's map
 * See DEFINE_ENUM_DERIVED macro for implementation
 */
template<typename T>
constexpr size_t enum_size(){
   FC_THROW_EXCEPTION(config_parse_error, "only specializations of enum_size can be used");
   return 0;
}
/**
 * this specialization is used for classed with no base class
 */
template<>
constexpr size_t enum_size<void>(){
   return 0;
}

/**
 * helper class to serialize only selected ids of the class
 */
template<typename T, typename Validator>
struct data_range {

   T& config;
   vector<fc::unsigned_int> ids;
   Validator validator;

   data_range(T& c, Validator val) : config(c), validator(val){}
   data_range(T& c, vector<fc::unsigned_int>&& id_list, const Validator& val) 
      : data_range(c, val){
      ids = std::move(id_list);
   }
};

/**
 * helper class to serialize specific class entry
 */
template<typename T, typename Validator>
struct data_entry {
private:
   struct _dummy{};
public:

   T& config;
   uint32_t id;
   Validator validator;
   data_entry(T& c, uint32_t entry_id, Validator validate)
    : config(c), 
      id(entry_id), 
      validator(validate) {}
   template <typename Y>
   explicit data_entry(const data_entry<Y, Validator>& another, 
              typename std::enable_if_t<std::is_base_of_v<T, Y>, _dummy> = _dummy{})
    : data_entry(another.config, another.id, another.validator)
   {}
   template <typename Y>
   explicit data_entry(const data_entry<Y, Validator>& another, 
              typename std::enable_if_t<!std::is_base_of_v<T, Y>, _dummy> = _dummy{})
    : config(std::forward<T&>(T{})) {
      FC_THROW_EXCEPTION(eosio::chain::config_parse_error, 
      "this constructor only for compilation of template magic and shouldn't ever be called");
   }

   bool is_allowed() const{
      return validator(id);
   }
};

}} // namespace eosio::chain