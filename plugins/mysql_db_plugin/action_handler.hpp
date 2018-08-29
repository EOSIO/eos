#pragma once

#include <eosio/chain_plugin/chain_plugin.hpp>

#include <odb/database.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/transaction.hxx>
#include <odb/mysql/database.hxx>

#include "odb/eos.hpp"
#include "odb/eos-odb.hxx"
#include "fifo.h"

namespace eosio {

class action_handler_base {
public:
    action_handler_base(std::string name, std::shared_ptr<odb::database> db);
    ~action_handler_base();
    virtual void start() = 0;
    void stop();
    virtual void handle(const chain::action& action, uint64_t receiver, uint64_t seq, const TransactionTracePtr& tx) = 0;

protected:
    virtual void awaken();

    std::string name_;
    std::shared_ptr<odb::database> db_;
    std::atomic<bool> done_{false};
    std::thread thread_;
};

using action_handler_ptr = std::shared_ptr<action_handler_base>;

class token_transfer_handler : public action_handler_base {
public:
    token_transfer_handler(const std::vector<std::string>& contracts, std::shared_ptr<odb::database> db);
    void start() override;
    void awaken() override;
    void handle(const chain::action& action, uint64_t receiver, uint64_t seq, const TransactionTracePtr& tx) override;

private:
    void consume_token_transfers();

    std::unordered_set<chain::account_name> contract_names_{"eosio.token"};
    chain::action_name action_name_{"transfer"};
    fifo<TokenTransferPtr> token_transfer_queue_;
};

}
