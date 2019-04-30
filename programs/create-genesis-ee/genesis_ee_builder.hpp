#pragma once

#include "golos_dump_container.hpp"
#include "event_engine_genesis.hpp"
#include "map_objects.hpp"

#include <cyberway/genesis/ee_genesis_container.hpp>
#include <boost/filesystem.hpp>
#include <fc/exception/exception.hpp>
#include <chainbase/chainbase.hpp>

namespace cyberway { namespace genesis {

namespace bfs = boost::filesystem;
using mvo = fc::mutable_variant_object;

FC_DECLARE_EXCEPTION(genesis_exception, 9000000, "genesis create exception");

class genesis_ee_builder final {
public:
    genesis_ee_builder(const genesis_ee_builder&) = delete;
    genesis_ee_builder(const std::string& shared_file, uint32_t last_block);
    ~genesis_ee_builder();

    void read_operation_dump(const bfs::path& in_dump_dir);
    void build(const bfs::path& out_dir);
private:
    golos_dump_header read_header(bfs::fstream& in);
    bool read_op_num(bfs::fstream& in, operation_number& op_num);

    void process_comments();
    void process_delete_comments();
    void process_rewards();
    void process_votes();
    void process_reblogs();
    void process_delete_reblogs();

    variants build_votes(uint64_t msg_hash, operation_number msg_created);
    variants build_reblogs(uint64_t msg_hash, operation_number msg_created, bfs::fstream& dump_reblogs);
    void build_messages();

    bfs::path in_dump_dir_;
    event_engine_genesis out_;
    uint32_t last_block_;
    chainbase::database maps_;
    uint32_t comment_count_;
};

} } // cyberway::genesis
