#pragma once

#include <eosio/chain/types.hpp>                      /* vector */

namespace eosio { namespace chain {

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