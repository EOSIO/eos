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

    void set_account_permission(const std::string &accountStr,
                                const std::string &permissionStr,
                                const std::string &authorityJsonOrFile,
                                const std::string &parentStr,
                                const std::string &permissionAuth,
                                bool skip_sign);

    void set_action_permission(const std::string &accountStr,
                               const std::string &codeStr,
                               const std::string &typeStr,
                               const std::string &requirementStr,
                               bool skip_sign);
private:
    fc::variant push_transaction(eosio::chain::signed_transaction &trx, bool sign);
    void send_transaction(const std::vector<chain::action> &actions, bool skip_sign = false);
    void sign_transaction(eosio::chain::signed_transaction &trx);
    eosio::chain_apis::read_only::get_info_results get_info();
    std::vector<chain::permission_level> get_account_permissions(const std::vector<std::string> &permissions);
    chain::action create_deleteauth(const chain::name &account, const chain::name &permission, const chain::name &permissionAuth);
    chain::action create_updateauth(const chain::name &account, const chain::name &permission, const chain::name &parent, const chain::authority &auth, const chain::name &permissionAuth);
    chain::action create_unlinkauth(const chain::name &account, const chain::name &code, const chain::name &type);
    chain::action create_linkauth(const chain::name &account, const chain::name &code, const chain::name &type, const chain::name &requirement);

    Wallet& m_wallet;
    Peer& m_peer;


};

} // namespace client
} // namespace eosio

#endif // EOSIOCLIENT_H
