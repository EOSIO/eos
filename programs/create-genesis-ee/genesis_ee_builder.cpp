#include "genesis_ee_builder.hpp"
#include "golos_operations.hpp"

#define MEGABYTE 1024*1024

// Comments:
// 8000000 * 68 = 0.5 GB
// +
// 8000000 * 44 * 10 (votes on comment) = 4.0 GB
#define MAP_FILE_SIZE uint64_t(9*1024)*MEGABYTE

namespace cyberway { namespace genesis {

genesis_ee_builder::genesis_ee_builder(const std::string& shared_file)
        : maps_(shared_file, chainbase::database::read_write, MAP_FILE_SIZE) {
    maps_.add_index<comment_header_index>();
    maps_.add_index<vote_header_index>();
}

genesis_ee_builder::~genesis_ee_builder() {
}

golos_dump_header genesis_ee_builder::read_header(bfs::fstream& in) {
    golos_dump_header h;
    in.read((char*)&h, sizeof(h));
    if (in) {
        EOS_ASSERT(std::string(h.magic) == golos_dump_header::expected_magic, genesis_exception,
            "Unknown format of the operation dump file.");
        EOS_ASSERT(h.version == golos_dump_header::expected_version, genesis_exception,
            "Wrong version of the operation dump file.");
    }
    return h;
}

operation_number genesis_ee_builder::read_op_num(bfs::fstream& in) {
    operation_number op_num;
    fc::raw::unpack(in, op_num);
    return op_num;
}

void genesis_ee_builder::process_comments() {
    std::cout << "-> Reading comments..." << std::endl;

    const auto& comment_index = maps_.get_index<comment_header_index, by_hash>();

    bfs::fstream in(in_dump_dir_ / "comments");
    read_header(in);
    while (in) {
        auto op_num = read_op_num(in);

        auto comment_offset = uint64_t(in.tellg());

        cyberway::golos::comment_operation cop;
        fc::raw::unpack(in, cop);

        auto comment_itr = comment_index.find(cop.hash);
        if (comment_itr != comment_index.end()) {
            maps_.modify(*comment_itr, [&](auto& comment) {
                comment.offset = comment_offset;
                comment.create_op = op_num;
            });
            continue;
        }

        maps_.create<comment_header>([&](auto& comment) {
            comment.hash = cop.hash;
            comment.offset = comment_offset;
            comment.create_op = op_num;
        });
    }
}

void genesis_ee_builder::process_delete_comments() {
    std::cout << "-> Reading comment deletions..." << std::endl;

    const auto& comment_index = maps_.get_index<comment_header_index, by_hash>();

    bfs::fstream in(in_dump_dir_ / "delete_comments");
    read_header(in);
    while (in) {
        auto op_num = read_op_num(in);

        cyberway::golos::delete_comment_operation dcop;
        fc::raw::unpack(in, dcop);

        auto comment_itr = comment_index.find(dcop.hash);
        if (op_num > comment_itr->create_op) {
            maps_.remove(*comment_itr);
        } else {
            maps_.modify(*comment_itr, [&](auto& comment) {
                comment.last_delete_op = op_num;
            });
        }
    }
}

void genesis_ee_builder::process_rewards() {
    std::cout << "-> Reading rewards..." << std::endl;

    const auto& comment_index = maps_.get_index<comment_header_index, by_hash>();

    bfs::fstream in(in_dump_dir_ / "total_comment_rewards");
    read_header(in);
    while (in) {
        auto op_num = read_op_num(in);

        cyberway::golos::total_comment_reward_operation tcrop;
        fc::raw::unpack(in, tcrop);

        auto comment_itr = comment_index.find(tcrop.hash);
        if (comment_itr != comment_index.end() && op_num > comment_itr->last_delete_op) {
            maps_.modify(*comment_itr, [&](auto& comment) {
                comment.author_reward = tcrop.author_reward.get_amount();
                comment.benefactor_reward = tcrop.benefactor_reward.get_amount();
                comment.curator_reward = tcrop.curator_reward.get_amount();
                comment.net_rshares = tcrop.net_rshares;
            });
        }
    }
}

void genesis_ee_builder::process_votes() {
    std::cout << "-> Reading votes..." << std::endl;

    const auto& vote_index = maps_.get_index<vote_header_index, by_hash_voter>();

    bfs::fstream in(in_dump_dir_ / "votes");
    read_header(in);
    while (in) {
        auto op_num = read_op_num(in);

        cyberway::golos::vote_operation vop;
        fc::raw::unpack(in, vop);

        auto vote_itr = vote_index.find(std::make_tuple(vop.hash, vop.voter));
        if (vote_itr != vote_index.end()) {
            maps_.modify(*vote_itr, [&](auto& vote) {
                vote.op_num = op_num;
                vote.weight = vop.weight;
                vote.timestamp = vop.timestamp;
            });
            continue;
        }

        maps_.create<vote_header>([&](auto& vote) {
            vote.hash = vop.hash;
            vote.voter = vop.voter;
            vote.op_num = op_num;
            vote.weight = vop.weight;
            vote.timestamp = vop.timestamp;
        });
    }
}

void genesis_ee_builder::read_operation_dump(const bfs::path& in_dump_dir) {
    in_dump_dir_ = in_dump_dir;

    std::cout << "Reading operation dump from " << in_dump_dir_ << "..." << std::endl;

    process_comments();
    process_delete_comments();
    process_rewards();
    process_votes();
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

void genesis_ee_builder::build_votes(uint64_t msg_hash, operation_number msg_created, message_ee_object& msg) {

    const auto& vote_index = maps_.get_index<vote_header_index, by_hash_voter>();

    auto vote_itr = vote_index.lower_bound(msg_hash);
    for (; vote_itr != vote_index.end() && vote_itr->hash == msg_hash; ++vote_itr) {
        if (vote_itr->op_num < msg_created) {
            continue;
        }

        vote_ee_object vote;
        vote.voter = generate_name(std::string(vote_itr->voter));
        vote.weight = vote_itr->weight;
        vote.time = uint64_t(vote_itr->timestamp.sec_since_epoch()) * 1000000;

        msg.votes.push_back(vote);
    }
}

void genesis_ee_builder::build_messages() {
    std::cout << "-> Writing messages..." << std::endl;

    fc::raw::pack(out_, section_ee_header(section_ee_type::messages));

    bfs::fstream dump_comments(in_dump_dir_ / "comments");
    bfs::fstream dump_votes(in_dump_dir_ / "votes");

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

        build_votes(comment_itr->hash, comment_itr->last_delete_op, msg);

        fc::raw::pack(out_, msg);
    }
}

void genesis_ee_builder::build(const bfs::path& out_file) {
    std::cout << "Writing genesis to " << out_file << "..." << std::endl;

    out_.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    out_.open(out_file, std::ios_base::binary);

    ee_genesis_header hdr;
    out_.write((const char*)&hdr, sizeof(hdr));

    build_messages();
}

} } // cyberway::genesis
