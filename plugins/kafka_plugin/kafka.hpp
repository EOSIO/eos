#pragma once

#include <cppkafka/cppkafka.h>

#include <eosio/chain_plugin/chain_plugin.hpp>

#include "types.hpp"

namespace kafka {

using namespace std;
using namespace cppkafka;
using namespace eosio;

class kafka {
public:
    void set_config(Configuration config);
    void set_topics(const string& block_topic, const string& tx_topic, const string& tx_trace_topic, const string& action_topic);
    void set_partition(int partition);
    void start();
    void stop();

    void push_block(const chain::block_state_ptr& block_state, bool irreversible);
    std::pair<uint32_t, uint32_t> push_transaction(const chain::transaction_receipt& transaction_receipt, const BlockPtr& block, uint16_t block_seq);
    void push_transaction_trace(const chain::transaction_trace_ptr& transaction_trace);
    void push_action(const chain::action_trace& action_trace, uint64_t parent_seq, const TransactionTracePtr& tx);

private:
    void consume_block(BlockPtr block);
    void consume_transaction(TransactionPtr tx);
    void consume_transaction_trace(TransactionTracePtr tx_trace);
    void consume_action(ActionPtr action);

    Configuration config_;
    string block_topic_;
    string tx_topic_;
    string tx_trace_topic_;
    string action_topic_;

    int partition_{-1};

    std::unique_ptr<Producer> producer_;
};

}
