#include <eos/native_contract/native_contract_chain_administrator.hpp>
#include <eos/native_contract/staked_balance_objects.hpp>

#include <eos/chain/global_property_object.hpp>
#include <eos/chain/producer_object.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace eos { namespace native_contract {

using administrator = native_contract_chain_administrator;

administrator::ProducerRound administrator::get_next_round(const chainbase::database& db) {

}

chain::BlockchainConfiguration administrator::get_blockchain_configuration(const chainbase::database& db) {
   using boost::adaptors::transformed;
   using types::AccountName;
   using chain::producer_object;

   auto get_producer_votes = transformed([&db](const AccountName& owner) {
      return db.get<producer_object, chain::by_owner>(owner).configuration;
   });

   const auto& gpo = db.get(chain::global_property_object::id_type());
   auto votes_range = gpo.active_producers | get_producer_votes;

   return chain::BlockchainConfiguration::get_median_values({votes_range.begin(), votes_range.end()});
}

} } //namespace eos::native_contract

