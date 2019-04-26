#include "genesis_info.hpp"
#include "genesis_create.hpp"
#include <eosio/chain/abi_def.hpp>

#include <fc/io/raw.hpp>
#include <fc/io/json.hpp>
#include <fc/io/fstream.hpp>
#include <fc/variant.hpp>
#include <fc/filesystem.hpp>

// #include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem/path.hpp>


namespace bfs = boost::filesystem;

namespace fc {

void to_variant(const bfs::path& path, fc::variant& v) {
    to_variant(path.generic_string(), v);
}
void from_variant(const fc::variant& v, bfs::path& path) {
    std::string t;
    from_variant(v, t);
    path = t;
}

} // fc


namespace cyberway { namespace genesis {

using namespace eosio::chain;
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;
using std::string;


struct config_reader {
    void set_program_options(options_description& cli);
    void read_config(const variables_map& options);
    void read_contracts();

    bfs::path info_file;
    bfs::path out_file;
    bfs::path ee_file;

    genesis_info info;
    genesis_state genesis;
    contracts_map contracts;
};


void config_reader::set_program_options(options_description& cli) {
    cli.add_options()
        ("genesis-info,g", bpo::value<bfs::path>(&info_file)->default_value("genesis-info.json"),
            "the location of the genesis info file (absolute path or relative to the current directory).")
        ("output-file,o", bpo::value<bfs::path>(&out_file)->default_value("cyberway-genesis.dat"),
            "the file to write generic genesis data to (absolute or relative path).")
        ("ee-output-file,e", bpo::value<bfs::path>(&ee_file)->default_value("events-genesis.dat"),
            "the file to write Event-Engine genesis data to (absolute or relative path).")
        ("help,h", "Print this help message and exit.")
        ;
}

void make_absolute(bfs::path& path, const string& title, bool file_exists = true) {
    path = bfs::absolute(path);
    if (file_exists) {
        EOS_ASSERT(fc::is_regular_file(path), genesis_exception,
            "${t} file '${f}' does not exist.", ("t",title)("f", path.generic_string()));
    }
}

fc::sha256 check_hash(const genesis_info::file_hash& fh) {
    std::string str;
    auto path = bfs::absolute(fh.path);
    fc::read_file_contents(path, str);
    EOS_ASSERT(!str.empty(), genesis_exception, "file is empty: ${f}", ("f", path));
    bytes data(str.begin(), str.end());
    auto hash = fc::sha256::hash(&(data[0]), data.size());
    if (fh.hash != fc::sha256()) {
        EOS_ASSERT(hash == fh.hash, genesis_exception,
            "Hash mismatch: ${f}; ${provided} / ${calculated}", ("f", path)("provided",fh.hash)("calculated",hash));
    }
    return hash;
}

void read_contract(const genesis_info::account& acc, contract_abicode& abicode) {
    if (acc.abi) {
        auto fh = *acc.abi;
        check_hash(fh);
        auto abi_path = bfs::absolute(fh.path);
        abicode.abi = fc::raw::pack(fc::json::from_file(abi_path).as<abi_def>());
    }
    if (acc.code) {
        auto fh = *acc.code;
        abicode.code_hash = check_hash(fh);
        auto wasm_path = bfs::absolute(fh.path);
        std::string wasm;
        fc::read_file_contents(wasm_path, wasm);
        const string binary_wasm_header("\x00\x61\x73\x6d\x01\x00\x00\x00", 8);
        if (wasm.compare(0, 8, binary_wasm_header))
            std::cerr << "WARNING: " << wasm_path
                << " doesn't look like a binary WASM file. Trying anyways..." << std::endl;
        abicode.code = bytes(wasm.begin(), wasm.end());
    };
}

void config_reader::read_config(const variables_map& options) {
    ilog("Genesis: read config");
    make_absolute(info_file, "Info");
    make_absolute(out_file, "Output", false);
    make_absolute(ee_file, "Events", false);

    info = fc::json::from_file(info_file).as<genesis_info>();
    make_absolute(info.state_file, "Golos state");
    make_absolute(info.genesis_json, "Genesis json");
    genesis = fc::json::from_file(info.genesis_json).as<genesis_state>();
}

void config_reader::read_contracts() {
    ilog("Reading pre-configured accounts");
    for (const auto& acc: info.accounts) {
        ilog("  ${a} (${o} / ${v})...", ("a",acc.name)("o",acc.owner_key)("v",acc.active_key));
        auto& data = contracts[acc.name];
        read_contract(acc, data);
        ilog("    done: abi size: ${a}, code size: ${c}.", ("a",data.abi.size())("c",data.code.size()));
    }
}


int main(int argc, char** argv) {
    fc::exception::enable_detailed_strace();
    std::ios::sync_with_stdio(false); // for potential performance boost
    options_description cli("create-genesis command line options");
    try {
        config_reader cr;
        cr.set_program_options(cli);
        variables_map vmap;
        bpo::store(bpo::parse_command_line(argc, argv, cli), vmap);
        bpo::notify(vmap);
        if (vmap.count("help") > 0) {
            cli.print(std::cerr);
            return 0;
        }

        cr.read_config(vmap);
        cr.read_contracts();

        genesis_create builder{};
        builder.read_state(cr.info.state_file);
        builder.write_genesis(cr.out_file, cr.ee_file, cr.info, cr.genesis, cr.contracts);

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

}} // cyberway::genesis

int main(int argc, char** argv) {
    return cyberway::genesis::main(argc, argv);
}
