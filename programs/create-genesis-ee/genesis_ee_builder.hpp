#pragma once

#include "golos_dump_container.hpp"
#include "map_objects.hpp"

#include <boost/filesystem.hpp>
#include <fc/exception/exception.hpp>
#include <chainbase/chainbase.hpp>

namespace cyberway { namespace genesis {

namespace bfs = boost::filesystem;

FC_DECLARE_EXCEPTION(genesis_exception, 9000000, "genesis create exception");

class genesis_ee_builder final {
public:
    genesis_ee_builder(const genesis_ee_builder&) = delete;
    genesis_ee_builder(const std::string& shared_file);
    ~genesis_ee_builder();

    void read_operation_dump(const bfs::path& in_dump_dir);
    void build(const bfs::path& out_file);
private:
    bfs::fstream& read_header(golos_dump_header& h, bfs::fstream& in, uint32_t expected_version);

    void process_comments();
    void process_delete_comments();
    void process_rewards();

    void build_messages();

    bfs::path in_dump_dir_;
    bfs::ofstream out_;
    chainbase::database maps_;
};

} } // cyberway::genesis
