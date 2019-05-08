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
    golos_dump_header read_header(bfs::ifstream& in);
    bool read_op_header(bfs::ifstream& in, operation_header& op);

    void process_comments();
    void process_delete_comments();
    void process_rewards();
    void process_votes();
    void process_reblogs();
    void process_delete_reblogs();
    void process_follows();

    void build_votes(std::vector<vote_info>& votes, uint64_t msg_hash, operation_number msg_created);
    void build_reblogs(std::vector<reblog_info>& reblogs, uint64_t msg_hash, operation_number msg_created, bfs::ifstream& dump_reblogs);
    void build_messages();
    void build_transfers();
    void build_pinblocks();

    bfs::path in_dump_dir_;
    event_engine_genesis out_;
    uint32_t last_block_;
    chainbase::database maps_;
    uint32_t comment_count_;
};

} } // cyberway::genesis
