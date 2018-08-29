/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/mysql_db_plugin/mysql_db_plugin.hpp>

#include <boost/date_time/c_local_time_adjustor.hpp>

#include <fc/io/raw.hpp>

#include <odb/database.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/transaction.hxx>
#include <odb/mysql/database.hxx>

#include "odb/eos.hpp"
#include "odb/eos-odb.hxx"
#include "fifo.h"
#include "try_handle.hpp"
#include "action_handler.hpp"

namespace std {
template<> struct hash<eosio::bytes> {
    typedef eosio::bytes argument_type;
    typedef size_t result_type;
    result_type operator()(argument_type const& s) const noexcept {
        return std::hash<string>{}(string(s.begin(), s.end()));
    }
};
}

namespace eosio {

using namespace std;
using namespace odb::core;

using chain::account_name;
using chain::action_name;
using chain::block_id_type;
using chain::permission_name;
using chain::transaction;
using chain::signed_transaction;
using chain::signed_block;
using chain::transaction_id_type;

using local_adj = boost::date_time::c_local_adjustor<boost::posix_time::ptime>;

static appbase::abstract_plugin& _mysql_db_plugin = app().register_plugin<mysql_db_plugin>();

class mysql_db_plugin_impl {
public:
    void consume_blocks();
    void consume_transactions();
    void consume_transaction_traces();
    void consume_actions();

    void push_block(const chain::block_state_ptr& block_state);
    std::pair<uint32_t, uint32_t> push_transaction(const chain::transaction_receipt& transaction_receipt, const BlockPtr& block, const uint16_t block_seq);
    void push_transaction_trace(const chain::transaction_trace_ptr& transaction_trace);
    void push_action(const chain::action_trace& action_trace, uint64_t parent_seq, const TransactionTracePtr& tx);

    void init();
    void wipe_database();
    void stop();

    bool configured_{false};
    bool wipe_database_on_startup_{false};

    chain_plugin* chain_plugin_{nullptr};
    shared_ptr<odb::database> db_;

    std::atomic<bool> done_{false};

    std::atomic<bool> start_sync_{false};

    StatsPtr stats_;
    fifo<BlockPtr> block_queue_;
    fifo<TransactionPtr> transaction_queue_;
    fifo<TransactionTracePtr> transaction_trace_queue_;
    fifo<ActionPtr> action_queue_;

    boost::signals2::connection block_conn_;
    boost::signals2::connection irreversible_block_conn_;
    boost::signals2::connection transaction_conn_;

    std::thread consume_block_thread_;
    std::thread consume_transaction_thread_;
    std::thread consume_transaction_trace_thread_;
    std::thread consume_action_thread_;

    std::vector<action_handler_ptr> action_handlers_;
};

void mysql_db_plugin_impl::stop() {
    try {
        done_ = true;

        block_conn_.disconnect();
        irreversible_block_conn_.disconnect();
        transaction_conn_.disconnect();

        block_queue_.awaken();
        transaction_queue_.awaken();
        transaction_trace_queue_.awaken();
        action_queue_.awaken();

        if (consume_block_thread_.joinable()) consume_block_thread_.join();
        if (consume_transaction_thread_.joinable()) consume_transaction_thread_.join();
        if (consume_transaction_trace_thread_.joinable()) consume_transaction_trace_thread_.join();
        if (consume_action_thread_.joinable()) consume_action_thread_.join();

        if (db_) db_.reset();

        for (auto& action_handler: action_handlers_) {
            action_handler->stop();
        }
    } catch (const std::exception& e) {
        elog("Exception on my_db_plugin shutdown: ${e}", ("e", e.what()));
    }
}

namespace {

inline bytes checksum_bytes(const fc::sha256& s) { return bytes(s.data(), s.data() + sizeof(fc::sha256)); }

boost::posix_time::ptime now() {
    return boost::posix_time::microsec_clock::local_time(); // adapt to MySQL Local TimeZone setting
}

boost::posix_time::ptime ptime_from_time_point(const fc::time_point& tp) {
    auto seconds = tp.sec_since_epoch();
    auto microseconds = tp.time_since_epoch().count() - static_cast<int64_t>(seconds) * 1e6;
    auto ptime = boost::posix_time::from_time_t(time_t(seconds));
    ptime += boost::posix_time::microseconds(static_cast<long>(microseconds));
    return local_adj::utc_to_local(ptime); // adapt to MySQL Local TimeZone setting
}

boost::posix_time::ptime ptime_from_block_timestamp(const chain::block_timestamp_type& bt) {
    auto tp = bt.to_time_point();
    return ptime_from_time_point(tp);
}

std::string timestampStr(const fc::time_point& tp) {
    return std::string(fc::string(tp));
}

fc::time_point time_point(int64_t microseconds) {
    return fc::time_point(fc::microseconds(microseconds));
}

TransactionStatus transactionStatus(fc::enum_type<uint8_t, chain::transaction_receipt::status_enum> status) {
    if (status == chain::transaction_receipt::executed) return TransactionStatus::executed;
    else if (status == chain::transaction_receipt::soft_fail) return TransactionStatus::soft_fail;
    else if (status == chain::transaction_receipt::hard_fail) return TransactionStatus::hard_fail;
    else if (status == chain::transaction_receipt::delayed) return TransactionStatus::delayed;
    else if (status == chain::transaction_receipt::expired) return TransactionStatus::expired;
    else return TransactionStatus::unknown;
}

}

void mysql_db_plugin_impl::push_block(const chain::block_state_ptr& block_state) {
    const auto& header = block_state->header;
    auto b = std::make_shared<Block>();

    b->id_ = checksum_bytes(block_state->id);
    b->num_ = block_state->block_num;
    b->timestamp_ = ptime_from_block_timestamp(header.timestamp);
    // b->producer_ = uint64_t(header.producer);
    // b->confirmed_ = header.confirmed;
    // b->prev_id_ =  checksum_bytes(header.previous);
    // b->tx_mroot_ = checksum_bytes(header.transaction_mroot);
    // b->action_mroot_ = checksum_bytes(header.action_mroot);
    // b->schedule_version_ = header.schedule_version;
    // if (header.new_producers.valid()) b->new_producers_ = fc::raw::pack(header.new_producers);
    // if (header.header_extensions.size()) b->header_extensions_ = fc::raw::pack(header.header_extensions);
    // b->producer_signature = string(header.producer_signature);
    // if (block_state->block->block_extensions.size()) b->block_extensions_ = fc::raw::pack(block_state->block->block_extensions);
    b->block_ = fc::raw::pack(*block_state->block);
    b->tx_count_ = static_cast<uint32_t>(block_state->block->transactions.size());
    b->created_at_ = now();

    uint16_t seq{};
    for (const auto& tx_receipt: block_state->block->transactions) {
        auto count = push_transaction(tx_receipt, b, seq++);
        b->action_count_ += count.first;
        b->context_free_action_count_ += count.second;
    }

    block_queue_.push(b);
}

std::pair<uint32_t, uint32_t> mysql_db_plugin_impl::push_transaction(const chain::transaction_receipt& tx_receipt, const BlockPtr& block, const uint16_t block_seq) {
    auto t = std::make_shared<Transaction>();
    if(tx_receipt.trx.contains<transaction_id_type>()) {
        t->id_ = checksum_bytes(tx_receipt.trx.get<transaction_id_type>());
    } else {
        auto signed_tx = tx_receipt.trx.get<chain::packed_transaction>().get_signed_transaction();
        t->id_ = checksum_bytes(signed_tx.id());
        // t->transaction_header_ = fc::raw::pack(static_cast<chain::transaction_header>(signed_tx));
        // if (signed_tx.transaction_extensions.size()) t->transaction_extensions_ = fc::raw::pack(signed_tx.transaction_extensions);
        // if (signed_tx.signatures.size()) t->signatures_ = fc::raw::pack(signed_tx.signatures);
        // if (signed_tx.context_free_data.size()) t->context_free_data_ = fc::raw::pack(signed_tx.context_free_data);
        t->action_count_ = static_cast<uint32_t>(signed_tx.actions.size());
        t->context_free_action_count_ = static_cast<uint32_t>(signed_tx.context_free_actions.size());
    }
    t->block_id_ = block->id_;
    t->block_num_ = block->num_;
    t->block_time_ = block->timestamp_;
    t->block_seq_ = block_seq;
    transaction_queue_.push(t);
    return {t->action_count_, t->context_free_action_count_};
}

void mysql_db_plugin_impl::push_transaction_trace(const chain::transaction_trace_ptr& tx_trace) {
    auto t = std::make_shared<TransactionTrace>();

    t->id_ = checksum_bytes(tx_trace->id);
    // t->elapsed_us_ = tx_trace->elapsed.count();
    t->scheduled_ = tx_trace->scheduled;
    if (tx_trace->receipt) {
        t->status_ = transactionStatus(tx_trace->receipt->status);
        t->cpu_usage_us_ = tx_trace->receipt->cpu_usage_us;
        t->net_usage_words_ = tx_trace->receipt->net_usage_words;
    }
    if (tx_trace->except) {
        t->exception_ = tx_trace->except->to_string();
    }
    transaction_trace_queue_.push(t);

    for (auto& action_trace: tx_trace->action_traces) {
        push_action(action_trace, 0, t); // 0 means no parent
    }
}

void mysql_db_plugin_impl::push_action(const chain::action_trace& action_trace, uint64_t parent_seq, const TransactionTracePtr& tx) {
    auto a = std::make_shared<Action>();

    a->global_seq_ = action_trace.receipt.global_sequence;
    a->account_seq_ = action_trace.receipt.recv_sequence;
    a->parent_seq_ = parent_seq;
    a->account_ = action_trace.act.account;
    a->name_ = action_trace.act.name;
    if (not action_trace.act.authorization.empty()) a->auth_ = fc::raw::pack(action_trace.act.authorization);
    a->data_ = action_trace.act.data;
    a->receiver_ = action_trace.receipt.receiver;
    if (not action_trace.receipt.auth_sequence.empty()) a->auth_seq_ = fc::raw::pack(action_trace.receipt.auth_sequence);
    a->code_seq_ = action_trace.receipt.code_sequence;
    a->abi_seq_ = action_trace.receipt.abi_sequence;
    // a->digest_ = checksum_bytes(action_trace.receipt.act_digest);
    a->tx_id_ = checksum_bytes(action_trace.trx_id);
    // a->elapsed_us_ = action_trace.elapsed.count();
    // a->cpu_usage_ = action_trace.cpu_usage;
    if (not action_trace.console.empty()) a->console_ = action_trace.console;
    // a->total_cpu_usage_ = action_trace.total_cpu_usage;

    action_queue_.push(a);

    for (auto& action_handler: action_handlers_) {
        action_handler->handle(action_trace.act, a->receiver_, a->global_seq_, tx);
    }

    for (auto& inline_trace: action_trace.inline_traces) {
        push_action(inline_trace, action_trace.receipt.global_sequence, tx);
    }
}

void mysql_db_plugin_impl::consume_blocks() {
    using query = odb::query<Block>;

    auto blocks = block_queue_.pop();
    if (blocks.empty()) return;

    unordered_map<bytes, BlockPtr> distinct_blocks_by_id;
    unordered_map<unsigned, BlockPtr> distinct_blocks_by_num;
    vector<bytes> ids;
    vector<unsigned> nums;
    ids.reserve(blocks.size());
    nums.reserve(blocks.size());
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        auto& block = *it;
        if (distinct_blocks_by_id.count(block->id_)) continue; // irreversible confirmed
        if (distinct_blocks_by_num.count(block->num_)) continue; // reversed, new override old
        distinct_blocks_by_id[block->id_] = block;
        distinct_blocks_by_num[block->num_] = block;
        ids.push_back(block->id_);
        nums.push_back(block->num_);
    }

    odb::transaction t(db_->begin());

    auto q = query::num.in_range(nums.begin(), nums.end());
    auto result = db_->query<Block>(q);
    for (auto it = result.begin(); it != result.end(); ++it) {
        if (not distinct_blocks_by_id.count(it.id())) { // reversed
            stats_->tx_count_ -= it->tx_count_;
            stats_->action_count_ -= it->action_count_;
            stats_->context_free_action_count_ -= it->context_free_action_count_;
            db_->erase(*it);
        }
    }

    q = query::id.in_range(ids.begin(), ids.end());
    result = db_->query<Block>(q);
    unordered_set<bytes> existing_blocks_by_id;
    for (auto it = result.begin(); it != result.end(); ++it) {
        existing_blocks_by_id.insert(it.id());
    }

    for (auto& p: distinct_blocks_by_id) {
        auto& b = p.second;
        if (existing_blocks_by_id.count(b->id_)) {
            // block can be uniquely distinguish by its id, so needs no update
            // db_->update(*b);
        } else {
            stats_->tx_count_ += b->tx_count_;
            stats_->action_count_ += b->action_count_;
            stats_->context_free_action_count_ += b->context_free_action_count_;
            db_->persist(*b);
        }
    }

    db_->update(*stats_);

    t.commit();
}

void mysql_db_plugin_impl::consume_transactions() {
    using tx_query = odb::query<Transaction>;

    auto txs = transaction_queue_.pop();
    if (txs.empty()) return;

    unordered_map<bytes, TransactionPtr> distinct_txs;
    vector<bytes> ids;
    ids.reserve(txs.size());
    for (auto it = txs.rbegin(); it != txs.rend(); ++it) {
        auto& tx = *it;
        if (distinct_txs.count(tx->id_)) continue;
        distinct_txs[tx->id_] = tx;
        ids.push_back(tx->id_);
    }

    odb::transaction t(db_->begin());

    tx_query q(tx_query::id.in_range(ids.begin(), ids.end()));
    auto result = db_->query<Transaction>(q);
    unordered_map<bytes, TransactionPtr> map;
    for (auto it = result.begin(); it != result.end(); ++it) {
        map[it.id()] = it.load();
    }
    for (auto& p: distinct_txs) {
        auto& tx = p.second;
        if (map.count(tx->id_)) {
            auto old = map.at(tx->id_);
            // transaction can be uniquely distinguish by its id, so only update it when its block has changed
            if (tx->block_id_ != old->block_id_) db_->update(*tx);
        }
        else db_->persist(*tx);
    }

    t.commit();
}

void mysql_db_plugin_impl::consume_transaction_traces() {
    using tx_query = odb::query<TransactionTrace>;

    auto txs = transaction_trace_queue_.pop();
    if (txs.empty()) return;

    unordered_map<bytes, TransactionTracePtr> distinct_txs;
    vector<bytes> ids;
    ids.reserve(txs.size());
    for (auto it = txs.rbegin(); it != txs.rend(); ++it) {
        auto& tx = *it;
        if (distinct_txs.count(tx->id_)) continue;
        distinct_txs[tx->id_] = tx;
        ids.push_back(tx->id_);
    }

    odb::transaction t(db_->begin());

    tx_query q(tx_query::id.in_range(ids.begin(), ids.end()));
    auto result = db_->query<TransactionTrace>(q);
    unordered_set<bytes> set;
    for (auto it = result.begin(); it != result.end(); ++it) {
        set.insert(it.id());
    }
    for (auto& p: distinct_txs) {
        auto& tx = p.second;
        if (set.count(tx->id_)) db_->update(*tx);
        else db_->persist(*tx);
    }

    t.commit();
}

void mysql_db_plugin_impl::consume_actions() {
    using action_query = odb::query<Action>;
    using action_prepared_query = odb::prepared_query<Action>;
    using action_result = odb::result<Action>;

    auto actions = action_queue_.pop();
    if (actions.empty()) return;

    unordered_map<uint64_t, ActionPtr> distinct_actions;
    vector<uint64_t> seqs;
    seqs.reserve(actions.size());
    for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
        auto& action = *it;
        if (distinct_actions.count(action->global_seq_)) continue; // deduplicate
        distinct_actions[action->global_seq_] = action;
        seqs.push_back(action->global_seq_);
    }

    odb::transaction t(db_->begin());

    action_query q(action_query::global_seq.in_range(seqs.begin(), seqs.end()));
    auto result = db_->query<Action>(q);
    unordered_set<uint64_t> set;
    for (auto it = result.begin(); it != result.end(); ++it) {
        set.insert(it.id());
    }
    for (auto& p: distinct_actions) {
        auto& action = p.second;
        if (set.count(action->global_seq_)) db_->update(*action);
        else db_->persist(*action);
    }

    t.commit();
}

void mysql_db_plugin_impl::init() {
    odb::transaction t (db_->begin ());
    if (db_->execute("SHOW TABLES LIKE 'Block'") == 0) {
        ilog("mysql db init, create tables");
        odb::schema_catalog::create_schema(*db_);
    }

    stats_ = db_->query_one<Stats>(odb::query<Stats>::id == 0);
    if (not stats_) {
        stats_ = make_shared<Stats>();
        db_->persist(*stats_);
    }

    t.commit();
}

void mysql_db_plugin_impl::wipe_database() {
    odb::transaction t (db_->begin ());
    if (odb::schema_catalog::exists(*db_)) {
        ilog("mysql db wipe_database, drop tables");
        odb::schema_catalog::drop_schema(*db_);
    }
    t.commit();
}

mysql_db_plugin::mysql_db_plugin() : my(new mysql_db_plugin_impl()) {}

mysql_db_plugin::~mysql_db_plugin() {}

void mysql_db_plugin::set_program_options(options_description&, options_description& cfg) {
    cfg.add_options()
            ("mysql-enable", bpo::value<bool>(), "MySQL enable")
            ("mysql-host", bpo::value<string>()->default_value("127.0.0.1"), "MySQL host")
            ("mysql-port", bpo::value<unsigned>()->default_value(3306), "MySQL port")
            ("mysql-user", bpo::value<string>(), "MySQL username")
            ("mysql-password", bpo::value<string>(), "MySQL password")
            ("mysql-db", bpo::value<string>()->default_value("eos"), "MySQL db name")
            ("mysql-only-irreversible", bpo::value<bool>()->default_value(false), "MySQL whether only stores irreversible blocks")
            ("mysql-start-block-num", bpo::value<unsigned>()->default_value(1), "MySQL starts syncing block number")
            ("mysql-filter-token-contract", boost::program_options::value<vector<string>>()->composing()->multitoken(),
             "MySQL token contract account added to token filtering list (may specify multiple times)");
}

void mysql_db_plugin::plugin_initialize(const variables_map& options) {
    if (not options.count("mysql-enable") || not options.at("mysql-enable").as<bool>()) {
        wlog("my_db_plugin configured, but no --mysql-enable=true specified.");
        wlog("my_db_plugin disabled.");
        return;
    }

    ilog("initializing mysql_db_plugin");
    my->configured_ = true;

    if (options.at("replay-blockchain").as<bool>()) {
        ilog("Replay requested: wiping mysql database on startup");
        my->wipe_database_on_startup_ = true;
    }
    if (options.at("hard-replay-blockchain").as<bool>()) {
        ilog("Hard replay requested: wiping mysql database on startup");
        my->wipe_database_on_startup_ = true;
    }

    string host = options.at("mysql-host").as<string>();
    unsigned port = options.at("mysql-port").as<unsigned>();
    string user, password;
    if (options.count("mysql-user")) user = options.at("mysql-user").as<std::string>();
    if (options.count("mysql-password")) password = options.at("mysql-password").as<std::string>();
    string db = options.at("mysql-db").as<std::string>();
    unsigned start_block_num = options.at("mysql-start-block-num").as<unsigned>();
    ilog("my_db_plugin connecting to ${host}:${port}", ("host", host)("port", port));

    std::unique_ptr<odb::mysql::connection_factory> conn_pool = make_unique<odb::mysql::connection_pool_factory>(5, 1, true);
    my->db_ = make_shared<odb::mysql::database>(user, password, db, host, port, nullptr, "utf8", 0, std::move(conn_pool));

    if (my->wipe_database_on_startup_) {
        my->wipe_database();
    }

    my->init();

    std::vector<std::string> filter_token_contracts;
    if(options.count("mysql-filter-token-contract")) {
        filter_token_contracts = options["mysql-filter-token-contract"].as<std::vector<std::string>>();
    }
    my->action_handlers_.push_back(std::make_shared<token_transfer_handler>(filter_token_contracts, my->db_));

    for (auto& action_handler: my->action_handlers_) {
        action_handler->start();
    }

    // add callback to chain_controller config
    my->chain_plugin_ = app().find_plugin<chain_plugin>();
    auto& chain = my->chain_plugin_->chain();
    if (not options.at("mysql-only-irreversible").as<bool>()) {
        my->block_conn_ = chain.accepted_block.connect([=](const chain::block_state_ptr& b) {
            if (not my->start_sync_) {
                if (b->block_num >= start_block_num) my->start_sync_ = true;
                else return;
            }
            handle([=] { my->push_block(b); }, "push block");
        });
    }
    my->irreversible_block_conn_ = chain.irreversible_block.connect([=](const chain::block_state_ptr& b) {
        if (not my->start_sync_) {
            if (b->block_num >= start_block_num) my->start_sync_ = true;
            else return;
        }
        handle([=] { my->push_block(b); }, "push irreversible block");
    });
    my->transaction_conn_ = chain.applied_transaction.connect([=](const chain::transaction_trace_ptr& t) {
        if (not my->start_sync_) return;
        // if (t->failed_dtrx_trace || t->except) return; // failed transaction
        handle([=] { my->push_transaction_trace(t); }, "push transaction");
    });
}

void mysql_db_plugin::plugin_startup() {
    if (my->configured_) {
        ilog("starting mysql_db_plugin");

        my->consume_block_thread_ = std::thread([=] {
            loop_handle(my->done_, "consume blocks", [=] { my->consume_blocks(); });
        });

        my->consume_transaction_thread_ = std::thread([=] {
            loop_handle(my->done_, "consume transactions", [=] { my->consume_transactions(); });
        });

        my->consume_transaction_trace_thread_ = std::thread([=] {
            loop_handle(my->done_, "consume transaction traces", [=] { my->consume_transaction_traces(); });
        });

        my->consume_action_thread_ = std::thread([=] {
            loop_handle(my->done_, "consume actions", [=] { my->consume_actions(); });
        });
    }
}

void mysql_db_plugin::plugin_shutdown() {
    my->stop();
    my.reset();
}

}
