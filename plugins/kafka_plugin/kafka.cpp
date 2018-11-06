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

void kafka::set_partition(int partition) {
    partition_ =  partition;
}

void kafka::start() {
    producer_ = std::make_unique<Producer>(config_);

    auto conf = producer_->get_configuration().get_all();
    ilog("Kafka config: ${conf}", ("conf", conf));
}

void kafka::stop() {
    producer_->flush();

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

    consume_block(b);
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

    consume_transaction(t);

    return {t->action_count, t->context_free_action_count};
}

void kafka::push_transaction_trace(const chain::transaction_trace_ptr& tx_trace) {
    auto t = std::make_shared<TransactionTrace>();

    t->id = checksum_bytes(tx_trace->id);
    t->block_num = tx_trace->block_num;
    t->scheduled = tx_trace->scheduled;
    if (tx_trace->receipt) {
        t->status = transactionStatus(tx_trace->receipt->status);
        t->cpu_usage_us = tx_trace->receipt->cpu_usage_us;
        t->net_usage_words = tx_trace->receipt->net_usage_words;
    }
    if (tx_trace->except) {
        t->exception = tx_trace->except->to_string();
    }

    consume_transaction_trace(t);

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
    a->block_num = action_trace.block_num;
    a->tx_id = checksum_bytes(action_trace.trx_id);
    if (not action_trace.console.empty()) a->console = action_trace.console;

    consume_action(a);

    for (auto& inline_trace: action_trace.inline_traces) {
        push_action(inline_trace, action_trace.receipt.global_sequence, tx);
    }
}

void kafka::consume_block(BlockPtr block) {
    auto payload = fc::json::to_string(*block, fc::json::legacy_generator);
    Buffer buffer (block->id.data(), block->id.size());
    producer_->produce(MessageBuilder(block_topic_).partition(partition_).key(buffer).payload(payload));
}

void kafka::consume_transaction(TransactionPtr tx) {
    auto payload = fc::json::to_string(*tx, fc::json::legacy_generator);
    Buffer buffer (tx->id.data(), tx->id.size());
    producer_->produce(MessageBuilder(tx_topic_).partition(partition_).key(buffer).payload(payload));
}

void kafka::consume_transaction_trace(TransactionTracePtr tx_trace) {
    auto payload = fc::json::to_string(*tx_trace, fc::json::legacy_generator);
    Buffer buffer (tx_trace->id.data(), tx_trace->id.size());
    producer_->produce(MessageBuilder(tx_trace_topic_).partition(partition_).key(buffer).payload(payload));
}

void kafka::consume_action(ActionPtr action) {
    auto payload = fc::json::to_string(*action, fc::json::legacy_generator);
    Buffer buffer((char*)&action->global_seq, sizeof(action->global_seq));
    producer_->produce(MessageBuilder(action_topic_).partition(partition_).key(buffer).payload(payload));
}

}
