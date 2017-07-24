#include <eos/native_contract/native_contract_chain_administrator.hpp>
#include <eos/native_contract/producer_objects.hpp>

#include <eos/chain/global_property_object.hpp>
#include <eos/chain/producer_object.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace eos { namespace native_contract {

using administrator = native_contract_chain_administrator;

ProducerRound administrator::get_next_round(chainbase::database& db) {
   return native::eos::ProducerScheduleObject::get(db).calculateNextRound(db);
}

chain::BlockchainConfiguration administrator::get_blockchain_configuration(const chainbase::database& db,
                                                                           const ProducerRound& round) {
   using boost::adaptors::transformed;
   using types::AccountName;
   using chain::producer_object;

   auto ProducerNameToConfiguration = transformed([&db](const AccountName& owner) {
      return db.get<producer_object, chain::by_owner>(owner).configuration;
   });

   auto votes_range = round | ProducerNameToConfiguration;

   return chain::BlockchainConfiguration::get_median_values({votes_range.begin(), votes_range.end()});
}

} } //namespace eos::native_contract

