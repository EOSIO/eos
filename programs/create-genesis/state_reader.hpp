#pragma once
#include "genesis_create.hpp"
#include "golos_objects.hpp"
#include "golos_state_container.hpp"
#include "config.hpp"


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

    enum balance_type {
        account, savings, order, conversion, escrow, escrow_fee, savings_withdraw, _size
    };
    static string balance_name(balance_type t) {
        switch (t) {
            case account:       return "accounts";
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

    std::vector<golos::vesting_delegation_object> delegations;
    std::vector<golos::vesting_delegation_expiration_object> delegation_expirations;
    fc::flat_map<acc_idx, share_type> delegated_vests;
    fc::flat_map<acc_idx, share_type> received_vests;

    fc::flat_map<acc_idx,acc_idx> withdraw_routes;  // from:to

    using post_bw_info = std::pair<int64_t,time_point_sec>;
    fc::flat_map<acc_idx,post_bw_info> post_bws;
    fc::flat_map<uint64_t,golos::comment_object> comments;  // id:comment
    std::vector<golos::comment_vote_object> votes;

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
        gls[idx]        = acc.balance + acc.savings_balance;
        gbg[idx]        = acc.sbd_balance + acc.savings_sbd_balance;
        vests[idx]      = vesting_balance{
            acc.vesting_shares.get_amount(),
            acc.delegated_vesting_shares.get_amount(),
            acc.received_vesting_shares.get_amount()};
        total_gests          += acc.vesting_shares;
        gls_by_type[account] += acc.balance;
        gbg_by_type[account] += acc.sbd_balance;
        gls_by_type[savings] += acc.savings_balance;
        gbg_by_type[savings] += acc.savings_sbd_balance;
    }

    void operator()(const golos::limit_order_object& o) {
        switch (o.sell_price.base.get_symbol().value()) {
        case GBG: {auto a = asset(o.for_sale, symbol(GBG)); gbg[o.seller.id] += a; gbg_by_type[order] += a;} break;
        case GLS: {auto a = asset(o.for_sale, symbol(GLS)); gls[o.seller.id] += a; gls_by_type[order] += a;} break;
        default:
            EOS_ASSERT(false, genesis_exception, "Unknown asset ${a} in limit order", ("a", o.sell_price.base));
        }
    }

    void operator()(const golos::convert_request_object& obj) {
        add_asset_balance(obj, &golos::convert_request_object::owner, &golos::convert_request_object::amount, conversion);
    }

    void operator()(const golos::escrow_object& e) {
        gls[e.from.id] += e.steem_balance;
        gbg[e.from.id] += e.sbd_balance;
        gls_by_type[escrow] += e.steem_balance;
        gbg_by_type[escrow] += e.sbd_balance;
        add_asset_balance(e, &golos::escrow_object::from, &golos::escrow_object::pending_fee, escrow_fee);
    }

    void operator()(const golos::savings_withdraw_object& sw) {
        add_asset_balance(sw, &golos::savings_withdraw_object::to, &golos::savings_withdraw_object::amount, savings_withdraw);
    }

    template<typename T, typename A, typename F>
    void add_asset_balance(const T& item, A account, F field, balance_type type) {
        const auto acc = (item.*account).id;
        const auto val = item.*field;
        switch (val.get_symbol().value()) {
        case GBG: gbg[acc] += val; gbg_by_type[type] += val; break;
        case GLS: gls[acc] += val; gls_by_type[type] += val; break;
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
        if (c.mode != golos::comment_mode::archived) {
            comments.emplace(c.id, std::move(c));
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

};

class state_reader {
    const bfs::path& _state_file;
    vector<string>& _accs_map;
    vector<string>& _plnk_map;

public:
    state_reader(const bfs::path& state_file, vector<string>& accs, vector<string>& permlinks)
    : _state_file(state_file), _accs_map(accs), _plnk_map(permlinks) {
    }

    void read_maps() {
        auto map_file = _state_file;
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

    void read_state(state_object_visitor& visitor) {
        // TODO: checksum
        EOS_ASSERT(fc::is_regular_file(_state_file), genesis_exception,
            "Genesis state file '${f}' does not exist.", ("f", _state_file.generic_string()));
        std::cout << "Reading state from " << _state_file << "..." << std::endl;
        read_maps();

        bfs::ifstream in(_state_file);
        golos_state_header h{"", 0};
        in.read((char*)&h, sizeof(h));
        EOS_ASSERT(string(h.magic) == golos_state_header::expected_magic && h.version == 1, genesis_exception,
            "Unknown format of the Genesis state file.");

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
        std::cout << "Done reading Genesis state." << std::endl;
        in.close();
    }
};

}} // cyberway::genesis
