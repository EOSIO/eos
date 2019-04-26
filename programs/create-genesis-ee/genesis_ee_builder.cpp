#include "genesis_ee_builder.hpp"
#include "golos_operations.hpp"
#include <cyberway/genesis/ee_genesis_container.hpp>

#define MEGABYTE 1024*1024

#define MAP_FILE_SIZE uint64_t(1.5*1024)*MEGABYTE

namespace cyberway { namespace genesis {

genesis_ee_builder::genesis_ee_builder(const std::string& shared_file)
        : maps_(shared_file, chainbase::database::read_write, MAP_FILE_SIZE) {
    maps_.add_index<comment_header_index>();
}

genesis_ee_builder::~genesis_ee_builder() {
}

bfs::fstream& genesis_ee_builder::read_header(golos_dump_header& h, bfs::fstream& in, uint32_t expected_version) {
    in.read((char*)&h, sizeof(h));
    if (in) {
        EOS_ASSERT(std::string(h.magic) == golos_dump_header::expected_magic && h.version == expected_version, genesis_exception,
            "Unknown format of the operation dump file.");
    }
    return in;
}

void genesis_ee_builder::process_comments() {
    std::cout << "-> Reading comments..." << std::endl;

    const auto& comment_index = maps_.get_index<comment_header_index, by_hash>();

    bfs::fstream in(in_dump_dir_ / "comments");

    golos_dump_header h;
    while (read_header(h, in, 1)) {
        uint64_t hash = 0;
        fc::raw::unpack(in, hash);

        auto comment_offset = uint64_t(in.tellg());

        cyberway::golos::comment_operation cop;
        fc::raw::unpack(in, cop);

        auto comment_itr = comment_index.find(hash);
        if (comment_itr != comment_index.end()) {
            maps_.modify(*comment_itr, [&](auto& comment) {
                comment.offset = comment_offset;
                comment.create_op = h.op;
            });
            continue;
        }

        maps_.create<comment_header>([&](auto& comment) {
            comment.hash = hash;
            comment.offset = comment_offset;
            comment.create_op = h.op;
        });
    }
}

void genesis_ee_builder::process_delete_comments() {
    std::cout << "-> Reading comment deletions..." << std::endl;

    const auto& comment_index = maps_.get_index<comment_header_index, by_hash>();

    bfs::fstream in(in_dump_dir_ / "delete_comments");

    golos_dump_header h;
    while (read_header(h, in, 1)) {
        uint64_t hash = 0;
        fc::raw::unpack(in, hash);

        auto comment_itr = comment_index.find(hash);
        if (h.op > comment_itr->create_op) {
            maps_.remove(*comment_itr);
        } else {
            maps_.modify(*comment_itr, [&](auto& comment) {
                comment.last_delete_op = h.op;
            });
        }
    }
}

void genesis_ee_builder::process_rewards() {
    std::cout << "-> Reading rewards..." << std::endl;

    const auto& comment_index = maps_.get_index<comment_header_index, by_hash>();

    bfs::fstream in(in_dump_dir_ / "total_comment_rewards");

    golos_dump_header h;
    while (read_header(h, in, 1)) {
        uint64_t hash = 0;
        fc::raw::unpack(in, hash);

        cyberway::golos::total_comment_reward_operation tcrop;
        fc::raw::unpack(in, tcrop);

        auto comment_itr = comment_index.find(hash);
        if (comment_itr != comment_index.end() && h.op > comment_itr->create_op) {
            maps_.modify(*comment_itr, [&](auto& comment) {
                comment.author_reward = tcrop.author_reward.get_amount();
                comment.benefactor_reward = tcrop.benefactor_reward.get_amount();
                comment.curator_reward = tcrop.curator_reward.get_amount();
                comment.net_rshares = tcrop.net_rshares;
            });
        }
    }
}

// TODO: Move to common library

account_name generate_name(string n) {
    // TODO: replace with better function
    // TODO: remove dots from result (+append trailing to length of 12 characters)
    uint64_t h = std::hash<std::string>()(n);
    return account_name(h & 0xFFFFFFFFFFFFFFF0);
}

asset golos2sys(const asset& golos) {
    return asset(golos.get_amount() * (10000/1000));
}

void genesis_ee_builder::build_messages() {
    std::cout << "-> Writing messages..." << std::endl;

    fc::raw::pack(out_, section_ee_header(section_ee_type::messages));

    bfs::fstream dump_comments(in_dump_dir_ / "comments");

    const auto& comment_index = maps_.get_index<comment_header_index, by_id>();
    for (auto comment_itr = comment_index.begin(); comment_itr != comment_index.end(); ++comment_itr) {
        dump_comments.seekg(comment_itr->offset);
        cyberway::golos::comment_operation cop;
        fc::raw::unpack(dump_comments, cop);

        message_ee_object msg;
        msg.author = generate_name(std::string(cop.author));
        msg.permlink = cop.permlink;
        msg.parent_author = generate_name(std::string(cop.parent_author));
        msg.parent_permlink = cop.parent_permlink;
        msg.title = cop.title;
        msg.body = cop.body;
        msg.tags = cop.tags;
        msg.net_rshares = comment_itr->net_rshares;
        msg.author_reward = golos2sys(asset(comment_itr->author_reward));
        msg.benefactor_reward = golos2sys(asset(comment_itr->benefactor_reward));
        msg.curator_reward = golos2sys(asset(comment_itr->curator_reward));
        fc::raw::pack(out_, msg);
    }
}

void genesis_ee_builder::read_operation_dump(const bfs::path& in_dump_dir) {
    in_dump_dir_ = in_dump_dir;

    std::cout << "Reading operation dump from " << in_dump_dir_ << "..." << std::endl;

    process_comments();
    process_delete_comments();
    process_rewards();
}

void genesis_ee_builder::build(const bfs::path& out_file) {
    std::cout << "Writing genesis to " << out_file << "..." << std::endl;

    out_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    out_.open(out_file, std::ios_base::binary);

    ee_genesis_header hdr;
    out_.write((const char*)&hdr, sizeof(ee_genesis_header));

    build_messages();
}

} } // cyberway::genesis
