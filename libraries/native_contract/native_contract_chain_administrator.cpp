/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/native_contract/native_contract_chain_administrator.hpp>
#include <eos/chain/producer_objects.hpp>

#include <eos/chain/global_property_object.hpp>
#include <eos/chain/producer_object.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace eosio { namespace native_contract {

using administrator = native_contract_chain_administrator;

producer_round administrator::get_next_round(chainbase::database& db) {
   return eosio::chain::producer_schedule_object::get(db).calculate_next_round(db);
}

chain::blockchain_configuration administrator::get_blockchain_configuration(const chainbase::database& db,
                                                                           const producer_round& round) {
   using boost::adaptors::transformed;
   using types::account_name;
   using chain::producer_object;

   auto ProducerNameToConfiguration = transformed([&db](const account_name& owner) {
      return db.get<producer_object, chain::by_owner>(owner).configuration;
   });

   auto votes_range = round | ProducerNameToConfiguration;

   return chain::blockchain_configuration::get_median_values({votes_range.begin(), votes_range.end()});
}

} } //namespace eosio::native_contract

