#include <cyberway/genesis/genesis_import.hpp>
#include <cyberway/genesis/genesis_container.hpp>
#include <cyberway/chaindb/abi_info.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>


namespace cyberway { namespace genesis {

FC_DECLARE_EXCEPTION(extract_genesis_exception, 9000000, "genesis extract exception");

using namespace eosio::chain;
using namespace cyberway::chaindb;
using resource_manager = eosio::chain::resource_limits::resource_limits_manager;


struct genesis_import::impl final {
    bfs::path _state_file;

    resource_manager& resource_mng;
    chaindb_controller& db;
    int db_updates;

    impl(const bfs::path& genesis_file, controller& ctrl)
    :   _state_file(genesis_file)
    ,   resource_mng(ctrl.get_mutable_resource_limits_manager())
    ,   db(ctrl.chaindb()) {
    }

    void apply_db_changes(bool force = false) {
        if (force || (++db_updates & 0xFFF) == 0) {
            db.apply_all_changes();
            db.push_cache();
        }
    }

    storage_payer_info ram_payer_info(const sys_table_row& row) {
        // TODO: Fix better. I am not yet fully investigated storage paying, it just fixes build
        return resource_mng.get_storage_payer(0, row.ram_payer);
    };

    bool update_account(const sys_table_row& r) {
        // we need primary key for update, but it depends on table. add this hacky shortcut for accounts
        primary_key_t pk = ((primary_key_t*)r.data.data())[0];
        const name n(pk);
        const auto* old = db.find<account_object>(n, cyberway::chaindb::cursor_kind::InRAM);  // vm_type/vm_version/privileged not set in genesis, copy
        if (!old) {
            return false;
        }
        fc::datastream<const char*> ds(r.data.data(), r.data.size());
        auto acc = account_object(primary_key::Unset, [&](auto& a) {
            fc::raw::unpack(ds, a);
        });
        db.modify(*old, [&](auto& a) {
            a.last_code_update = acc.last_code_update;
            a.code_version = acc.code_version;
            a.abi_version = acc.abi_version;
            a.creation_date = acc.creation_date;
            a.code = acc.code;
            if (a.name == eosio::chain::config::system_account_name) {
                a.abi = chaindb::merge_abi_def(eosio::chain::eosio_contract_abi(), acc.abi);
            } else if (a.name == eosio::chain::config::domain_account_name) {
                a.abi = chaindb::merge_abi_def(eosio::chain::domain_contract_abi(), acc.abi);
            } else {
                a.abi = acc.abi;
            }
        });
        return true;
    }

    void import_state() {
        // file existance already checked when calculated hash
        std::cout << "Reading state from " << _state_file << "..." << std::endl;
        bfs::ifstream in(_state_file);
        genesis_header h{"", 0, 0};
        in.read((char*)&h, sizeof(h));
        std::cout << "Header magic: " << h.magic << "; ver: " << h.version << std::endl;
        EOS_ASSERT(h.is_valid(), extract_genesis_exception, "Unknown format of the Genesis state file.");

        while (in) {
            table_header t;
            fc::raw::unpack(in, t);
            if (!in)
                break;
            std::cout << "Reading " << t.count << " record(s) from table " << t.code << "::" << t.name <<
                " (type: " << t.abi_type << ")" << std::endl;
            int i = 0;
            if (t.code == config::system_account_name) {
                bool is_accounts_tbl = t.name == N(account);
                while (i++ < t.count) {
                    sys_table_row r;
                    fc::raw::unpack(in, r);
                    EOS_ASSERT(r.data.size() >= 8, extract_genesis_exception, "System table row is too small");
                    if (!is_accounts_tbl || !update_account(r)) {
                        primary_key_t pk = ((primary_key_t*)r.data.data())[0]; // all system tables have pk in the 1st field
                        db.insert(r.request(t.name), ram_payer_info(r), pk, r.data.data(), r.data.size());
                    }
                    apply_db_changes();
                }
            } else {
                while (i++ < t.count) {
                    table_row r;
                    fc::raw::unpack(in, r);
                    db.insert(r.request(t.code, t.name), ram_payer_info(r), r.pk, r.data.data(), r.data.size());
                    apply_db_changes();
                }
            }
        }
        std::cout << "Done reading Genesis state." << std::endl;
        in.close();
        db.push_cache();
    }
};

genesis_import::genesis_import(const bfs::path& genesis_file, controller& ctrl)
: _impl(new impl(genesis_file, ctrl)) {
}
genesis_import::~genesis_import() {
}

void genesis_import::import() {
    _impl->import_state();
    _impl->apply_db_changes(true);
}


}} // cyberway::genesis
