#pragma once

#include "golos_objects.hpp"
#include <eosio/chain/name.hpp>
#include <fc/container/flat_fwd.hpp>
#include <fc/container/flat.hpp>
#include <fc/variant.hpp>

namespace cyberway { namespace genesis {

using mvo = fc::mutable_variant_object;
using namespace eosio::chain;
using acc_idx = uint32_t;

struct converted_info {
    asset value;
    string memo;

private:
    string _cur_type;   // to group values by type
    asset _cur_sum;

    void update_memo(const asset& x, string type) {
        if (_cur_type == type) {
            _cur_sum += x;
        } else {
            if (_cur_type.size() > 0) {
                if (memo.size() > 0)
                    memo += " + ";
                memo += _cur_sum.to_string() + " (" + _cur_type + ")";
            }
            _cur_type = type;
            _cur_sum = x;
        }
    }

public:
    void add(const asset& x, string type) {
        if (x.get_amount() == 0)
            return;
        if (empty()) {
            value = x;                  // sets symbol
        } else {
            value += x;
        }
        update_memo(x, type);
    }

    void finish() {
        update_memo(asset(), "non-existing");
    }

    void converted(const asset& x) {
        if (x.get_amount() == 0 || value.get_amount() == 0)
            return;
        finish();
        if (std::count(memo.begin(), memo.end(), '(') > 1)  // only add "=" if there are several records
            memo = value.to_string() + " = " + memo;
        value = x;
    }

    bool empty() const {
        return value.get_amount() == 0;
    }
};

struct export_info {
    fc::flat_map<acc_idx, mvo> account_infos;
    fc::flat_map<acc_idx, mvo> witnesses;
    fc::flat_map<acc_idx, fc::flat_set<name>> witness_votes;
    std::vector<mvo> currency_stats;
    std::vector<mvo> balance_events;
    std::vector<mvo> delegations;
    fc::flat_map<uint64_t, cyberway::golos::active_comment_data> active_comments;

    fc::flat_map<acc_idx, converted_info>* conv_gbg = nullptr;
    fc::flat_map<acc_idx, converted_info>* conv_gls = nullptr;
};

}} // cyberway::genesis
