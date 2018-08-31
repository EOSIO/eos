#pragma once

#include <cppkafka/cppkafka.h>

#include <eosio/chain_plugin/chain_plugin.hpp>

#include "types.hpp"
#include "fifo.h"

namespace kafka {

using namespace std;
using namespace cppkafka;
using namespace eosio;

class kafka {
public:
    void set_config(Configuration config);
    void set_topics(const string& block_topic, const string& tx_topic, const string& tx_trace_topic, const string& action_topic);
    void start();
    void stop();

    void push_block(const chain::block_state_ptr& block_state, bool irreversible);
    std::pair<uint32_t, uint32_t> push_transaction(const chain::transaction_receipt& transaction_receipt, const BlockPtr& block, uint16_t block_seq);
    void push_transaction_trace(const chain::transaction_trace_ptr& transaction_trace);
    void push_action(const chain::action_trace& action_trace, uint64_t parent_seq, const TransactionTracePtr& tx);

private:
    void consume_blocks();
    void consume_transactions();
    void consume_transaction_traces();
    void consume_actions();

    /**
     * Why use fifo queue?
     * Though the Kafka client has local queue to buffer messages, we still use fifo queue to
     * deduplicate recent coming blocks or transactions that will be sent.
     */
    fifo<BlockPtr> block_queue_;
    fifo<TransactionPtr> transaction_queue_;
    fifo<TransactionTracePtr> transaction_trace_queue_;
    fifo<ActionPtr> action_queue_;

    std::thread consume_block_thread_;
    std::thread consume_transaction_thread_;
    std::thread consume_transaction_trace_thread_;
    std::thread consume_action_thread_;

    std::atomic<bool> stopped_{false};

    Configuration config_;
    string block_topic_;
    string tx_topic_;
    string tx_trace_topic_;
    string action_topic_;

    std::unique_ptr<Producer> producer_;
};

}
