#include "genesis_ee_builder.hpp"
#include "golos_operations.hpp"

#define MEGABYTE 1024*1024

// Comments:
// 8000000 * 68 = 0.5 GB
// +
// 8000000 * 44 * 10 (votes on comment) = 4.0 GB
// +
// 1000000 * 46 = (reblogs on comment) = 0.1 GB
#define MAP_FILE_SIZE uint64_t(10*1024)*MEGABYTE

namespace cyberway { namespace genesis {

static constexpr uint64_t gls_post_account_name  = N(gls.publish);

constexpr auto GLS = SY(3, GOLOS);

genesis_ee_builder::genesis_ee_builder(const std::string& shared_file, uint32_t last_block)
        : last_block_(last_block), maps_(shared_file, chainbase::database::read_write, MAP_FILE_SIZE) {
    maps_.add_index<comment_header_index>();
    maps_.add_index<vote_header_index>();
    maps_.add_index<reblog_header_index>();
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

bool genesis_ee_builder::read_op_num(bfs::fstream& in, operation_number& op_num) {
    if (!in) {
        return false;
    }

    fc::raw::unpack(in, op_num);

    if (op_num.first > last_block_) {
        return false;
    }
    return true;
}

void genesis_ee_builder::process_comments() {
    std::cout << "-> Reading comments..." << std::endl;

    const auto& comment_index = maps_.get_index<comment_header_index, by_hash>();

    bfs::fstream in(in_dump_dir_ / "comments");
    read_header(in);

    operation_number op_num;
    while (read_op_num(in, op_num)) {
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

        comment_count_++;
    }
}

void genesis_ee_builder::process_delete_comments() {
    std::cout << "-> Reading comment deletions..." << std::endl;

    const auto& comment_index = maps_.get_index<comment_header_index, by_hash>();

    bfs::fstream in(in_dump_dir_ / "delete_comments");
    read_header(in);

    operation_number op_num;
    while (read_op_num(in, op_num)) {
        cyberway::golos::delete_comment_operation dcop;
        fc::raw::unpack(in, dcop);

        auto comment_itr = comment_index.find(dcop.hash);
        if (op_num > comment_itr->create_op) {
            maps_.remove(*comment_itr);

            comment_count_--;
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

    operation_number op_num;
    while (read_op_num(in, op_num)) {
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

    operation_number op_num;
    while (read_op_num(in, op_num)) {
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

void genesis_ee_builder::process_reblogs() {
    std::cout << "-> Reading reblogs..." << std::endl;

    const auto& reblog_index = maps_.get_index<reblog_header_index, by_hash_account>();

    bfs::fstream in(in_dump_dir_ / "reblogs");
    read_header(in);

    operation_number op_num;
    while (read_op_num(in, op_num)) {
        auto reblog_offset = uint64_t(in.tellg());

        cyberway::golos::reblog_operation rop;
        fc::raw::unpack(in, rop);

        auto reblog_itr = reblog_index.find(std::make_tuple(rop.hash, rop.account));
        if (reblog_itr != reblog_index.end()) {
            maps_.modify(*reblog_itr, [&](auto& reblog) {
                reblog.op_num = op_num;
                reblog.offset = reblog_offset;
            });
            continue;
        }

        maps_.create<reblog_header>([&](auto& reblog) {
            reblog.hash = rop.hash;
            reblog.account = rop.account;
            reblog.op_num = op_num;
            reblog.offset = reblog_offset;
        });
    }
}

void genesis_ee_builder::process_delete_reblogs() {
    std::cout << "-> Reading delete reblogs..." << std::endl;

    const auto& reblog_index = maps_.get_index<reblog_header_index, by_hash_account>();

    bfs::fstream in(in_dump_dir_ / "delete_reblogs");
    read_header(in);

    operation_number op_num;
    while (read_op_num(in, op_num)) {
        cyberway::golos::delete_reblog_operation drop;
        fc::raw::unpack(in, drop);

        auto reblog_itr = reblog_index.find(std::make_tuple(drop.hash, drop.account));
        if (op_num > reblog_itr->op_num) {
            maps_.remove(*reblog_itr);
        }
    }
}

void genesis_ee_builder::read_operation_dump(const bfs::path& in_dump_dir) {
    in_dump_dir_ = in_dump_dir;

    std::cout << "Reading operation dump from " << in_dump_dir_ << "..." << std::endl;

    comment_count_ = 0;

    process_comments();
    process_delete_comments();
    process_rewards();
    process_votes();
    process_reblogs();
    process_delete_reblogs();
}

// TODO: Move to common library

account_name generate_name(string n) {
    // TODO: replace with better function
    // TODO: remove dots from result (+append trailing to length of 12 characters)
    uint64_t h = std::hash<std::string>()(n);
    return account_name(h & 0xFFFFFFFFFFFFFFF0);
}

variants genesis_ee_builder::build_votes(uint64_t msg_hash, operation_number msg_created) {
    variants votes;

    const auto& vote_index = maps_.get_index<vote_header_index, by_hash_voter>();

    auto vote_itr = vote_index.lower_bound(msg_hash);
    for (; vote_itr != vote_index.end() && vote_itr->hash == msg_hash; ++vote_itr) {
        if (vote_itr->op_num < msg_created) {
            continue;
        }

        auto vote = mvo
            ("voter", generate_name(std::string(vote_itr->voter)))
            ("weight", vote_itr->weight)
            ("time", vote_itr->timestamp);

        votes.push_back(vote);
    }

    return votes;
}

variants genesis_ee_builder::build_reblogs(uint64_t msg_hash, operation_number msg_created, bfs::fstream& dump_reblogs) {
    variants reblogs;

    const auto& reblog_idx = maps_.get_index<reblog_header_index, by_hash_account>();

    auto reblog_itr = reblog_idx.lower_bound(msg_hash);
    for (; reblog_itr != reblog_idx.end() && reblog_itr->hash == msg_hash; ++reblog_itr) {
        if (reblog_itr->op_num < msg_created) {
            continue;
        }

        dump_reblogs.seekg(reblog_itr->offset);
        cyberway::golos::reblog_operation rop;
        fc::raw::unpack(dump_reblogs, rop);

        auto reblog = mvo
            ("account", generate_name(std::string(reblog_itr->account)))
            ("title", rop.title)
            ("body", rop.body)
            ("time", rop.timestamp);

        reblogs.push_back(reblog);
    }

    return reblogs;
}

void genesis_ee_builder::build_messages() {
    std::cout << "-> Writing messages..." << std::endl;

    out_.messages.start_section(gls_post_account_name, N(message), "message_info", comment_count_);

    bfs::fstream dump_comments(in_dump_dir_ / "comments");
    bfs::fstream dump_reblogs(in_dump_dir_ / "reblogs");

    const auto& comment_index = maps_.get_index<comment_header_index, by_id>();

    for (auto comment_itr = comment_index.begin(); comment_itr != comment_index.end(); ++comment_itr) {
        dump_comments.seekg(comment_itr->offset);
        cyberway::golos::comment_operation cop;
        fc::raw::unpack(dump_comments, cop);

        auto votes = build_votes(comment_itr->hash, comment_itr->last_delete_op);

        auto reblogs = build_reblogs(comment_itr->hash, comment_itr->last_delete_op, dump_reblogs);

        auto msg = mvo
            ("parent_author", generate_name(std::string(cop.parent_author)))
            ("parent_permlink", cop.parent_permlink)
            ("author", generate_name(std::string(cop.author)))
            ("permlink", cop.permlink)
            ("title", cop.title)
            ("body", cop.body)
            ("tags", cop.tags)
            ("net_rshares", comment_itr->net_rshares)
            ("author_reward", asset(comment_itr->author_reward, symbol(GLS)))
            ("benefactor_reward", asset(comment_itr->benefactor_reward, symbol(GLS)))
            ("curator_reward", asset(comment_itr->curator_reward, symbol(GLS)))
            ("votes", votes)
            ("reblogs", reblogs);

        out_.messages.insert(msg);
    }
}

void genesis_ee_builder::build(const bfs::path& out_dir) {
    std::cout << "Writing genesis to " << out_dir << "..." << std::endl;

    out_.start(out_dir, fc::sha256());

    build_messages();

    out_.finalize();
}

} } // cyberway::genesis
