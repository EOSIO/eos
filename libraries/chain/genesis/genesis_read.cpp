#include "custom_unpack.hpp"
#include "golos_objects.hpp"
#include "genesis_container.hpp"
#include <cyberway/genesis/genesis_read.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/authorization_manager.hpp>

#include <fc/io/raw.hpp>
#include <fc/variant.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

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
using std::string;
using std::vector;

constexpr auto posting_auth_name = "posting";
constexpr auto golos_account_name = "golos";


account_name generate_name(string n);
string pubkey_string(const golos::public_key_type& k);


struct genesis_read::genesis_read_impl final {
    bfs::path _state_file;
    controller& _control;
    time_point _genesis_ts;
    read_flags _flags;

    vector<string> _accs_map;
    vector<string> _plnk_map;

    vector<golos::account_authority_object> _auths;

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
        while (in) {
            table_header t;
            fc::raw::unpack(in, t);
            if (!in)
                break;
            auto type = t.type_id;
            std::cout << "Reading " << t.records_count << " record(s) from table with id=" << type << std::endl;
            objects o;
            o.set_which(type);
            auto v = fc::raw::unpack_static_variant<decltype(in)>(in);
            int i = 0;
            for (; in && i < t.records_count; i++) {
                o.visit(v);
                if (type == account_authority_object_id) {
                    _auths.emplace_back(o.get<golos::account_authority_object>());
                }
            }
            std::cout << "  done, " << i << " record(s) read." << std::endl;
            if ((_flags & read_flags::accounts) == read_flags::accounts && _auths.size() > 0)
                break;              // we do not need other tables
        }
        std::cout << "Done reading Genesis state." << std::endl;
        in.close();
    }

    void create_accounts() {
        std::cout << "Creating accounts, authorities and usernames in Golos domain..." << std::endl;
        auto& db = const_cast<chaindb_controller&>(_control.chaindb());
        auto& auth_mgr = _control.get_mutable_authorization_manager();
        std::unordered_map<string,account_name> names;
        names.reserve(_auths.size());

        // first fill names, we need them to check is authority account exists
        for (const auto a: _auths) {
            const auto n = a.account.value(_accs_map);
            const auto name =
                n.size() == 0 ? account_name() : generate_name(n);
                // n == "null" ? account_name(config::null_account_name) : generate_name(n);
                //txt_name == "temp"
            names[n] = name;
        }

        // fill auths
        for (const auto a: _auths) {
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
        }

        // add usernames
        const auto app = names[golos_account_name];
        for (const auto& auth : _auths) {                // loop through auths to preserve names order
            const auto& n = auth.account.value(_accs_map);
            db.emplace<username_object>([&](auto& u) {
                u.owner = names[n]; //generate_name(n);
                u.scope = app;
                u.name = n;
            });
        }
        std::cout << "Done." << std::endl;
    }

};

genesis_read::genesis_read(const bfs::path& genesis_file, controller& ctrl, time_point genesis_ts)
: _impl(new genesis_read_impl(genesis_file, ctrl, genesis_ts)) {
}
genesis_read::~genesis_read() {
}

void genesis_read::read(const read_flags flags) {
    _impl->_flags = flags;
    _impl->read_state();
    if (flags & accounts) {
        _impl->create_accounts();
    }
    // TODO: balances, witnesses
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


}} // cyberway
