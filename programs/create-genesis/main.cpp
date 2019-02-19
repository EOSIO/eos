#include <eosio/chain/config.hpp>
#include <fc/variant.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

using namespace eosio::chain;
namespace bfs = boost::filesystem;
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

struct converter {
    void read_state();
    void set_program_options(options_description& cli);
    void initialize(const variables_map& options);

    bfs::path state_dir;
};

void converter::read_state() {
    std::cout << "Reading state from " << state_dir << "..." << std::endl;
    // TODO
    std::cout << "Done." << std::endl;
}

void converter::set_program_options(options_description& cli) {
    cli.add_options()
        // TODO
        ("state-dir", bpo::value<bfs::path>()->default_value("blocks"),
            "the location of the state directory (absolute path or relative to the current directory)")
        ("help", "Print this help message and exit.")
        ;

}

void converter::initialize(const variables_map& options) { try {
    auto d = options.at("state-dir").as<bfs::path>();
    state_dir = d.is_relative() ? bfs::current_path() / d : d;
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
