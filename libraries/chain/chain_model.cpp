#include <eos/chain/chain_model.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/producer_object.hpp>

namespace eos { namespace chain {

const account_object& chain_model::get_account(const AccountName& name) const {
   return db.get<account_object, by_name>(name);
}

const producer_object& chain_model::get_producer(const AccountName& name) const {
   return db.get<producer_object, by_owner>(get_account(name).id);
}

} } // namespace eos::chain
