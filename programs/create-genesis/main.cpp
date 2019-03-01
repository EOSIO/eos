#include "custom_unpack.hpp"
#include "golos_objects.hpp"
#include "data_format.hpp"
#include <eosio/chain/config.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/authorization_manager.hpp>

#include <fc/io/raw.hpp>
#include <fc/variant.hpp>
#include <fc/filesystem.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#define READ_ACCOUNTS_ONLY

namespace fc {

// ugly, but must be accessible from free functions (operator>>)
static std::vector<std::string>* _gls_accs_map;
static std::vector<std::string>* _gls_plnk_map;
static std::vector<std::string>* _gls_meta_map;

template<typename S>
S& operator>>(S& s, cw::gls_acc_name& a) {
    unsigned_int idx;
    fc::raw::unpack(s, idx);
    a.value = _gls_accs_map->at(idx);
    return s;
}
template<typename S, int N>
inline S& operator>>(S& s, cw::gls_shared_str<N>& a) {
    std::string n;
    if (N == cw::golos::sstr_type::permlink) {
        unsigned_int idx;
        fc::raw::unpack(s, idx);
        n = _gls_plnk_map->at(idx);
    } else {
        fc::raw::unpack(s, n);
    }
    a.value = n;
    return s;
}

} // fc

namespace fc { namespace raw {

template<typename T> void unpack(T& s, cw::golos::comment_object& c) {
    fc::raw::unpack(s, c.id);
    fc::raw::unpack(s, c.parent_author);
    fc::raw::unpack(s, c.parent_permlink);
    fc::raw::unpack(s, c.author);
    fc::raw::unpack(s, c.permlink);
    fc::raw::unpack(s, c.mode);
    if (c.mode != cw::golos::comment_mode::archived) {
        fc::raw::unpack(s, c.active);
    }
}

}} // fc::raw

namespace cw {

using namespace eosio::chain;
using namespace cyberway::chaindb;
namespace bfs = boost::filesystem;
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;
using std::string;


template<typename KeyType = fc::ecc::private_key_shim>
static private_key_type get_private_key(name keyname, string role = "owner") {
    return private_key_type::regenerate<KeyType>(fc::sha256::hash(string(keyname)+role));
}
template<typename KeyType = fc::ecc::private_key_shim>
static public_key_type get_public_key(name keyname, string role = "owner") {
    return get_private_key<KeyType>(keyname, role).get_public_key();
}


struct map_info {
    bool read = false;
    std::vector<std::string> items;
};

struct converter {
    void read_state();
    void read_maps();
    void create_accounts();
    void set_program_options(options_description& cli);
    void initialize(const variables_map& options);

    bfs::path state_file;
    string chaindb_url;
    string mongo_db_prefix;

    std::vector<golos::account_authority_object> auths;

    map_info accs_map;
    map_info permlink_map;
    map_info meta_map;

    fc::time_point _genesis_ts;

protected:
    // tempdir field must come before control so that during destruction the tempdir is deleted only after controller finishes
    fc::temp_directory tempdir;
public:
    std::unique_ptr<controller> control;
};

void converter::read_maps() {
    auto map_file = state_file;
    map_file += ".map";
    EOS_ASSERT(fc::is_regular_file(map_file), extract_genesis_state_exception,  // TODO: proper exceprtion
        "Genesis state map file '${f}' does not exist.", ("f", map_file.generic_string()));
    bfs::ifstream im(map_file);
    auto read_map = [&](char type, map_info& map, auto& put_to) {
        std::cout << " reading map of type " << type << "... ";
        char t;
        uint32_t len;
        im >> t;
        im.read((char*)&len, sizeof(len));
        EOS_ASSERT(im, extract_genesis_state_exception, "Unknown format of Genesis state map file.");
        EOS_ASSERT(t == type, extract_genesis_state_exception, "Wrong map type in Genesis state map file.");
        std::cout << "count=" << len << "... ";
        while (im && len) {
            std::string a;
            std::getline(im, a, '\0' );
            map.items.emplace_back(a);
            len--;
        }
        EOS_ASSERT(im, extract_genesis_state_exception, "Failed to read map from Genesis state map file.");
        map.read = true;
        put_to = &map.items;
        std::cout << "done, " << map.items.size() << " item(s) read" << std::endl;
    };
    read_map('A', accs_map, fc::_gls_accs_map);
#ifdef READ_ACCOUNTS_ONLY
    return;
#endif
    read_map('P', permlink_map, fc::_gls_plnk_map);
    read_map('M', meta_map, fc::_gls_meta_map);
}

void converter::read_state() {
    EOS_ASSERT(fc::is_regular_file(state_file), extract_genesis_state_exception,
    "Genesis state file '${f}' does not exist.", ("f", state_file.generic_string()));
    std::cout << "Reading state from " << state_file << "..." << std::endl;
    read_maps();

    bfs::ifstream in(state_file);
    state_header h{"", 0};
    in.read((char*)&h, sizeof(h));
    EOS_ASSERT(string(h.magic) == "Golos\astatE" && h.version == 1, extract_genesis_state_exception,
        "Unknown format of the Genesis state file.");
    while (in) {
        table_header t;
        fc::raw::unpack(in, t);
        if (!in)
            break;
        auto type = t.type_id;
        std::cout << "Reading " << t.records_count << " record(s) from table with id=" << type << std::endl;
        int i = 0;
        objects o;
        o.set_which(type);
        auto v = fc::raw::unpack_static_variant<decltype(in)>(in);
        while (in && i < t.records_count) {
            o.visit(v);
            if (type == account_authority_object_id) {
                auths.emplace_back(o.get<golos::account_authority_object>());
            }
            i++;
        }
#ifdef READ_ACCOUNTS_ONLY
        if (auths.size() > 0)
            break;              // we do not need other tables for now
#endif
    }

    // TODO
    std::cout << "Done." << std::endl;
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

void converter::create_accounts() {
    static const string golos_name = "golos";
    std::cout << "Creating accounts, authorities and usernames in Golos domain..." << std::endl;
    auto& db = const_cast<database&>(control->db());
    auto& auth_mgr = control->get_mutable_authorization_manager();
    std::unordered_map<string,account_name> names;

    // first fill names, we need them to check is authority account exists
    names.reserve(auths.size());
    for (const auto a: auths) {
        const auto n = a.account.value;
        const auto& name =
            n.size() == 0 ? account_name() : generate_name(n);
            // n == "null" ? account_name(config::null_account_name) : generate_name(n);
            //txt_name == "temp"
        names[n] = name;
    }
    for (const auto a: auths) {
        const auto n = a.account.value;
        const auto& name = names[n];
        db.create<account_object>([&](auto& a) {
            a.name = name;
            a.creation_date = _genesis_ts;
        });
        db.create<account_sequence_object>([&](auto & a) {
            a.name = name;
        });
        if (n == golos_name) {
            db.create<domain_object>([&](auto& a) {
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
                if (names.count(p.first.value)) {
                    accounts.emplace_back(permission_level_weight{{names[p.first.value], perm}, p.second});
                } else {
                    std::cout << "Note: account " << n << " tried to add unexisting account " << p.first.value
                        << " to it's " << perm << " authority. Skipped." << std::endl;
                }
            }
            return auth_mgr.create_permission(name, perm, id, authority{threshold, keys, accounts}, _genesis_ts);
        };
        const auto& owner_perm = add_permission(config::owner_name, a.owner, 0);
        const auto& active_perm = add_permission(config::active_name, a.active, owner_perm.id);
        const auto& posting_perm = add_permission("posting", a.posting, active_perm.id);
    }
    // add usernames
    const auto app = names[golos_name];
    for (const auto& auth : auths) {                // loop through auths to preserve names order
        const auto& n = auth.account.value;
        db.create<username_object>([&](auto& a) {
            a.owner = names[n]; //generate_name(n);
            a.scope = app;
            a.name = n;
        });
    }
    std::cout << "Done." << std::endl;
}

void converter::set_program_options(options_description& cli) {
    cli.add_options()
        ("state", bpo::value<bfs::path>()->default_value("shared_memory.bin"),
            "the location of the state file (absolute path or relative to the current directory)")
        // chaindb-type derived from url protocol
        ("chaindb-url", bpo::value<string>(&chaindb_url)->default_value("mongodb://127.0.0.1:27017"),
            "Connection address to chaindb")
        // TODO: make it configurable (it's now constant in mongodb driver)
        ("mongodb-db-prefix", bpo::value<string>(&mongo_db_prefix)->default_value("_CYBERWAY_"),
            "Prefix for database names (only for MongoDB). !Not implemented!")
        ("help", "Print this help message and exit.")
        ;

}

cyberway::chaindb::chaindb_type chaindb_type_by_url(string url) {
    chaindb_type r = chaindb_type(-1);  // TODO: define "unknown" inside chaindb_type
    auto split = url.find("://");
    if (split != string::npos) {
        auto protocol = url.substr(0, split);
        if (protocol == "mongodb") {
            r = chaindb_type::MongoDB;
        }
    }
    return r;
}

void converter::initialize(const variables_map& options) { try {
    fc::exception::enable_detailed_strace();
    _genesis_ts = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");

    auto d = options.at("state").as<bfs::path>();
    state_file = d.is_relative() ? bfs::current_path() / d : d;
    auto type = chaindb_type_by_url(chaindb_url);
    // _chaindb.reset(new chaindb_controller(type, chaindb_url));  // TODO: pass mongo_db_prefix if MongoDB
    controller::config cfg;
    cfg.chaindb_address = chaindb_url;

    cfg.blocks_dir = tempdir.path() / config::default_blocks_dir_name;
    cfg.state_dir  = tempdir.path() / config::default_state_dir_name;
    cfg.state_size = 1024*1024*8;
    cfg.state_guard_size = 0;
    cfg.reversible_cache_size = 1024*1024*8;
    cfg.reversible_guard_size = 0;
    cfg.genesis.initial_timestamp = _genesis_ts;
    cfg.genesis.initial_key = get_public_key(config::system_account_name, "active");
    // open
    control.reset(new controller(cfg));
    control->add_indices();
    control->startup([]() { return false; }, nullptr);
} FC_LOG_AND_RETHROW() }


int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false); // for potential performance boost
    options_description cli("create-genesis command line options");
    try {
        converter conv;
        conv.set_program_options(cli);
        variables_map vmap;
        bpo::store(bpo::parse_command_line(argc, argv, cli), vmap);
        bpo::notify(vmap);
        if (vmap.count("help") > 0) {
            cli.print(std::cerr);
            return 0;
        }
        conv.initialize(vmap);
        conv.read_state();
        conv.create_accounts();
    } catch (const fc::exception& e) {
        elog("${e}", ("e", e.to_detail_string()));
        return -1;
    } catch (const boost::exception& e) {
        elog("${e}", ("e", boost::diagnostic_information(e)));
        return -1;
    } catch (const std::exception& e) {
        elog("${e}", ("e", e.what()));
        return -1;
    } catch (...) {
        elog("unknown exception");
        return -1;
    }
    return 0;
}

} // cw

int main(int argc, char** argv) {
    return cw::main(argc, argv);
}


// base
FC_REFLECT(cw::golos::shared_authority, (weight_threshold)(account_auths)(key_auths))
FC_REFLECT(cw::golos::chain_properties_17, (account_creation_fee)(maximum_block_size)(sbd_interest_rate))
FC_REFLECT_DERIVED(cw::golos::chain_properties_18, (cw::golos::chain_properties_17),
    (create_account_min_golos_fee)(create_account_min_delegation)(create_account_delegation_time)(min_delegation))
FC_REFLECT_DERIVED(cw::golos::chain_properties_19, (cw::golos::chain_properties_18),
    (max_referral_interest_rate)(max_referral_term_sec)(min_referral_break_fee)(max_referral_break_fee)
    (posts_window)(posts_per_window)(comments_window)(comments_per_window)(votes_window)(votes_per_window)
    (auction_window_size)(max_delegated_vesting_interest_rate)(custom_ops_bandwidth_multiplier)
    (min_curation_percent)(max_curation_percent)(curation_reward_curve)
    (allow_distribute_auction_reward)(allow_return_auction_reward_to_fund))
FC_REFLECT(cw::golos::price, (base)(quote))
FC_REFLECT(cw::golos::beneficiary_route_type, (account)(weight))


// objects
FC_REFLECT(cw::golos::dynamic_global_property_object,
    (id)(head_block_number)(head_block_id)(time)(current_witness)(total_pow)(num_pow_witnesses)
    (virtual_supply)(current_supply)(confidential_supply)(current_sbd_supply)(confidential_sbd_supply)
    (total_vesting_fund_steem)(total_vesting_shares)(total_reward_fund_steem)(total_reward_shares2)
    (sbd_interest_rate)(sbd_print_rate)(average_block_size)(maximum_block_size)
    (current_aslot)(recent_slots_filled)(participation_count)(last_irreversible_block_num)(max_virtual_bandwidth)
    (current_reserve_ratio)(vote_regeneration_per_day)(custom_ops_bandwidth_multiplier)(is_forced_min_price)
)

FC_REFLECT(cw::golos::account_object,
    (id)(name)(memo_key)(proxy)(last_account_update)(created)(mined)
    (owner_challenged)(active_challenged)(last_owner_proved)(last_active_proved)
    (recovery_account)(last_account_recovery)(reset_account)
    (comment_count)(lifetime_vote_count)(post_count)(can_vote)(voting_power)(last_vote_time)
    (balance)(savings_balance)(sbd_balance)(sbd_seconds)(sbd_seconds_last_update)(sbd_last_interest_payment)
    (savings_sbd_balance)(savings_sbd_seconds)(savings_sbd_seconds_last_update)
    (savings_sbd_last_interest_payment)(savings_withdraw_requests)
    (vesting_shares)(delegated_vesting_shares)(received_vesting_shares)
    (vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
    (benefaction_rewards)(curation_rewards)(delegation_rewards)(posting_rewards)
    (proxied_vsf_votes)(witnesses_voted_for)
    (last_post)
    (referrer_account)(referrer_interest_rate)(referral_end_date)(referral_break_fee)
)
FC_REFLECT(cw::golos::account_authority_object, (id)(account)(owner)(active)(posting)(last_owner_update))
FC_REFLECT(cw::golos::account_bandwidth_object,
    (id)(account)(type)(average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update))
FC_REFLECT(cw::golos::account_metadata_object, (id)(account)(json_metadata))
FC_REFLECT(cw::golos::vesting_delegation_object,
    (id)(delegator)(delegatee)(vesting_shares)(interest_rate)(min_delegation_time))
FC_REFLECT(cw::golos::vesting_delegation_expiration_object, (id)(delegator)(vesting_shares)(expiration))
FC_REFLECT(cw::golos::owner_authority_history_object, (id)(account)(previous_owner_authority)(last_valid_time))
FC_REFLECT(cw::golos::account_recovery_request_object, (id)(account_to_recover)(new_owner_authority)(expires))
FC_REFLECT(cw::golos::change_recovery_account_request_object,
    (id)(account_to_recover)(recovery_account)(effective_on))

FC_REFLECT(cw::golos::witness_object,
    (id)(owner)(created)(url)(votes)(schedule)(virtual_last_update)(virtual_position)(virtual_scheduled_time)(total_missed)
    (last_aslot)(last_confirmed_block_num)(pow_worker)(signing_key)(props)(sbd_exchange_rate)(last_sbd_exchange_update)
    (last_work)(running_version)(hardfork_version_vote)(hardfork_time_vote))
FC_REFLECT(cw::golos::witness_schedule_object,
    (id)(current_virtual_time)(next_shuffle_block_num)(current_shuffled_witnesses)(num_scheduled_witnesses)
    (top19_weight)(timeshare_weight)(miner_weight)(witness_pay_normalization_factor)
    (median_props)(majority_version))
FC_REFLECT(cw::golos::witness_vote_object, (id)(witness)(account))

FC_REFLECT(cw::golos::block_summary_object, (id)(block_id))

// comment needs to be unpacked manually
// FC_REFLECT(cw::golos::comment_object, (id)(parent_author)(parent_permlink)(author)(permlink)(mode))
FC_REFLECT(cw::golos::active_comment_data,
    (created)(last_payout)(depth)(children)
    (children_rshares2)(net_rshares)(abs_rshares)(vote_rshares)(children_abs_rshares)(cashout_time)(max_cashout_time)
    (reward_weight)(net_votes)(total_votes)(root_comment)
    (curation_reward_curve)(auction_window_reward_destination)(auction_window_size)(max_accepted_payout)
    (percent_steem_dollars)(allow_replies)(allow_votes)(allow_curation_rewards)(curation_rewards_percent)
    (beneficiaries))
FC_REFLECT(cw::golos::delegator_vote_interest_rate, (account)(interest_rate)(payout_strategy))
FC_REFLECT(cw::golos::comment_vote_object,
    (id)(voter)(comment)(orig_rshares)(rshares)(vote_percent)(auction_time)(last_update)(num_changes)
    (delegator_vote_interest_rates))


FC_REFLECT(cw::golos::limit_order_object, (id)(created)(expiration)(seller)(orderid)(for_sale)(sell_price))
FC_REFLECT(cw::golos::convert_request_object, (id)(owner)(requestid)(amount)(conversion_date))
FC_REFLECT(cw::golos::liquidity_reward_balance_object, (id)(owner)(steem_volume)(sbd_volume)(weight)(last_update))
FC_REFLECT(cw::golos::withdraw_vesting_route_object, (id)(from_account)(to_account)(percent)(auto_vest))
FC_REFLECT(cw::golos::escrow_object,
    (id)(escrow_id)(from)(to)(agent)(ratification_deadline)(escrow_expiration)
    (sbd_balance)(steem_balance)(pending_fee)(to_approved)(agent_approved)(disputed))
FC_REFLECT(cw::golos::savings_withdraw_object, (id)(from)(to)(memo)(request_id)(amount)(complete))
FC_REFLECT(cw::golos::decline_voting_rights_request_object, (id)(account)(effective_date))

FC_REFLECT(cw::golos::transaction_object,       BOOST_PP_SEQ_NIL)
FC_REFLECT(cw::golos::feed_history_object,      BOOST_PP_SEQ_NIL)
FC_REFLECT(cw::golos::hardfork_property_object, BOOST_PP_SEQ_NIL)
FC_REFLECT(cw::golos::block_stats_object,       BOOST_PP_SEQ_NIL)
FC_REFLECT(cw::golos::proposal_object,          BOOST_PP_SEQ_NIL)
FC_REFLECT(cw::golos::required_approval_object, BOOST_PP_SEQ_NIL)


FC_REFLECT_ENUM(cw::golos::bandwidth_type, (post)(forum)(market)(custom_json))
FC_REFLECT_ENUM(cw::golos::curation_curve, (detect)(bounded)(linear)(square_root)(_size))
FC_REFLECT_ENUM(cw::golos::delegator_payout_strategy, (to_delegator)(to_delegated_vesting))
FC_REFLECT_ENUM(cw::golos::witness_schedule_type, (top19)(timeshare)(miner)(none))
FC_REFLECT_ENUM(cw::golos::comment_mode, (not_set)(first_payout)(second_payout)(archived))
FC_REFLECT_ENUM(cw::golos::auction_window_reward_destination_type, (to_reward_fund)(to_curators)(to_author))
