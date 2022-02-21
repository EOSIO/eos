#pragma once

#include <b1/rodeos/callbacks/kv.hpp>
#include <b1/rodeos/rodeos_tables.hpp>

namespace b1::rodeos {

template <typename T, typename K>
std::optional<std::pair<std::shared_ptr<const chain_kv::bytes>, T>> get_state_row(chain_kv::view& view, const K& key) {
   std::optional<std::pair<std::shared_ptr<const chain_kv::bytes>, T>> result;
   result.emplace();
   result->first =
         view.get(state_account.value,
                  chain_kv::to_slice(eosio::convert_to_key(std::make_tuple((uint8_t)0x01, key))));
   if (!result->first) {
      result.reset();
      return result;
   }

   eosio::input_stream stream{ *result->first };
   try {
      from_bin(result->second, stream);
   } catch(std::exception& e) {
      throw std::runtime_error("An error occurred deserializing state: " + std::string(e.what()));
   }
   return result;
}

template <typename T, typename K>
std::optional<std::pair<std::shared_ptr<const chain_kv::bytes>, T>> get_state_row_secondary(chain_kv::view& view,
                                                                                            const K&        key) {
   std::optional<std::pair<std::shared_ptr<const chain_kv::bytes>, T>> result;
   auto                                                                pk =
         view.get(state_account.value,
                  chain_kv::to_slice(eosio::convert_to_key(std::make_tuple((uint8_t)0x01, key))));
   if (!pk)
      return result;

   result.emplace();
   result->first = view.get(state_account.value, chain_kv::to_slice(*pk));
   if (!result->first) {
      result.reset();
      return result;
   }

   eosio::input_stream stream{ *result->first };
   try {
      from_bin(result->second, stream);
   } catch(std::exception& e) {
      throw std::runtime_error("An error occurred deserializing state: " + std::string(e.what()));
   }
   return result;
}

} // namespace b1::rodeos
