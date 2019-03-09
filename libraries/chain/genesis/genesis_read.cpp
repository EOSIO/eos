#include "custom_unpack.hpp"
#include "golos_objects.hpp"
#include "genesis_container.hpp"
#include <cyberway/genesis/genesis_read.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>

#include <fc/io/raw.hpp>
#include <fc/variant.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

//#define IMPORT_SYS_BALANCES

namespace fc { namespace raw {

template<typename T> void unpack(T& s, cyberway::golos::comment_object& c) {
    fc::raw::unpack(s, c.id);
    fc::raw::unpack(s, c.parent_author);
    fc::raw::unpack(s, c.parent_permlink);
    fc::raw::unpack(s, c.author);
    fc::raw::unpack(s, c.permlink);
    fc::raw::unpack(s, c.mode);
    if (c.mode != cyberway::golos::comment_mode::archived) {
        fc::raw::unpack(s, c.active);
    }
}

}} // fc::raw


namespace cyberway { namespace genesis {

using namespace eosio::chain;
using namespace cyberway::chaindb;
using mvo = mutable_variant_object;
using std::string;
using std::vector;


constexpr auto GBG = SY(3,GBG);
constexpr auto GLS = SY(3,GOLOS);
constexpr auto GESTS = SY(6,GESTS);
constexpr auto posting_auth_name = "posting";
constexpr auto golos_account_name = "golos";
constexpr auto issuer_account_name = config::system_account_name;   // ?cyberfounder
constexpr auto notify_account_name = config::gls_ctrl_account_name;
constexpr auto posting_account_name = config::gls_post_account_name;
constexpr int64_t system_max_supply = 1'000'000'000ll * 10000; // 4 digits precision
constexpr int64_t golos_max_supply = 1'000'000'000ll * 1000; // 3 digits precision


account_name generate_name(string n);
string pubkey_string(const golos::public_key_type& k);
asset gbg2golos(const asset& gbg, const golos::price& price);
asset golos2sys(const asset& golos);


struct state_object_visitor {
    using result_type = void;

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
        EOS_ASSERT(false, extract_genesis_state_exception, "Invalid balance type ${t}", ("t", int(t)));
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
    vector<golos::account_object>           accounts;
    vector<golos::account_authority_object> auths;

    fc::flat_map<uint32_t, asset> gbg;
    fc::flat_map<uint32_t, asset> gls;
    fc::flat_map<uint32_t, asset> gests;
    asset total_gests;
    fc::flat_map<balance_type, asset> gbg_by_type;
    fc::flat_map<balance_type, asset> gls_by_type;

    template<typename T>
    void operator()(const T& x) {}

    void operator()(const golos::dynamic_global_property_object& x) {
        gpo = x;
    }

    void operator()(const golos::account_authority_object& auth) {
        auths.emplace_back(auth);
    }

    void operator()(const golos::account_object& acc) {
        accounts.emplace_back(acc);
        gls[acc.name.id] = acc.balance + acc.savings_balance;
        gbg[acc.name.id] = acc.sbd_balance + acc.savings_sbd_balance;
        gests[acc.name.id] = acc.vesting_shares;
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
            EOS_ASSERT(false, extract_genesis_state_exception,
                "Unknown asset ${a} in limit order", ("a", o.sell_price.base));
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
        early_exit = true;
    }

    template<typename T, typename A, typename F>
    void add_asset_balance(const T& item, A account, F field, balance_type type) {
        const auto acc = (item.*account).id;
        const auto val = item.*field;
        switch (val.get_symbol().value()) {
        case GBG: gbg[acc] += val; gbg_by_type[type] += val; break;
        case GLS: gls[acc] += val; gls_by_type[type] += val; break;
        default:
            EOS_ASSERT(false, extract_genesis_state_exception,
                string("Unknown asset ${a} in ") + balance_name(type), ("a", val));
        }
    };

};

struct genesis_read::genesis_read_impl final {
    bfs::path _state_file;
    controller& _control;
    time_point _genesis_ts;

    vector<string> _accs_map;
    vector<string> _plnk_map;

    state_object_visitor _visitor;

    genesis_read_impl(const bfs::path& genesis_file, controller& ctrl, time_point genesis_ts)
    :   _state_file(genesis_file),
        _control(ctrl),
        _genesis_ts(genesis_ts) {
    }

    void read_maps() {
        auto map_file = _state_file;
        map_file += ".map";
        EOS_ASSERT(fc::is_regular_file(map_file), extract_genesis_state_exception,  // TODO: proper exception type
            "Genesis state map file '${f}' does not exist.", ("f", map_file.generic_string()));
        bfs::ifstream im(map_file);

        auto read_map = [&](char type, vector<string>& map) {
            std::cout << " reading map of type " << type << "... ";
            char t;
            uint32_t len;
            im >> t;
            im.read((char*)&len, sizeof(len));
            EOS_ASSERT(im, extract_genesis_state_exception, "Unknown format of Genesis state map file.");
            EOS_ASSERT(t == type, extract_genesis_state_exception, "Unexpected map type in Genesis state map file.");
            std::cout << "count=" << len << "... ";
            while (im && len) {
                string a;
                std::getline(im, a, '\0' );
                map.emplace_back(a);
                len--;
            }
            EOS_ASSERT(im, extract_genesis_state_exception, "Failed to read map from Genesis state map file.");
            std::cout << "done, " << map.size() << " item(s) read" << std::endl;
        };
        read_map('A', _accs_map);
        read_map('P', _plnk_map);
        // TODO: checksum
        im.close();
    }

    void read_state() {
        // TODO: checksum
        EOS_ASSERT(fc::is_regular_file(_state_file), extract_genesis_state_exception,
            "Genesis state file '${f}' does not exist.", ("f", _state_file.generic_string()));
        std::cout << "Reading state from " << _state_file << "..." << std::endl;
        read_maps();

        bfs::ifstream in(_state_file);
        state_header h{"", 0};
        in.read((char*)&h, sizeof(h));
        EOS_ASSERT(string(h.magic) == state_header::expected_magic && h.version == 1, extract_genesis_state_exception,
            "Unknown format of the Genesis state file.");

        while (in && !_visitor.early_exit) {
            table_header t;
            fc::raw::unpack(in, t);
            if (!in)
                break;
            auto type = t.type_id;
            std::cout << "Reading " << t.records_count << " record(s) from table with id=" << type << std::endl;
            objects o;
            o.set_which(type);
            auto unpacker = fc::raw::unpack_static_variant<decltype(in)>(in);
            int i = 0;
            for (; in && i < t.records_count; i++) {
                o.visit(unpacker);
                o.visit(_visitor);
            }
            std::cout << "  done, " << i << " record(s) read." << std::endl;
        }
        std::cout << "Done reading Genesis state." << std::endl;
        in.close();
    }

    void create_accounts() {
        std::cout << "Creating accounts, authorities and usernames in Golos domain..." << std::endl;
        auto& db = const_cast<chaindb_controller&>(_control.chaindb());
        auto& auth_mgr = _control.get_mutable_authorization_manager();
        auto& reslim_mgr = _control.get_mutable_resource_limits_manager();
        std::unordered_map<string,account_name> names;
        names.reserve(_visitor.auths.size());

        // first fill names, we need them to check is authority account exists
        for (const auto a: _visitor.auths) {
            const auto n = a.account.value(_accs_map);
            const auto name = n.size() == 0 ? account_name() : generate_name(n);    //?config::null_account_name
            names[n] = name;
        }

        // fill auths
        int i = 0;
        for (const auto a: _visitor.auths) {
            const auto n = a.account.value(_accs_map);
            const auto& name = names[n];
            db.emplace<account_object>([&](auto& a) {
                a.name = name;
                a.creation_date = _genesis_ts;
            });
            db.emplace<account_sequence_object>([&](auto & a) {
                a.name = name;
            });
            if (n == golos_account_name) {
                db.emplace<domain_object>([&](auto& a) {
                    a.owner = name;
                    a.linked_to = name;
                    a.creation_date = _genesis_ts;
                    a.name = n;
                });
            }
            auto add_permission = [&](permission_name perm, const golos::shared_authority& a, auto id) {
                uint32_t threshold = a.weight_threshold;
                vector<key_weight> keys;
                for (const auto& k: a.key_auths) {
                    // can't construct public key directly, constructor is private. transform to string first:
                    const auto key = string(fc::crypto::config::public_key_legacy_prefix) + pubkey_string(k.first);
                    keys.emplace_back(key_weight{public_key_type(key), k.second});
                }
                vector<permission_level_weight> accounts;
                for (const auto& p: a.account_auths) {
                    auto auth_acc = p.first.value(_accs_map);
                    if (names.count(auth_acc)) {
                        accounts.emplace_back(permission_level_weight{{names[auth_acc], perm}, p.second});
                    } else {
                        std::cout << "Note: account " << n << " tried to add unexisting account " << auth_acc
                            << " to it's " << perm << " authority. Skipped." << std::endl;
                    }
                }
                return auth_mgr.create_permission({}, name, perm, id, authority{threshold, keys, accounts}, _genesis_ts);
            };
            const auto& owner_perm = add_permission(config::owner_name, a.owner, 0);
            const auto& active_perm = add_permission(config::active_name, a.active, owner_perm.id);
            const auto& posting_perm = add_permission(posting_auth_name, a.posting, active_perm.id);
            // TODO: do we need memo key ?

            reslim_mgr.initialize_account(name);
            if ((++i & 0xFF) == 0) {
                db.apply_all_changes();
                db.clear_cache();
            }
        }

        // add usernames
        const auto app = names[golos_account_name];
        for (const auto& auth : _visitor.auths) {                // loop through auths to preserve names order
            const auto& n = auth.account.value(_accs_map);
            db.emplace<username_object>([&](auto& u) {
                u.owner = names[n]; //generate_name(n);
                u.scope = app;
                u.name = n;
            });
            if ((++i & 0x3FF) == 0) {
                db.apply_all_changes();
                db.clear_cache();
            }
        }

        _visitor.auths.clear();
        std::cout << "Done." << std::endl;
    }

    void create_balances() {
        std::cout << "Creating balances..." << std::endl;
        auto& db = const_cast<chaindb_controller&>(_control.chaindb());

        // check invariants first
        auto& data = _visitor;
        const auto& gp = data.gpo;
        std::cout << " Global Properties:" << std::endl;
        std::cout << "  current_supply = " << gp.current_supply << std::endl;
        std::cout << "  current_sbd_supply = " << gp.current_sbd_supply << std::endl;
        std::cout << "  total_vesting_shares = " << gp.total_vesting_shares << std::endl;
        std::cout << "  total_reward_fund_steem = " << gp.total_reward_fund_steem << std::endl;
        std::cout << "  total_vesting_fund_steem = " << gp.total_vesting_fund_steem << std::endl;

        std::cout << " GESTS total: " << data.total_gests << std::endl;
        auto total_idx = state_object_visitor::balance_type::_size;
        for (const auto& t: data.gbg_by_type) {
            auto& type = t.first;
            auto& gbg = t.second;
            auto& gls = data.gls_by_type[type];
            std::cout << std::setw(19) << std::left << (" " + data.balance_name(type) + ":")
                << std::setw(19) << std::right << gls << std::setw(17) << gbg << std::endl;
            if (type < total_idx) {
                data.gls_by_type[total_idx] += gls;
                data.gbg_by_type[total_idx] += gbg;
            }
        }
        auto gests_diff = gp.total_vesting_shares - data.total_gests;
        auto gls_diff = gp.current_supply -
            (gp.total_vesting_fund_steem + gp.total_reward_fund_steem + data.gls_by_type[total_idx]);
        auto gbg_diff = gp.current_sbd_supply - data.gbg_by_type[total_idx];
        EOS_ASSERT(gests_diff.get_amount() == 0 && gls_diff.get_amount() == 0 && gbg_diff.get_amount() == 0, extract_genesis_state_exception,
            "Failed while check balances invariants", ("gests", gests_diff)("golos", gls_diff)("gbg", gbg_diff));

        golos::price price;
        if (gp.is_forced_min_price) {
            // This price limits SBD to 10% market cap
            price = golos::price{asset(9 * gp.current_sbd_supply.get_amount(), symbol(GBG)), gp.current_supply};
        } else {
            EOS_ASSERT(false, extract_genesis_state_exception, "Not implemented");
        }
        auto gbg2gls = gbg2golos(gp.current_sbd_supply, price);
        std::cout << "GBG 2 GOLOS = " << gbg2gls << std::endl;

        // token stats
        auto supply = gp.current_supply + gbg2gls;
        ram_payer_info ram_payer{};
        auto insert_stat_record = [&](const symbol& sym, const asset& supply, int64_t max_supply, name issuer) {
            table_request tbl{config::token_account_name, sym.value(), N(stat)};
            primary_key_t pk = sym.value() >> 8;
            auto stat = mvo
                ("supply", supply)
                ("maximum_supply", asset(max_supply, sym))
                ("issuer", issuer);
            db.insert(tbl, pk, stat, ram_payer);
            return pk;
        };

        const auto sys_sym = asset().get_symbol();
        const auto golos_sym = symbol(GLS);
        const auto vests_sym = symbol(6,"GOLOS");   // Cyberway vesting
#ifdef IMPORT_SYS_BALANCES
        auto sys_pk = insert_stat_record(sys_sym, golos2sys(supply), system_max_supply, config::system_account_name);
#endif
        auto gls_pk = insert_stat_record(golos_sym, supply, golos_max_supply, issuer_account_name);

        // vesting info
        // table_request tbl{config::gls_vest_account_name, vests_sym.value(), N(vesting)}; // TODO change scope
        table_request tbl{config::gls_vest_account_name, config::gls_vest_account_name, N(vesting)};
        primary_key_t vests_pk = vests_sym.value() >> 8;
        auto vests_info = mvo
            ("supply", asset(data.total_gests.get_amount(), vests_sym))
            ("notify_acc", notify_account_name);
        db.insert(tbl, vests_pk, vests_info, ram_payer);

        // funds
        // TODO: convert vesting to staking and burn system tokens in reward pool?
        db.insert({config::token_account_name, config::gls_vest_account_name, N(accounts)}, gls_pk,
            mvo("balance", gp.total_vesting_fund_steem), ram_payer);
        db.insert({config::token_account_name, config::gls_post_account_name, N(accounts)}, gls_pk,
            mvo("balance", gp.total_reward_fund_steem), ram_payer);
        // TODO: internal pool of posting contract

        // accounts GOLOS/GESTS
        auto vests_obj = mvo
            ("delegate_vesting", asset(0, vests_sym))
            ("received_vesting", asset(0, vests_sym))
            ("unlocked_limit", asset(0, vests_sym));
        asset total_gls = asset(0, golos_sym);
        int i = 0;
        for (const auto& balance: data.gbg) {
            auto acc = balance.first;
            auto gbg = balance.second;
            auto gls = data.gls[acc] + gbg2golos(gbg, price);
            total_gls += gls;
            auto n = generate_name(_accs_map[acc]);
            db.insert({config::token_account_name, n, N(accounts)}, gls_pk, mvo("balance", gls), ram_payer);
#ifdef IMPORT_SYS_BALANCES
            db.insert({config::token_account_name, n, N(accounts)}, sys_pk, mvo("balance", golos2sys(gls)), ram_payer);
#endif
            db.insert({config::gls_vest_account_name, n, N(accounts)}, vests_pk,
                vests_obj("vesting", asset(data.gests[acc].get_amount(), vests_sym)), ram_payer);
            if ((++i & 0xFF) == 0) {
                db.apply_all_changes();
                db.clear_cache();
            }
        }
        // TODO: update gbg2golos to uniformly distribute rounded amount
        std::cout << " Total accounts' GOLOS + converted GBG = " << total_gls << "; diff = " <<
            (supply - (gp.total_vesting_fund_steem + gp.total_reward_fund_steem) - total_gls) << std::endl;

        // TODO: delegation / vesting withdraws

        std::cout << "Done." << std::endl;
    }
};

genesis_read::genesis_read(const bfs::path& genesis_file, controller& ctrl, time_point genesis_ts)
: _impl(new genesis_read_impl(genesis_file, ctrl, genesis_ts)) {
}
genesis_read::~genesis_read() {
}

void genesis_read::read() {
    _impl->read_state();
    _impl->create_accounts();
    _impl->create_balances();
    // TODO: witnesses
}



account_name generate_name(string n) {
    // TODO: replace with better function
    // TODO: remove dots from result (+append trailing to length of 12 characters)
    uint64_t h = std::hash<std::string>()(n);
    return account_name(h & 0xFFFFFFFFFFFFFFF0);
}

string pubkey_string(const golos::public_key_type& k) {
    using checksummer = fc::crypto::checksummed_data<golos::public_key_type>;
    checksummer wrapper;
    wrapper.data = k;
    wrapper.check = checksummer::calculate_checksum(wrapper.data);
    auto packed = raw::pack(wrapper);
    return fc::to_base58(packed.data(), packed.size());
}

asset gbg2golos(const asset& gbg, const golos::price& price) {
    return asset(
        static_cast<int64_t>(static_cast<eosio::chain::int128_t>(gbg.get_amount()) * price.quote.get_amount() / price.base.get_amount()),
        price.quote.get_symbol()
    );
}

asset golos2sys(const asset& golos) {
    return asset(golos.get_amount() * (10000/1000));
}

}} // cyberway
