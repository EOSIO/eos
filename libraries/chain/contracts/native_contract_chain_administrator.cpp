/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chaincontracts/native_contract_chain_administrator.hpp>
#include <eosio/chaincontracts/producer_objects.hpp>
#include <eosio/chaincontracts/types.hpp>

#include <eosio/chainglobal_property_object.hpp>
#include <eosio/chainproducer_object.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace eosio { namespace chain { namespace contracts {


using administrator = native_contract_chain_administrator;

producer_round administrator::get_next_round(chainbase::database& db) {
   return producer_schedule_object::get(db).calculate_next_round(db);
}

blockchain_configuration administrator::get_blockchain_configuration(const chainbase::database& db,
                                                                           const producer_round& round) {
   using boost::adaptors::transformed;
   using account_name;
   using producer_object;

   auto producer_name_to_configuration = transformed([&db](const account_name& owner) {
      return db.get<producer_object, by_owner>(owner).configuration;
   });

   auto votes_range = round | producer_name_to_configuration;

   return blockchain_configuration::get_median_values({votes_range.begin(), votes_range.end()});
}

} } } // namespace eosio::contracts

