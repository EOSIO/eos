#include "genesis_ee_builder.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem/path.hpp>
#include <fc/filesystem.hpp>

namespace cyberway { namespace genesis {

namespace bfs = boost::filesystem;
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

void make_dir_absolute(bfs::path& path, const string& title, bool dir_exists = true) {
    path = bfs::absolute(path);
    if (dir_exists) {
        EOS_ASSERT(fc::is_directory(path), genesis_exception,
            "${t} directory '${f}' does not exist.", ("t",title)("f", path.generic_string()));
    } else {
        if (fc::exists(path)) {
            EOS_ASSERT(fc::is_directory(path), genesis_exception,
                "${t} directory expected but '${f}' is a file.", ("t",title)("f", path.generic_string()));
        } else {
            fc::create_directories(path);
        }
    }
}

int main(int argc, char** argv) {
    fc::exception::enable_detailed_strace();
    std::ios::sync_with_stdio(false); // for potential performance boost

    bfs::path input_dir;
    bfs::path output_dir;

    options_description cli("create-genesis-ee command line options");
    try {
        cli.add_options() (
            "input-dir,i", bpo::value<bfs::path>(&input_dir)->default_value("/var/lib/golosd/witness_node_data_dir/operation_dump"),
            "operation dump dir from Golos (absolute path or relative to the current directory)."
        ) (
            "output-dir,e", bpo::value<bfs::path>(&output_dir)->default_value("events-genesis"),
            "the directory to write Event-Engine genesis data files to (absolute or relative path)."
        ) (
            "help,h",
            "Print this help message and exit."
        );

        variables_map vmap;
        bpo::store(bpo::parse_command_line(argc, argv, cli), vmap);
        bpo::notify(vmap);
        if (vmap.count("help") > 0) {
            cli.print(std::cerr);
            return 0;
        }

        make_dir_absolute(input_dir, "Operation dump", true);
        make_dir_absolute(output_dir, "Events", false);

        bfs::remove_all("shared_memory");

        genesis_ee_builder builder("shared_memory");
        builder.read_operation_dump(input_dir);
        builder.build(output_dir);

        bfs::remove_all("shared_memory");
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

} } // cyberway::genesis

int main(int argc, char** argv) {
    return cyberway::genesis::main(argc, argv);
}
