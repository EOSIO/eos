#include "golos_objects.hpp"
#include "data_format.hpp"
#include <eosio/chain/config.hpp>
#include <fc/variant.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

namespace cw {

using namespace eosio::chain;
using namespace cyberway::chaindb;
namespace bfs = boost::filesystem;
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

struct converter {
    int  read_state();
    void set_program_options(options_description& cli);
    void initialize(const variables_map& options);

    bfs::path state_file;
    string chaindb_url;
    string mongo_db_prefix;
private:
    unique_ptr<chaindb_controller> _chaindb;
};

int converter::read_state() {
    if (bfs::exists(state_file)) {
        std::cout << "Reading state from " << state_file << "..." << std::endl;
        bfs::ifstream in(state_file);
    } else {
        std::cerr << "State file doesn't exist" << std::endl;
        return -1;
    }
    // TODO
    std::cout << "Done." << std::endl;
    return 0;
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
    auto d = options.at("state").as<bfs::path>();
    state_file = d.is_relative() ? bfs::current_path() / d : d;
    auto type = chaindb_type_by_url(chaindb_url);
    _chaindb.reset(new chaindb_controller(type, chaindb_url));  // TODO: pass mongo_db_prefix if MongoDB
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
        auto r = conv.read_state();
        if (r < 0)
            return r;
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
