#include <cyberway/genesis/genesis_import.hpp>
#include <cyberway/genesis/genesis_container.hpp>
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
const fc::microseconds abi_serializer_max_time = fc::seconds(10);


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
            db.clear_cache();
        }
    }

    storage_payer_info ram_payer_info(const sys_table_row& row) {
        return resource_mng.get_storage_payer(row.ram_payer);
    };

    void import_state() {
        // file existance already checked when calculated hash
        std::cout << "Reading state from " << _state_file << "..." << std::endl;
        bfs::ifstream in(_state_file);
        genesis_header h{"", 0, 0};
        in.read((char*)&h, sizeof(h));
        std::cout << "Header magic: " << h.magic << "; ver: " << h.version << std::endl;
        EOS_ASSERT(h.is_valid(), extract_genesis_exception, "Unknown format of the Genesis state file.");

        bool abis_initialized = false;  // we don't want to deserialize golos contracts twice, so mark system
        abi_serializer sys_abi(eosio_contract_abi(), abi_serializer_max_time);
        while (in) {
            table_header t;
            fc::raw::unpack(in, t);
            if (!in)
                break;
            std::cout << "Reading " << t.count << " record(s) from table " << t.code << "::" << t.name <<
                " (type: " << t.abi_type << ")" << std::endl;
            int i = 0;
            if (t.code == config::system_account_name) {
                while (i++ < t.count) {
                    sys_table_row r;
                    fc::raw::unpack(in, r);
                    EOS_ASSERT(r.data.size() >= 8, extract_genesis_exception, "System table row is too small");
                    primary_key_t pk = ((primary_key_t*)r.data.data())[0]; // all system tables have pk in the 1st field
                    db.insert(r.request(t.name), ram_payer_info(r), pk, r.data.data(), r.data.size());
                    apply_db_changes();

                    // need to directly add abi to chaindb if exists
                    if (!abis_initialized && t.name == N(account)) {
                        fc::datastream<const char*> ds(static_cast<const char*>(r.data.data()), r.data.size());
                        auto acc = sys_abi.binary_to_variant("account_object", ds, abi_serializer_max_time);
                        auto abi_bytes = acc["abi"].as<bytes>();
                        if (abi_bytes.size() > 0) {
                            auto n = acc["name"].as<account_name>();
                            abi_def abi;
                            if (abi_serializer::to_abi(abi_bytes, abi) ) {
                                db.add_abi(n, abi);
                                std::cout << "  add " << n << " abi" << std::endl;
                            } else {
                                elog("Failed to read abi provided in ${a} contract", ("a",n));
                            }
                        }
                    }
                }
                if (t.name == N(account)) {
                    abis_initialized = true;
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
        db.clear_cache();
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


}} // cyberway::gensis
