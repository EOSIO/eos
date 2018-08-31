#include "kafka.hpp"

#include <fc/io/json.hpp>

#include "try_handle.hpp"

namespace std {
template<> struct hash<kafka::bytes> {
    typedef kafka::bytes argument_type;
    typedef size_t result_type;
    result_type operator()(argument_type const& s) const noexcept {
        return std::hash<string>{}(string(s.begin(), s.end()));
    }
};
}

namespace kafka {

using chain::account_name;
using chain::action_name;
using chain::block_id_type;
using chain::permission_name;
using chain::transaction;
using chain::signed_transaction;
using chain::signed_block;
using chain::transaction_id_type;

namespace {

inline bytes checksum_bytes(const fc::sha256& s) { return bytes(s.data(), s.data() + sizeof(fc::sha256)); }

TransactionStatus transactionStatus(fc::enum_type<uint8_t, chain::transaction_receipt::status_enum> status) {
    if (status == chain::transaction_receipt::executed) return TransactionStatus::executed;
    else if (status == chain::transaction_receipt::soft_fail) return TransactionStatus::soft_fail;
    else if (status == chain::transaction_receipt::hard_fail) return TransactionStatus::hard_fail;
    else if (status == chain::transaction_receipt::delayed) return TransactionStatus::delayed;
    else if (status == chain::transaction_receipt::expired) return TransactionStatus::expired;
    else return TransactionStatus::unknown;
}

}

void kafka::set_config(Configuration config) {
    config_ = config;
}

void kafka::set_topics(const string& block_topic, const string& tx_topic, const string& tx_trace_topic, const string& action_topic) {
    block_topic_ = block_topic;
    tx_topic_ = tx_topic;
    tx_trace_topic_ = tx_trace_topic;
    action_topic_ = action_topic;
}

void kafka::start() {
    stopped_ = false;
    producer_ = std::make_unique<Producer>(config_);

    auto conf = producer_->get_configuration().get_all();
    ilog("Kafka config: ${conf}", ("conf", conf));

    consume_block_thread_ = std::thread([=] {
        loop_handle(stopped_, "consume blocks", [=] { consume_blocks(); });
    });

    consume_transaction_thread_ = std::thread([=] {
        loop_handle(stopped_, "consume transactions", [=] { consume_transactions(); });
    });

    consume_transaction_trace_thread_ = std::thread([=] {
        loop_handle(stopped_, "consume transaction traces", [=] { consume_transaction_traces(); });
    });

    consume_action_thread_ = std::thread([=] {
        loop_handle(stopped_, "consume actions", [=] { consume_actions(); });
    });
}

void kafka::stop() {
    stopped_ = true;

    block_queue_.awaken();
    transaction_queue_.awaken();
    transaction_trace_queue_.awaken();
    action_queue_.awaken();

    if (consume_block_thread_.joinable()) consume_block_thread_.join();
    if (consume_transaction_thread_.joinable()) consume_transaction_thread_.join();
    if (consume_transaction_trace_thread_.joinable()) consume_transaction_trace_thread_.join();
    if (consume_action_thread_.joinable()) consume_action_thread_.join();

    producer_.reset();
}

void kafka::push_block(const chain::block_state_ptr& block_state, bool irreversible) {
    const auto& header = block_state->header;
    auto b = std::make_shared<Block>();

    b->id = checksum_bytes(block_state->id);
    b->num = block_state->block_num;
    b->timestamp = header.timestamp;

    b->lib = irreversible;

    b->block = fc::raw::pack(*block_state->block);
    b->tx_count = static_cast<uint32_t>(block_state->block->transactions.size());

    uint16_t seq{};
    for (const auto& tx_receipt: block_state->block->transactions) {
        auto count = push_transaction(tx_receipt, b, seq++);
        b->action_count += count.first;
        b->context_free_action_count += count.second;
    }

    block_queue_.push(b);
}

std::pair<uint32_t, uint32_t> kafka::push_transaction(const chain::transaction_receipt& tx_receipt, const BlockPtr& block, uint16_t block_seq) {
    auto t = std::make_shared<Transaction>();
    if(tx_receipt.trx.contains<transaction_id_type>()) {
        t->id = checksum_bytes(tx_receipt.trx.get<transaction_id_type>());
    } else {
        auto signed_tx = tx_receipt.trx.get<chain::packed_transaction>().get_signed_transaction();
        t->id = checksum_bytes(signed_tx.id());
        t->action_count = static_cast<uint32_t>(signed_tx.actions.size());
        t->context_free_action_count = static_cast<uint32_t>(signed_tx.context_free_actions.size());
    }
    t->block_id = block->id;
    t->block_num = block->num;
    t->block_time = block->timestamp;
    t->block_seq = block_seq;
    transaction_queue_.push(t);
    return {t->action_count, t->context_free_action_count};
}

void kafka::push_transaction_trace(const chain::transaction_trace_ptr& tx_trace) {
    auto t = std::make_shared<TransactionTrace>();

    t->id = checksum_bytes(tx_trace->id);
    t->scheduled = tx_trace->scheduled;
    if (tx_trace->receipt) {
        t->status = transactionStatus(tx_trace->receipt->status);
        t->cpu_usage_us = tx_trace->receipt->cpu_usage_us;
        t->net_usage_words = tx_trace->receipt->net_usage_words;
    }
    if (tx_trace->except) {
        t->exception = tx_trace->except->to_string();
    }
    transaction_trace_queue_.push(t);

    for (auto& action_trace: tx_trace->action_traces) {
        push_action(action_trace, 0, t); // 0 means no parent
    }
}

void kafka::push_action(const chain::action_trace& action_trace, uint64_t parent_seq, const TransactionTracePtr& tx) {
    auto a = std::make_shared<Action>();

    a->global_seq = action_trace.receipt.global_sequence;
    a->recv_seq = action_trace.receipt.recv_sequence;
    a->parent_seq = parent_seq;
    a->account = action_trace.act.account;
    a->name = action_trace.act.name;
    if (not action_trace.act.authorization.empty()) a->auth = fc::raw::pack(action_trace.act.authorization);
    a->data = action_trace.act.data;
    a->receiver = action_trace.receipt.receiver;
    if (not action_trace.receipt.auth_sequence.empty()) a->auth_seq = fc::raw::pack(action_trace.receipt.auth_sequence);
    a->code_seq = action_trace.receipt.code_sequence;
    a->abi_seq = action_trace.receipt.abi_sequence;
    a->tx_id = checksum_bytes(action_trace.trx_id);
    if (not action_trace.console.empty()) a->console = action_trace.console;

    action_queue_.push(a);

    for (auto& inline_trace: action_trace.inline_traces) {
        push_action(inline_trace, action_trace.receipt.global_sequence, tx);
    }
}

void kafka::consume_blocks() {
    auto blocks = block_queue_.pop();
    if (blocks.empty()) return;

    // local deduplicate
    unordered_map<bytes, BlockPtr> distinct_blocks_by_id;
    map<unsigned, BlockPtr> distinct_blocks_by_num; // use std::map to ensure block num order
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        auto& block = *it;
        if (distinct_blocks_by_id.count(block->id)) continue; // irreversible confirmed
        if (distinct_blocks_by_num.count(block->num)) continue; // reversed, new override old
        distinct_blocks_by_id[block->id] = block;
        distinct_blocks_by_num[block->num] = block;
    }

    for (auto& p: distinct_blocks_by_num) {
        auto payload = fc::json::to_string(*p.second, fc::json::legacy_generator);
        Buffer buffer (p.second->id.data(), p.second->id.size());
        producer_->produce(MessageBuilder(block_topic_).key(buffer).payload(payload));
    }
}

void kafka::consume_transactions() {
    auto txs = transaction_queue_.pop();
    if (txs.empty()) return;

    unordered_map<bytes, TransactionPtr> distinct_txs;
    for (auto it = txs.rbegin(); it != txs.rend(); ++it) {
        auto& tx = *it;
        if (distinct_txs.count(tx->id)) continue;
        distinct_txs[tx->id] = tx;

        auto payload = fc::json::to_string(*tx, fc::json::legacy_generator);
        Buffer buffer (tx->id.data(), tx->id.size());
        producer_->produce(MessageBuilder(tx_topic_).key(buffer).payload(payload));
    }
}

void kafka::consume_transaction_traces() {
    auto txs = transaction_trace_queue_.pop();
    if (txs.empty()) return;

    unordered_map<bytes, TransactionTracePtr> distinct_txs;
    for (auto it = txs.rbegin(); it != txs.rend(); ++it) {
        auto& tx = *it;
        if (distinct_txs.count(tx->id)) continue;
        distinct_txs[tx->id] = tx;

        auto payload = fc::json::to_string(*tx, fc::json::legacy_generator);
        Buffer buffer (tx->id.data(), tx->id.size());
        producer_->produce(MessageBuilder(tx_trace_topic_).key(buffer).payload(payload));
    }
}

void kafka::consume_actions() {
    auto actions = action_queue_.pop();
    if (actions.empty()) return;

    unordered_map<uint64_t, ActionPtr> distinct_actions;
    for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
        auto& action = *it;
        if (distinct_actions.count(action->global_seq)) continue; // deduplicate
        distinct_actions[action->global_seq] = action;

        auto payload = fc::json::to_string(*action, fc::json::legacy_generator);
        Buffer buffer((char*)&action->global_seq, sizeof(action->global_seq));
        producer_->produce(MessageBuilder(action_topic_).key(buffer).payload(payload));
    }
}

}
