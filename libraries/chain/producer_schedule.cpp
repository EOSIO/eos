#include <eosio/chain/producer_schedule.hpp>

namespace eosio { namespace chain {

fc::variant producer_authority::get_abi_variant() const {
      auto authority_variant = authority.visit([](const auto& a){
         fc::variant value;
         fc::to_variant(a, value);

         std::string full_type_name = fc::get_typename<std::decay_t<decltype(a)>>::name();
         auto last_colon = full_type_name.rfind(":");
         std::string type_name = last_colon == std::string::npos ? std::move(full_type_name): full_type_name.substr(last_colon + 1);

         return fc::variants {
               fc::variant(std::move(type_name)),
               std::move(value)
         };
      });

      return fc::mutable_variant_object()
            ("producer_name", producer_name)
            ("authority", std::move(authority_variant));
}

} } /// eosio::chain