#ifndef EOSIOCLIENT_H
#define EOSIOCLIENT_H

#include "eosio/chain/transaction.hpp"
#include "eosio/chain_plugin/chain_plugin.hpp"
#include "wallet.hpp"
#include "peer.hpp"

namespace eosio {
namespace client {

class eosioclient
{
public:
    eosioclient(peer& peer, wallet& wallet);

    void get_currency_stats(const std::string& code,
                            const std::string& symbol);

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

    void set_account_permission(const std::string& accountStr,
                                const std::string& permissionStr,
                                const std::string& authorityJsonOrFile,
                                const std::string& parentStr,
                                const std::string& permissionAuth,
                                bool skip_sign);

    void set_action_permission(const std::string& accountStr,
                               const std::string& codeStr,
                               const std::string& typeStr,
                               const std::string& requirementStr,
                               bool skip_sign);

    void transfer(const std::string& sender,
                  const std::string& recipient,
                  uint64_t amount,
                  std::string memo,
                  bool skip_sign,
                  bool tx_force_unique);

    std::vector<chain::permission_level> get_account_permissions(const std::vector<std::string>& permissions);

    eosio::chain_apis::read_only::get_info_results get_info();

    void get_code(const std::string& account,
                  const std::string& code_filename,
                  const std::string& abi_filename);

    void get_table(const std::string& scope, const std::string& code, const std::string& table);
    void execute_random_transactions(uint64_t number_account,
                                     uint64_t number_of_transfers,
                                     bool loop,
                                     const std::string &memo);

    void configure_benchmarks(uint64_t number_of_accounts,
                                          const std::string& c_account,
                                          const std::string& owner_key,
                                          const std::string& active_key);

    void push_transaction_with_single_action(const std::string& contract,
                                             const std::string& action,
                                             const std::string& data,
                                             const std::vector<std::string>& permissions,
                                             bool skip_sign,
                                             bool tx_force_unique);

    void set_proxy_account_for_voting(const std::string& account_name,
                                      std::string proxy,
                                      std::vector<std::string> permissions,
                                      bool skip_sign);

    void approve_unapprove_producer(const std::string& account_name,
                                    const std::string& producer,
                                    std::vector<std::string> permissions,
                                    bool approve,
                                    bool skip_sign);

    void get_balance(const std::string& account_name,
                     const std::string& code,
                     const std::string& symbol);

    fc::variant push_transaction(eosio::chain::signed_transaction& trx, bool sign); ///< \todo make private
    std::vector<uint8_t> assemble_wast(const std::string &wast); ///< \todo make private
private:
    void send_transaction(const std::vector<chain::action>& actions, bool skip_sign = false);
    void sign_transaction(eosio::chain::signed_transaction& trx);
    chain::action create_deleteauth(const chain::name& account, const chain::name& permission, const chain::name& permissionAuth);
    chain::action create_updateauth(const chain::name& account, const chain::name& permission, const chain::name& parent, const chain::authority& auth, const chain::name& permissionAuth);
    chain::action create_unlinkauth(const chain::name& account, const chain::name& code, const chain::name& type);
    chain::action create_linkauth(const chain::name& account, const chain::name& code, const chain::name& type, const chain::name& requirement);
    uint64_t generate_nonce_value();
    chain::action generate_nonce();

    wallet& m_wallet;
    peer& m_peer;
};

} // namespace client
} // namespace eosio

#endif // EOSIOCLIENT_H
