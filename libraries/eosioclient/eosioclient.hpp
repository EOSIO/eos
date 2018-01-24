#ifndef EOSIOCLIENT_H
#define EOSIOCLIENT_H

#include "eosio/chain/transaction.hpp"
#include "eosio/chain_plugin/chain_plugin.hpp"
#include "wallet.hpp"
#include "peer.hpp"

namespace eosio {
namespace client {

class Eosioclient
{
public:
    Eosioclient(Peer& peer, Wallet& wallet);

    void create_account(chain::name creator,
                        chain::name newaccount,
                        chain::public_key_type owner,
                        chain::public_key_type active,
                        bool sign,
                        uint64_t staked_deposit);

    void create_producer(const std::string& account_name,
                         const std::string& owner_key,
                         std::vector<std::string> permissions,
                         bool skip_sign);
private:
    fc::variant push_transaction(eosio::chain::signed_transaction &trx, bool sign);
    void send_transaction(const std::vector<chain::action> &actions, bool skip_sign = false);
    void sign_transaction(eosio::chain::signed_transaction &trx);
    eosio::chain_apis::read_only::get_info_results get_info();
    std::vector<chain::permission_level> get_account_permissions(const std::vector<std::string> &permissions);

    Wallet& m_wallet;
    Peer& m_peer;
};

} // namespace client
} // namespace eosio

#endif // EOSIOCLIENT_H
