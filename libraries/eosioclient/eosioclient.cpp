#include "eosioclient.hpp"

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "fc/io/json.hpp"
#include "localize.hpp"

FC_DECLARE_EXCEPTION( explained_exception, 9000000, "explained exception, see error log" );
FC_DECLARE_EXCEPTION( localized_exception, 10000000, "an error occured" );
#define EOSC_ASSERT( TEST, ... ) \
  FC_EXPAND_MACRO( \
    FC_MULTILINE_MACRO_BEGIN \
      if( UNLIKELY(!(TEST)) ) \
      {                                                   \
        std::cerr << localized( __VA_ARGS__ ) << std::endl;  \
        FC_THROW_EXCEPTION( explained_exception, #TEST ); \
      }                                                   \
    FC_MULTILINE_MACRO_END \
)

namespace eosio {
namespace client {

const auto tx_expiration = fc::seconds(30);

Eosioclient::Eosioclient(Peer &peer, Wallet &wallet):
    m_peer(peer),
    m_wallet(wallet)
{

}

void Eosioclient::create_account(chain::name creator,
                                 chain::name newaccount,
                                 chain::public_key_type owner,
                                 chain::public_key_type active,
                                 bool sign,
                                 uint64_t staked_deposit)
{
      auto owner_auth   = eosio::chain::authority{1, {{owner, 1}}, {}};
      auto active_auth  = eosio::chain::authority{1, {{active, 1}}, {}};
      auto recovery_auth = eosio::chain::authority{1, {}, {{{creator, "active"}, 1}}};

      uint64_t deposit = staked_deposit;

      chain::signed_transaction trx;
      trx.actions.emplace_back( vector<chain::permission_level>{{creator,"active"}},
                                chain::contracts::newaccount{creator, newaccount, owner_auth, active_auth, recovery_auth, deposit});

      std::cout << fc::json::to_pretty_string(push_transaction(trx, sign)) << std::endl;
}

void Eosioclient::create_producer(const std::string &account_name, const std::string &owner_key, std::vector<std::string> permissions, bool skip_sign)
{
    if (permissions.empty()) {
       permissions.push_back(account_name + "@active");
    }
    auto account_permissions = get_account_permissions(permissions);

    chain::signed_transaction trx;
    trx.actions.emplace_back(account_permissions,
                             chain::contracts::setproducer{account_name,
                                                           chain::public_key_type(owner_key),
                                                           chain::chain_config{}} );

    std::cout << fc::json::to_pretty_string(push_transaction(trx, !skip_sign)) << std::endl;
}

void Eosioclient::send_transaction(const std::vector<chain::action>& actions, bool skip_sign) {
   eosio::chain::signed_transaction trx;
   for (const auto& m: actions) {
      trx.actions.emplace_back( m );
   }

   auto info = get_info();
   trx.expiration = info.head_block_time + tx_expiration;
   trx.set_reference_block(info.head_block_id);
   if (!skip_sign) {
      sign_transaction(trx);
   }

   std::cout << fc::json::to_pretty_string(m_peer.push_transaction( trx )) << std::endl;
}

fc::variant Eosioclient::push_transaction(eosio::chain::signed_transaction& trx, bool sign )
{
    auto info = get_info();
    trx.expiration = info.head_block_time + tx_expiration;
    trx.set_reference_block(info.head_block_id);

    if (sign) {
       sign_transaction(trx);
    }

    return m_peer.push_transaction( trx );
}

void Eosioclient::sign_transaction(eosio::chain::signed_transaction& trx)
{
   // TODO better error checking
   const auto& public_keys = m_wallet.public_keys();
   const auto& required_keys = m_peer.get_keys_required(trx, public_keys);
   // TODO determine chain id
   const auto& signed_trx = m_wallet.sign_transaction(trx, required_keys, eosio::chain::chain_id_type{});
   trx = signed_trx.as<eosio::chain::signed_transaction>();
}

eosio::chain_apis::read_only::get_info_results Eosioclient::get_info() {
  return m_peer.get_info().as<eosio::chain_apis::read_only::get_info_results>();
}

std::vector<chain::permission_level> Eosioclient::get_account_permissions(const std::vector<std::string>& permissions)
{
   auto fixedPermissions = permissions | boost::adaptors::transformed([](const std::string& p) {
      std::vector<std::string> pieces;
      split(pieces, p, boost::algorithm::is_any_of("@"));
      EOSC_ASSERT(pieces.size() == 2, "Invalid permission: ${p}", ("p", p));
      return chain::permission_level{ .actor = pieces[0], .permission = pieces[1] };
   });
   std::vector<chain::permission_level> accountPermissions;
   boost::range::copy(fixedPermissions, back_inserter(accountPermissions));
   return accountPermissions;
}

} // namespace client
} // namespace eosio
