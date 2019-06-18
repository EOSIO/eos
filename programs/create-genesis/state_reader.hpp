#pragma once
#include "genesis_create.hpp"
#include "golos_objects.hpp"
#include "golos_state_container.hpp"
#include "config.hpp"


//#define STORE_CLOSED_PERMLINKS    // disabled by default (there are 8M posts), can be enabled with -D


namespace cyberway { namespace genesis {

using namespace eosio::chain;
using acc_idx = uint32_t;       // lookup index in _accs_map
using plk_idx = uint32_t;       // lookup index in _plnk_map


struct state_object_visitor {
    using result_type = void;

    struct vesting_balance {
        share_type vesting;
        share_type delegated;
        share_type received;
    };
    struct post_permlink {
        acc_idx author;
        plk_idx permlink;
        acc_idx parent_author;
        plk_idx parent_permlink;
        uint16_t depth;
        uint32_t children;
    };

    enum balance_type {
        account, savings, order, conversion, escrow, escrow_fee, savings_withdraw, _size
    };
    static string balance_name(balance_type t, bool memo = false) {
        switch (t) {
            case account:       return memo ? "balance" : "accounts";
            case savings:       return "savings";
            case order:         return "orders";
            case conversion:    return "conversions";
            case escrow:        return "escrows";
            case escrow_fee:    return "escrow fees";
            case savings_withdraw: return "savings withdraws";
            case _size:         return "Total";
        }
        EOS_ASSERT(false, genesis_exception, "Invalid balance type ${t}", ("t", int(t)));
    }

    state_object_visitor() {
        for (int t = 0; t <= balance_type::_size; t++) {
            gls_by_type[balance_type(t)] = asset(0, symbol(GLS));
            gbg_by_type[balance_type(t)] = asset(0, symbol(GBG));
        }
        total_gests = asset(0, symbol(GESTS));
    }

    bool early_exit = false;
    golos::dynamic_global_property_object   gpo;
    fc::flat_map<acc_idx,golos::account_object> accounts;
    vector<golos::account_authority_object>     auths;
    vector<golos::witness_object>               witnesses;
    fc::flat_map<acc_idx,vector<acc_idx>>       witness_votes;
    fc::flat_map<golos::id_type,acc_idx>        acc_id2idx;     // witness_votes and some other tables store id of acc
    golos::account_object acc_by_id(golos::id_type id) {
        return accounts[acc_id2idx[id]];
    }

    fc::flat_map<acc_idx, asset> gbg;
    fc::flat_map<acc_idx, asset> gls;
    fc::flat_map<acc_idx, vesting_balance> vests;
    asset total_gests;
    fc::flat_map<balance_type, asset> gbg_by_type;
    fc::flat_map<balance_type, asset> gls_by_type;
    fc::flat_map<acc_idx, converted_info> conv_gbg;
    fc::flat_map<acc_idx, converted_info> conv_gls;

    std::vector<golos::vesting_delegation_object> delegations;
    std::vector<golos::vesting_delegation_expiration_object> delegation_expirations;
    fc::flat_map<acc_idx, share_type> delegated_vests;
    fc::flat_map<acc_idx, share_type> received_vests;

    fc::flat_map<acc_idx,acc_idx> withdraw_routes;  // from:to

    using post_bw_info = std::pair<int64_t,time_point_sec>;
    fc::flat_map<acc_idx,post_bw_info> post_bws;
    fc::flat_map<uint64_t,golos::comment_object> comments;  // id:comment
    fc::flat_map<uint64_t,post_permlink> permlinks;         // id:permlink
    std::vector<golos::comment_vote_object> votes;

    fc::flat_map<acc_idx, share_type> reputations;

    template<typename T>
    void operator()(const T& x) {}

    void operator()(const golos::dynamic_global_property_object& x) {
        gpo = x;
    }

    void operator()(const golos::account_authority_object& auth) {
        auths.emplace_back(auth);
    }

    // accounts and balances
    void operator()(const golos::account_object& acc) {
        auto idx = acc.name.id;
        acc_id2idx[acc.id] = idx;
        accounts[idx]   = acc;
        // GOLOS balance is not part of genesis conversions, but GBG balance is
        gls[idx]        = acc.balance;
        gbg[idx]        = asset(0, symbol(GBG));
        vests[idx]      = vesting_balance{
            acc.vesting_shares.get_amount(),
            acc.delegated_vesting_shares.get_amount(),
            acc.received_vesting_shares.get_amount()};
        total_gests          += acc.vesting_shares;
        gls_by_type[account] += acc.balance;
        add_asset_balance(acc.name, acc.sbd_balance, account);
        add_asset_balance(acc.name, acc.savings_balance, savings);
        add_asset_balance(acc.name, acc.savings_sbd_balance, savings);
    }

    void operator()(const golos::limit_order_object& o) {
        add_asset_balance(o.seller, asset(o.for_sale, o.sell_price.base.get_symbol()), order);
    }

    void operator()(const golos::convert_request_object& obj) {
        add_asset_balance(obj.owner, obj.amount, conversion);
    }

    void operator()(const golos::escrow_object& e) {
        add_asset_balance(e.from, e.steem_balance, escrow);
        add_asset_balance(e.from, e.sbd_balance, escrow);
        add_asset_balance(e.from, e.pending_fee, escrow_fee);
    }

    void operator()(const golos::savings_withdraw_object& sw) {
        ilog("swo: ${o}", ("o", sw));
        add_asset_balance(sw.to, sw.amount, savings_withdraw);
    }

    void add_asset_balance(golos::account_name_type account, const asset& val, balance_type type) {
        const auto acc = account.id;
        switch (val.get_symbol().value()) {
        case GBG: gbg[acc] += val; gbg_by_type[type] += val; conv_gbg[acc].add(val, balance_name(type, true)); break;
        case GLS: gls[acc] += val; gls_by_type[type] += val; conv_gls[acc].add(val, balance_name(type, true)); break;
        default:
            EOS_ASSERT(false, genesis_exception, string("Unknown asset ${a} in ") + balance_name(type), ("a", val));
        }
    };

    void operator()(const golos::vesting_delegation_object& d) {
        delegations.emplace_back(d);
        delegated_vests[d.delegator.id] += d.vesting_shares.get_amount();
        received_vests[d.delegatee.id] += d.vesting_shares.get_amount();
    }

    void operator()(const golos::vesting_delegation_expiration_object& d) {
        delegation_expirations.emplace_back(d);
        delegated_vests[d.delegator.id] += d.vesting_shares.get_amount();
        early_exit = true;
    }

    void operator()(const golos::withdraw_vesting_route_object& w) {
        if (w.from_account != w.to_account && w.percent == config::percent_100) {
            withdraw_routes[acc_id2idx[w.from_account]] = acc_id2idx[w.to_account];
        }
    }

    void operator()(const golos::change_recovery_account_request_object& r) {
        // instant apply new recovery
        accounts[r.account_to_recover.id].recovery_account = r.recovery_account;
    }

    // witnesses
    void operator()(const golos::witness_object& w) {
        witnesses.emplace_back(w);
    }

    void operator()(const golos::witness_vote_object& v) {
        witness_votes[acc_id2idx[v.account]].emplace_back(witnesses[v.witness].owner.id);
    }

    void operator()(const golos::witness_schedule_object& s) {
        // it's simpler to skip schedule
    }

    // posting data
    void operator()(const golos::account_bandwidth_object& b) {
        if (b.type == golos::bandwidth_type::post) {
            post_bws.emplace(b.account.id, post_bw_info{b.average_bandwidth, b.last_bandwidth_update});
        }
    }

    void operator()(const golos::comment_object& c) {
        bool active = c.mode != golos::comment_mode::archived;
        if (active) {
            comments.emplace(c.id, std::move(c));
        }
#ifndef STORE_CLOSED_PERMLINKS
        if (active)
#endif
        {
            permlinks.emplace(c.id, post_permlink{
                .author = c.author.id,
                .permlink = c.permlink.id,
                .parent_author = c.parent_author.id,
                .parent_permlink = c.parent_permlink.id,
                .depth = static_cast<uint16_t>(c.depth.value),
                .children = c.children.value
            });
        }
    }

    int skipped_votes = 0;
    void operator()(const golos::comment_vote_object& v) {
        if (v.num_changes >= 0) {
            votes.emplace_back(std::move(v));
        } else {
            // ilog("Vote ${id} is non-consensus. Skip.", ("id", v.id));
            skipped_votes++;
        }
    }

// EE-genesis

    void operator()(const golos::reputation_object& rep) {
        reputations[rep.account.id] = rep.reputation;
    }
};

class state_reader {
    const bfs::path& _main_state_file;
    vector<string>& _accs_map;
    vector<string>& _plnk_map;

public:
    state_reader(const bfs::path& main_state_file, vector<string>& accs, vector<string>& permlinks)
    : _main_state_file(main_state_file), _accs_map(accs), _plnk_map(permlinks) {
    }

    void read_maps() {
        auto map_file = _main_state_file;
        map_file += ".map";
        EOS_ASSERT(fc::is_regular_file(map_file), genesis_exception,
            "Genesis state map file '${f}' does not exist.", ("f", map_file.generic_string()));
        bfs::ifstream im(map_file);

        auto read_map = [&](char type, vector<string>& map) {
            std::cout << " reading map of type " << type << "... " << std::flush;
            char t;
            uint32_t len;
            im >> t;
            im.read((char*)&len, sizeof(len));
            EOS_ASSERT(im, genesis_exception, "Unknown format of Genesis state map file.");
            EOS_ASSERT(t == type, genesis_exception, "Unexpected map type in Genesis state map file.");
            std::cout << "count=" << len << "... " << std::flush;
            while (im && len) {
                string a;
                std::getline(im, a, '\0');
                map.emplace_back(a);
                len--;
            }
            EOS_ASSERT(im, genesis_exception, "Failed to read map from Genesis state map file.");
            std::cout << "done, " << map.size() << " item(s) read" << std::endl;
        };
        read_map('A', _accs_map);
        read_map('P', _plnk_map);
        // TODO: checksum
        im.close();
    }

    void read_state_file(const bfs::path& state_file, state_object_visitor& visitor) {
        if (!bfs::exists(state_file)) {
            return;
        }
        ilog("Reading state from ${f}...", ("f", state_file.generic_string()));

        bfs::ifstream in(state_file);
        golos_state_header h{"", 0, 0};
        in.read((char*)&h, sizeof(h));
        EOS_ASSERT(string(h.magic) == golos_state_header::expected_magic, genesis_exception,
            "Unknown format of the Golos state file.");
        EOS_ASSERT(h.version == 2, genesis_exception,
            "Can only open Golos state file version 2, but version ${v} provided.", ("v", h.version));
        EOS_ASSERT(h.block_num > 0, genesis_exception, "Golos state file block_num should be greater than 0.");

        while (in && !visitor.early_exit) {
            golos_table_header t;
            fc::raw::unpack(in, t);
            if (!in)
                break;
            auto type = t.type_id;
            std::cout << "Reading " << t.records_count << " record(s) from table with id=" << type << "." << std::flush;
            objects o;
            o.set_which(type);
            auto unpacker = fc::raw::unpack_static_variant<decltype(in)>(in);
            int i = 0;
            for (; in && i < t.records_count; i++) {
                o.visit(unpacker);
                o.visit(visitor);
            }
            std::cout << "  Done, " << i << " record(s) read." << std::endl;
        }

        in.close();
    }

    void read_state(state_object_visitor& visitor) {
        // TODO: checksum
        EOS_ASSERT(fc::is_regular_file(_main_state_file), genesis_exception,
            "Genesis state file '${f}' does not exist.", ("f", _main_state_file.generic_string()));
        read_maps();

        read_state_file(_main_state_file, visitor);

        auto rep_state = _main_state_file;
        rep_state += ".reputation";
        read_state_file(rep_state, visitor);

        ilog("Done reading Genesis state.");
    }
};

}} // cyberway::genesis
