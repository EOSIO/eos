#include "action_handler.hpp"

#include <fc/io/raw.hpp>

#include "try_handle.hpp"

namespace eosio {

action_handler_base::action_handler_base(std::string name, std::shared_ptr<odb::database> db) : name_(std::move(name)), db_(db) {
}

action_handler_base::~action_handler_base() {
}

void action_handler_base::stop() {
    try {
        done_ = true;
        awaken();
        if (thread_.joinable()) thread_.join();
        if (db_) db_.reset();
    } catch (const std::exception& e) {
        elog("Exception on my_db_plugin '${name}' action handler shutdown: ${e}", ("name", name_)("e", e.what()));
    }
}

void action_handler_base::awaken() {}

token_transfer_handler::token_transfer_handler(const std::vector<std::string>& contracts, std::shared_ptr<odb::database> db) : action_handler_base("token_transfer", db) {
    ilog("my_db_plugin token_transfer_handler extra contracts ${contracts}", ("contracts", contracts));
    for (auto& contract: contracts) {
        contract_names_.emplace(contract);
    }
}

void token_transfer_handler::start() {
    thread_ = std::thread([this] {
        loop_handle(done_, "consume token transfer actions", [=] { consume_token_transfers(); });
    });
}

void token_transfer_handler::awaken() {
    token_transfer_queue_.awaken();
}

void token_transfer_handler::consume_token_transfers() {
    using query = odb::query<TokenTransfer>;

    auto transfers = token_transfer_queue_.pop();
    if (transfers.empty()) return;

    std::unordered_map<uint64_t, TokenTransferPtr> distinct_transfers;
    vector<uint64_t> seqs;
    seqs.reserve(transfers.size());
    for (auto it = transfers.rbegin(); it != transfers.rend(); ++it) {
        auto& transfer = *it;
        if (distinct_transfers.count(transfer->action_global_seq_)) continue; // deduplicate
        distinct_transfers[transfer->action_global_seq_] = transfer;
        seqs.push_back(transfer->action_global_seq_);
    }

    odb::transaction t(db_->begin());

    query q(query::action_global_seq.in_range(seqs.begin(), seqs.end()));
    auto result = db_->query<TokenTransfer>(q);
    std::unordered_set<uint64_t> set;
    for (auto it = result.begin(); it != result.end(); ++it) {
        set.insert(it.id());
    }
    for (auto& p: distinct_transfers) {
        auto& transfer = p.second;
        if (set.count(transfer->action_global_seq_)) db_->update(*transfer);
        else db_->persist(*transfer);
    }

    t.commit();
}

struct transfer_args {
    account_name  from;
    account_name  to;
    asset         quantity;
    string        memo;
};

void token_transfer_handler::handle(const chain::action& action, uint64_t receiver, uint64_t seq, const TransactionTracePtr& tx) {
    // filter interested action
    if (action.name != action_name_) return;
    if (not contract_names_.count(action.account)) return;
    if (name(receiver) != action.account) return; // filter out notification in `transfer` action

    auto args = fc::raw::unpack<transfer_args>(action.data);
    auto& quantity = args.quantity;

    auto token_transfer = std::make_shared<TokenTransfer>();
    token_transfer->action_global_seq_ = seq;
    token_transfer->symbol_ = quantity.symbol_name();
    token_transfer->precision_ = quantity.decimals();
    token_transfer->amount_ = quantity.get_amount();
    token_transfer->from_ = args.from;
    token_transfer->to_ = args.to;
    if (not args.memo.empty()) token_transfer->memo_ = args.memo;
    token_transfer->tx_id_ = tx->id_;

    token_transfer_queue_.push(token_transfer);
}

}

FC_REFLECT(eosio::transfer_args, (from)(to)(quantity)(memo))
