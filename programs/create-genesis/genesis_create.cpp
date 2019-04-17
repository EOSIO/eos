#include "custom_unpack.hpp"
#include "golos_objects.hpp"
#include "genesis_create.hpp"
#include "golos_state_container.hpp"
#include "serializer.hpp"
#include <eosio/chain/config.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <fc/io/raw.hpp>
#include <fc/variant.hpp>
#include <boost/filesystem/path.hpp>


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
using acc_idx = uint32_t;       // lookup index in _acc_map


static constexpr uint64_t gls_ctrl_account_name  = N(gls.ctrl);
static constexpr uint64_t gls_vest_account_name  = N(gls.vesting);
static constexpr uint64_t gls_post_account_name  = N(gls.publish);

constexpr auto GBG = SY(3,GBG);
constexpr auto GLS = SY(3,GOLOS);
constexpr auto GESTS = SY(6,GESTS);
constexpr auto VESTS = SY(6,GOLOS);                 // Cyberway vesting
constexpr auto posting_auth_name = "posting";
constexpr auto golos_account_name = "golos";
constexpr auto issuer_account_name = config::system_account_name;   // ?cyberfounder
constexpr auto notify_account_name = gls_ctrl_account_name;
constexpr auto posting_account_name = gls_post_account_name;

constexpr auto withdraw_interval_seconds = 60*60*24*7;
constexpr auto withdraw_intervals = 13;

constexpr int64_t system_max_supply = 1'000'000'000ll * 10000;  // 4 digits precision
constexpr int64_t golos_max_supply = 1'000'000'000ll * 1000;    // 3 digits precision

constexpr uint64_t accounts_tbl_start_id = 7;           // some accounts created natively by node, this value is starting autoincrement for tables
constexpr uint64_t permissions_tbl_start_id = 17;       // Note: usage_id is id-1


account_name generate_name(string n);
string pubkey_string(const golos::public_key_type& k);
asset gbg2golos(const asset& gbg, const golos::price& price);
asset golos2sys(const asset& golos);


struct vesting_balance {
    share_type vesting;
    share_type delegated;
    share_type received;
};

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

};


struct genesis_create::genesis_create_impl final {
    genesis_info _info;
    genesis_state _conf;
    contracts_map _contracts;

    genesis_serializer db;

    vector<string> _accs_map;
    vector<string> _plnk_map;

    state_object_visitor _visitor;
    fc::flat_map<acc_idx, account_name> _acc_names;

    genesis_create_impl() {
        db.set_autoincrement<permission_object>(permissions_tbl_start_id);
        db.set_autoincrement<permission_usage_object>(permissions_tbl_start_id-1);

        db.set_autoincrement<account_object>(accounts_tbl_start_id);
        db.set_autoincrement<account_sequence_object>(accounts_tbl_start_id);
        db.set_autoincrement<resource_usage_object>(accounts_tbl_start_id);
    }

    template<typename T, typename Lambda>
    void store_accounts_wo_perms(const T& accounts, Lambda&& get_name) {
        const auto l = accounts.size();
        const auto ts = _conf.initial_timestamp;

        db.start_section(config::system_account_name, N(account), "account_object", l);
        for (const auto& acc: accounts) {
            db.emplace<account_object>([&](auto& a) {
                a.name = get_name(acc);
                a.creation_date = ts;
                if (_contracts.count(a.name) > 0) {
                    const auto& abicode = _contracts[a.name];
                    if (abicode.abi.size()) {
                        a.abi = abicode.abi;
                    }
                    const auto& code = abicode.code;
                    if (code.size()) {
                        a.last_code_update = ts;
                        a.code_version = abicode.code_hash;
                        a.code.resize(code.size());
                        memcpy(const_cast<char*>(a.code.data()), code.data(), code.size());
                    }
                }
            });
        }

        db.start_section(config::system_account_name, N(accountseq), "account_sequence_object", l);
        for (const auto& acc: accounts) {
            db.emplace<account_sequence_object>([&](auto& a) {
                a.name = get_name(acc);
            });
        }

        db.start_section(config::system_account_name, N(resusage), "resource_usage_object", l);
        for (const auto& acc: accounts) {
            db.emplace<resource_usage_object>([&](auto& u) {
                u.owner = get_name(acc);
            });
        }
    }

    void store_contracts() {
        store_accounts_wo_perms(_contracts, [](auto& a){ return a.first; });

        // we can't store records from different tables simultaneously, so save current autoincrement id to use later
        uint64_t usage_id = db.get_autoincrement<permission_usage_object>();
        const auto perms_l = _contracts.size() * 2;   // 2 = owner+active
        db.start_section(config::system_account_name, N(permusage), "permission_usage_object", perms_l);
        for (int i = 0; i < perms_l; i++) {
            db.emplace<permission_usage_object>([&](auto& p) {
                p.last_used = _conf.initial_timestamp;
            });
        }

        db.start_section(config::system_account_name, N(permission), "permission_object", perms_l);
        authority system_auth(_conf.initial_key);
        auto str2auth = [&](const std::string& str) {
            return str == "INITIAL" ? system_auth : authority(public_key_type(str));
        };
        for (const auto& acc: _info.accounts) {
            create_account_perms(acc.name, usage_id, str2auth(acc.owner_key), str2auth(acc.active_key));
        }
    }

    void read_maps(const bfs::path& state_file) {
        auto map_file = state_file;
        map_file += ".map";
        EOS_ASSERT(fc::is_regular_file(map_file), genesis_exception,
            "Genesis state map file '${f}' does not exist.", ("f", map_file.generic_string()));
        bfs::ifstream im(map_file);

        auto read_map = [&](char type, vector<string>& map) {
            std::cout << " reading map of type " << type << "... ";
            char t;
            uint32_t len;
            im >> t;
            im.read((char*)&len, sizeof(len));
            EOS_ASSERT(im, genesis_exception, "Unknown format of Genesis state map file.");
            EOS_ASSERT(t == type, genesis_exception, "Unexpected map type in Genesis state map file.");
            std::cout << "count=" << len << "... ";
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
        // TODO: checksum
        im.close();
    }

    void read_state(const bfs::path& state_file) {
        // TODO: checksum
        EOS_ASSERT(fc::is_regular_file(state_file), genesis_exception,
            "Genesis state file '${f}' does not exist.", ("f", state_file.generic_string()));
        std::cout << "Reading state from " << state_file << "..." << std::endl;
        read_maps(state_file);

        bfs::ifstream in(state_file);
        golos_state_header h{"", 0};
        in.read((char*)&h, sizeof(h));
        EOS_ASSERT(string(h.magic) == golos_state_header::expected_magic && h.version == 1, genesis_exception,
            "Unknown format of the Genesis state file.");

        while (in && !_visitor.early_exit) {
            golos_table_header t;
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

    void create_account_perms(
        name name, uint64_t& usage_id, const authority& own, const authority& act, const authority& post = authority()
    ) {
        const auto& owner  = store_permission({}, name, config::owner_name, 0, own, usage_id++);
        const auto& active = store_permission({}, name, config::active_name, owner.id, act, usage_id++);
        if (post != authority()) {
            store_permission({}, name, posting_auth_name, active.id, post, usage_id++);
        }
    }

    const permission_object store_permission(
        const ram_payer_info& ram, account_name account, permission_name name,
        authorization_manager::permission_id_type parent, authority auth, uint64_t usage_id
    ) {
        const auto perm = db.emplace<permission_object>(ram, [&](auto& p) {
            p.usage_id     = usage_id;  // perm_usage.id;
            p.parent       = parent;
            p.owner        = account;
            p.name         = name;
            p.last_updated = _conf.initial_timestamp;
            p.auth         = std::move(auth);
        });
        return perm;
    }

    void store_accounts() {
        std::cout << "Creating accounts, authorities and usernames in Golos domain..." << std::endl;
        std::unordered_map<string,account_name> names;
        names.reserve(_visitor.auths.size());

        // first fill names, we need them to check is authority account exists
        for (const auto a: _visitor.auths) {
            const auto n = a.account.value(_accs_map);
            const auto name = n.size() == 0 ? account_name() : generate_name(n);    //?config::null_account_name
            names[n] = name;
            _acc_names[a.account.id] = name;
        }

        store_accounts_wo_perms(_visitor.auths, [&](auto& a) {
            const auto n = a.account.value(_accs_map);
            return names[n];
        });

        // fill auths
        uint64_t usage_id = db.get_autoincrement<permission_usage_object>();
        const auto perms_l = _visitor.auths.size() * 3;   // 3 = owner+active+posting
        db.start_section(config::system_account_name, N(permusage), "permission_usage_object", perms_l);
        for (int i = 0; i < perms_l; i++) {
            db.emplace<permission_usage_object>([&](auto& p) {
                p.last_used = _conf.initial_timestamp;
            });
        }

        db.start_section(config::system_account_name, N(permission), "permission_object", perms_l);
        auto ts = _conf.initial_timestamp;
        for (const auto a: _visitor.auths) {
            const auto n = a.account.value(_accs_map);
            auto convert_authority = [&](permission_name perm, const golos::shared_authority& a) {
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
                return authority{threshold, keys, accounts};
            };
            const auto owner = convert_authority(config::owner_name, a.owner);
            const auto active = convert_authority(config::active_name, a.active);
            const auto posting = convert_authority(posting_auth_name, a.posting);
            create_account_perms(names[n], usage_id, owner, active, posting);

            // TODO: do we need memo key ?
            // TODO: recovery ?
        }

        // add usernames
        db.start_section(config::system_account_name, N(domain), "domain_object", 1);
        const auto app = names[golos_account_name];
        db.emplace<domain_object>([&](auto& a) {
            a.owner = app;
            a.linked_to = app;
            a.creation_date = ts;
            a.name = golos_account_name;
        });

        db.start_section(config::system_account_name, N(username), "username_object", _visitor.auths.size());
        for (const auto& auth : _visitor.auths) {                // loop through auths to preserve names order
            const auto& n = auth.account.value(_accs_map);
            db.emplace<username_object>([&](auto& u) {
                u.owner = names[n]; //generate_name(n);
                u.scope = app;
                u.name = n;
            });
        }

        _visitor.auths.clear();
        std::cout << "Done." << std::endl;
    }

    void store_balances() {
        std::cout << "Creating balances..." << std::endl;

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
        EOS_ASSERT(gests_diff.get_amount() == 0 && gls_diff.get_amount() == 0 && gbg_diff.get_amount() == 0,
            genesis_exception,
            "Failed while check balances invariants", ("gests", gests_diff)("golos", gls_diff)("gbg", gbg_diff));

        golos::price price;
        if (gp.is_forced_min_price) {
            // This price limits SBD to 10% market cap
            price = golos::price{asset(9 * gp.current_sbd_supply.get_amount(), symbol(GBG)), gp.current_supply};
        } else {
            EOS_ASSERT(false, genesis_exception, "Not implemented");
        }
        auto gbg2gls = gbg2golos(gp.current_sbd_supply, price);
        std::cout << "GBG 2 GOLOS = " << gbg2gls << std::endl;

        // token stats
#ifdef IMPORT_SYS_BALANCES
        const auto n_stats = 2;
#else
        const auto n_stats = 1;
#endif
        db.start_section(config::token_account_name, N(stat), "currency_stats", n_stats);

        auto supply = gp.current_supply + gbg2gls;
        ram_payer_info ram_payer{};
        auto insert_stat_record = [&](const symbol& sym, const asset& supply, int64_t max_supply, name issuer) {
            table_request tbl{config::token_account_name, sym.value(), N(stat)};
            primary_key_t pk = sym.value() >> 8;
            auto stat = mvo
                ("supply", supply)
                ("max_supply", asset(max_supply, sym))
                ("issuer", issuer);
            db.insert(tbl, pk, stat, ram_payer);
            return pk;
        };

        const auto sys_sym = asset().get_symbol();
        const auto golos_sym = symbol(GLS);
#ifdef IMPORT_SYS_BALANCES
        auto sys_pk = insert_stat_record(sys_sym, golos2sys(supply), system_max_supply, config::system_account_name);
#endif
        auto gls_pk = insert_stat_record(golos_sym, supply, golos_max_supply, issuer_account_name);

        // vesting info
        db.start_section(gls_vest_account_name, N(stat), "vesting_stats", 1);
        table_request tbl{gls_vest_account_name, gls_vest_account_name, N(stat)};
        primary_key_t vests_pk = VESTS >> 8;
        auto vests_info = mvo
            ("supply", asset(data.total_gests.get_amount(), symbol(VESTS)))
            ("notify_acc", notify_account_name);
        db.insert(tbl, vests_pk, vests_info, ram_payer);

        // funds
#ifdef IMPORT_SYS_BALANCES
        const auto n_balances = 2 + 2*data.gbg.size();
#else
        const auto n_balances = 2 + data.gbg.size();
#endif
        db.start_section(config::token_account_name, N(accounts), "account", n_balances);
        // TODO: convert vesting to staking and burn system tokens in reward pool?
        db.insert({config::token_account_name, gls_vest_account_name, N(accounts)}, gls_pk,
            mvo("balance", gp.total_vesting_fund_steem)("payments", asset(0, golos_sym)), ram_payer);
        db.insert({config::token_account_name, gls_post_account_name, N(accounts)}, gls_pk,
            mvo("balance", gp.total_reward_fund_steem)("payments", asset(0, golos_sym)), ram_payer);
        // TODO: internal pool of posting contract

        // accounts GOLOS/GESTS
        asset total_gls = asset(0, golos_sym);
        for (const auto& balance: data.gbg) {
            auto acc = balance.first;
            auto gbg = balance.second;
            auto gls = data.gls[acc] + gbg2golos(gbg, price);
            total_gls += gls;
            auto n = generate_name(_accs_map[acc]);
            db.insert({config::token_account_name, n, N(accounts)}, gls_pk,
                mvo("balance", gls)("payments", asset(0, golos_sym)), ram_payer);
#ifdef IMPORT_SYS_BALANCES
            db.insert({config::token_account_name, n, N(accounts)}, sys_pk,
                mvo("balance", golos2sys(gls))("payments", asset(0, sys_sym)), ram_payer);
#endif
        }

        db.start_section(gls_vest_account_name, N(accounts), "account", data.vests.size());
        for (const auto& v: data.vests) {
            auto acc = v.first;
            const auto& vests = v.second;
            EOS_ASSERT(vests.delegated == data.delegated_vests[acc], genesis_exception,
                "Calculated delegated vests doesn't equal to provided in object",
                ("account", _accs_map[acc])("calculated", data.delegated_vests[acc])("provided", vests.delegated));
            EOS_ASSERT(vests.received == data.received_vests[acc], genesis_exception,
                "Calculated received delegations doesn't equal to provided in object",
                ("account", _accs_map[acc])("calculated", data.received_vests[acc])("provided", vests.received));
            auto vests_obj = mvo
                ("vesting", asset(vests.vesting, symbol(VESTS)))
                ("delegated", asset(vests.delegated, symbol(VESTS)))
                ("received", asset(vests.received, symbol(VESTS)))
                ("unlocked_limit", asset(0, symbol(VESTS)));
            const auto n = generate_name(_accs_map[acc]);
            db.insert({gls_vest_account_name, n, N(accounts)}, vests_pk, vests_obj, ram_payer);
        }
        // TODO: update gbg2golos to uniformly distribute rounded amount
        std::cout << " Total accounts' GOLOS + converted GBG = " << total_gls << "; diff = " <<
            (supply - (gp.total_vesting_fund_steem + gp.total_reward_fund_steem) - total_gls) << std::endl;

        std::cout << "Done." << std::endl;
    }

    void store_delegation_records() {
        std::cout << "Creating vesting delegation records..." << std::endl;
        ram_payer_info ram_payer{};

        db.start_section(gls_vest_account_name, N(delegation), "delegation", _visitor.delegations.size());
        table_request tbl{gls_vest_account_name, VESTS, N(delegation)};
        primary_key_t pk = 0;
        for (const auto& d: _visitor.delegations) {
            auto obj = mvo
                ("id", pk)
                ("delegator", _acc_names[d.delegator.id])
                ("delegatee", _acc_names[d.delegatee.id])
                ("quantity", asset(d.vesting_shares.get_amount(), symbol(VESTS)))
                ("interest_rate", d.interest_rate)
                ("payout_strategy", int(d.payout_strategy))
                ("min_delegation_time", d.min_delegation_time);
            db.insert(tbl, pk, obj, ram_payer);
            pk++;
        }

        db.start_section(gls_vest_account_name, N(rdelegation), "return_delegation",
            _visitor.delegation_expirations.size());
        table_request rtbl = {gls_vest_account_name, gls_vest_account_name, N(rdelegation)};
        pk = 0;
        for (const auto& d: _visitor.delegation_expirations) {
            auto obj = mvo
                ("id", pk)
                ("delegator", _acc_names[d.delegator.id])
                ("quantity", asset(d.vesting_shares.get_amount(), symbol(VESTS)))
                ("date", d.expiration);
            db.insert(rtbl, pk, obj, ram_payer);
            pk++;
        }
        std::cout << "Done." << std::endl;
    }

    void store_withdrawals() {
        std::cout << "Creating vesting withdraw records..." << std::endl;
        const auto withdrawing_acc = [](const auto& a) {
            bool good = a.to_withdraw > 0 && a.vesting_withdraw_rate.get_amount() > 0 &&
                a.next_vesting_withdrawal != time_point_sec::maximum();
            if (good) {
                // some Golos accounts have low remainders due rounding (<0.001 GOLOS) or low withdraw rate,
                // both resulting wrong payments count. skip them
                auto payments_done = a.withdrawn / a.vesting_withdraw_rate.get_amount();
                good = payments_done >= 0 && payments_done < withdraw_intervals;
            }
            return good;
        };

        const auto& accs = _visitor.accounts;
        auto n = std::count_if(accs.begin(), accs.end(), [&](const auto& a){ return withdrawing_acc(a.second); });
        db.start_section(gls_vest_account_name, N(withdrawal), "withdrawal", n);
        table_request tbl{gls_vest_account_name, VESTS, N(withdrawal)};

        const auto& routes = _visitor.withdraw_routes;
        for (const auto& acc: accs) {
            auto a = acc.second;
            if (withdrawing_acc(a)) {
                auto idx = acc.first;
                auto owner = _acc_names[idx];
                auto to = routes.count(idx) ? _acc_names[routes.at(idx)] : owner;
                auto remaining = a.to_withdraw - a.withdrawn;
                EOS_ASSERT(remaining <= a.vesting_shares.get_amount() - a.delegated_vesting_shares.get_amount(),
                    genesis_exception, "${a} trying to withdraw more vesting than allowed", ("a",_accs_map[idx]));
                auto obj = mvo
                    ("owner", owner)
                    ("to", to)
                    ("interval_seconds", withdraw_interval_seconds)
                    ("remaining_payments", withdraw_intervals - a.withdrawn / a.vesting_withdraw_rate.get_amount())
                    ("next_payout", a.next_vesting_withdrawal)
                    ("withdraw_rate", a.vesting_withdraw_rate)
                    ("to_withdraw", asset(remaining, symbol(VESTS)));
                db.insert(tbl, owner, obj, {});
            }
        }
        std::cout << "Done." << std::endl;
    }

    void store_witnesses() {
        std::cout << "Creating witnesses..." << std::endl;
        fc::flat_map<acc_idx,int64_t> weights;  // accumulate weights per witness to compare with witness.total_weight
        ram_payer_info ram_payer{}; // TODO: fix

        // Golos dApp have no proxy for witnesses, so create direct votes instead
        const auto& empty_acc = std::distance(_accs_map.begin(), std::find(_accs_map.begin(), _accs_map.end(), string("")));
        for (const auto& acc: _visitor.accounts) {
            auto idx = acc.first;
            const auto& a = acc.second;
            if (a.proxy.id.value != empty_acc) {
                bool found = false;
                auto final_proxy = a.proxy.id;
                for (int depth = 0; !found && depth < 4; depth++) {
                    const auto& proxy = _visitor.accounts[final_proxy].proxy.id.value;
                    found = proxy == empty_acc;
                    if (!found) {
                        final_proxy = proxy;
                    }
                }
                EOS_ASSERT(found, genesis_exception, "Account proxy depth >= 4");
                _visitor.witness_votes[idx];    // force create element so container won't change after obtainig final_proxy element
                _visitor.witness_votes[idx] = _visitor.witness_votes[final_proxy];
            }
        }

        // process votes before witnesses to calculate total weights
        db.start_section(gls_ctrl_account_name, N(witnessvote), "witness_voter", _visitor.witness_votes.size());
        for (const auto& v: _visitor.witness_votes) {
            const auto& acc = v.first;
            const auto& votes = v.second;
            EOS_ASSERT(votes.size() <= 30, genesis_exception,
                "Account `${a}` have ${n} witness votes, but max 30 allowed", ("a",_accs_map[acc])("n",votes.size()));

            vector<account_name> witnesses;
            const auto& vests = _visitor.accounts[acc].vesting_shares.get_amount();
            for (const auto& w: votes) {
                witnesses.emplace_back(generate_name(_accs_map[w]));
                weights[w] += vests;
            }
            const auto& n = generate_name(_accs_map[acc]);
            auto o = mvo
                ("voter", n)
                ("witnesses", witnesses);
            primary_key_t pk = n.value;
            db.insert({gls_ctrl_account_name, gls_ctrl_account_name, N(witnessvote)}, pk, o, ram_payer);
            // TODO: BPs
        }

        db.start_section(gls_ctrl_account_name, N(witness), "witness_info", _visitor.witnesses.size());
        for (const auto& w : _visitor.witnesses) {
            const auto& n = generate_name(_accs_map[w.owner.id]);
            primary_key_t pk = n.value;
            auto obj = mvo
                ("name", n)
                ("url", w.url)
                ("active", true)
                ("total_weight", w.votes);
            db.insert({gls_ctrl_account_name, gls_ctrl_account_name, N(witness)}, pk, obj, ram_payer);
            if (weights[w.owner.id] != w.votes) {
                wlog("Witness `${a}` .votes value ${w} ≠ ${c}",
                    ("a",_accs_map[w.owner.id])("w",w.votes)("c",weights[w.owner.id]));
            }
            EOS_ASSERT(weights[w.owner.id] == w.votes, genesis_exception,
                "Witness .votes value is not equal to sum individual votes", ("w",w)("calculated",weights[w.owner.id]));
            // TODO: BPs
        }

        _visitor.witnesses.clear();
        _visitor.witness_votes.clear();
        std::cout << "Done." << std::endl;
    }

    void prepare_writer(const bfs::path& out_file) {
        const int n_sections = 5*2 + static_cast<int>(stored_contract_tables::_max);  // there are 5 duplicating account tables (system+golos)
        db.start(out_file, n_sections);
        db.prepare_serializers(_contracts);
    };
};

genesis_create::genesis_create(): _impl(new genesis_create_impl()) {
}
genesis_create::~genesis_create() {
}

void genesis_create::read_state(const bfs::path& state_file) {
    _impl->read_state(state_file);
}

void genesis_create::write_genesis(
    const bfs::path& out_file, const genesis_info& info, const genesis_state& conf, const contracts_map& accs
) {
    _impl->_info = info;
    _impl->_conf = conf;
    _impl->_contracts = accs;

    _impl->prepare_writer(out_file);
    _impl->store_contracts();
    _impl->store_accounts();
    _impl->store_balances();
    _impl->store_delegation_records();
    _impl->store_withdrawals();
    _impl->store_witnesses();

    for (const auto& i: _impl->db.autoincrement) {
        wlog("Type: ${t}; id: ${n}", ("t",i.first)("n",i.second));
    }

    _impl->db.finalize();
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
