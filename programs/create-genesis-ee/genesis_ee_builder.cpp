#include "genesis_ee_builder.hpp"
#include "golos_operations.hpp"

#define MEGABYTE 1024*1024

// Comments:
// 8000000 * 144 = 1.2 GB
// +
// 8000000 * 144 * 5 (votes on comment) = 6.0 GB
// +
// 1000000 * 128 (reblogs on comment) = 0.2 GB
// Follows:
// 2300000 * 160 = 0.4 GB
#define MAP_FILE_SIZE uint64_t(20*1024)*MEGABYTE

namespace cyberway { namespace genesis {

static constexpr uint64_t gls_post_account_name = N(gls.publish);
static constexpr uint64_t gls_social_account_name = N(gls.social);

constexpr auto GLS = SY(3, GOLOS);

genesis_ee_builder::genesis_ee_builder(const std::string& shared_file, uint32_t last_block)
        : last_block_(last_block), maps_(shared_file, chainbase::database::read_write, MAP_FILE_SIZE) {
    maps_.add_index<comment_header_index>();
    maps_.add_index<vote_header_index>();
    maps_.add_index<reblog_header_index>();
    maps_.add_index<follow_header_index>();
}

genesis_ee_builder::~genesis_ee_builder() {
}

golos_dump_header genesis_ee_builder::read_header(bfs::ifstream& in) {
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

bool genesis_ee_builder::read_op_header(bfs::ifstream& in, operation_header& op) {
    fc::raw::unpack(in, op);

    if (!in) {
        return false;
    }

    if (op.num.first > last_block_) {
        return false;
    }
    return true;
}

void genesis_ee_builder::process_comments() {
    std::cout << "-> Reading comments..." << std::endl;

    const auto& comment_index = maps_.get_index<comment_header_index, by_hash>();

    bfs::ifstream in(in_dump_dir_ / "comments");
    read_header(in);

    operation_header op;
    while (read_op_header(in, op)) {
        auto comment_offset = uint64_t(in.tellg());

        cyberway::golos::comment_operation cop;
        fc::raw::unpack(in, cop);

        auto comment_itr = comment_index.find(op.hash);
        if (comment_itr != comment_index.end()) {
            maps_.modify(*comment_itr, [&](auto& comment) {
                comment.offset = comment_offset;
                comment.create_op = op.num;
            });
            continue;
        }

        maps_.create<comment_header>([&](auto& comment) {
            comment.hash = op.hash;
            comment.offset = comment_offset;
            comment.create_op = op.num;
        });

        comment_count_++;
    }
}

void genesis_ee_builder::process_delete_comments() {
    std::cout << "-> Reading comment deletions..." << std::endl;

    const auto& comment_index = maps_.get_index<comment_header_index, by_hash>();

    bfs::ifstream in(in_dump_dir_ / "delete_comments");
    read_header(in);

    operation_header op;
    while (read_op_header(in, op)) {
        auto comment_itr = comment_index.find(op.hash);
        if (op.num > comment_itr->create_op) {
            maps_.remove(*comment_itr);

            comment_count_--;
        } else {
            maps_.modify(*comment_itr, [&](auto& comment) {
                comment.last_delete_op = op.num;
            });
        }
    }
}

void genesis_ee_builder::process_rewards() {
    std::cout << "-> Reading rewards..." << std::endl;

    const auto& comment_index = maps_.get_index<comment_header_index, by_hash>();

    bfs::ifstream in(in_dump_dir_ / "total_comment_rewards");
    read_header(in);

    operation_header op;
    while (read_op_header(in, op)) {
        cyberway::golos::total_comment_reward_operation tcrop;
        fc::raw::unpack(in, tcrop);

        auto comment_itr = comment_index.find(op.hash);
        if (comment_itr != comment_index.end() && op.num > comment_itr->last_delete_op) {
            maps_.modify(*comment_itr, [&](auto& comment) {
                comment.author_reward += tcrop.author_reward.get_amount();
                comment.benefactor_reward += tcrop.benefactor_reward.get_amount();
                comment.curator_reward += tcrop.curator_reward.get_amount();
                comment.net_rshares = tcrop.net_rshares;
            });
        }
    }
}

void genesis_ee_builder::process_votes() {
    std::cout << "-> Reading votes..." << std::endl;

    const auto& vote_index = maps_.get_index<vote_header_index, by_hash_voter>();

    bfs::ifstream in(in_dump_dir_ / "votes");
    read_header(in);

    operation_header op;
    while (read_op_header(in, op)) {
        cyberway::golos::vote_operation vop;
        fc::raw::unpack(in, vop);

        auto vote_itr = vote_index.find(std::make_tuple(op.hash, vop.voter));
        if (vote_itr != vote_index.end()) {
            maps_.modify(*vote_itr, [&](auto& vote) {
                vote.op_num = op.num;
                vote.weight = vop.weight;
                vote.timestamp = vop.timestamp;
            });
            continue;
        }

        maps_.create<vote_header>([&](auto& vote) {
            vote.hash = op.hash;
            vote.voter = vop.voter;
            vote.op_num = op.num;
            vote.weight = vop.weight;
            vote.timestamp = vop.timestamp;
        });
    }
}

void genesis_ee_builder::process_reblogs() {
    std::cout << "-> Reading reblogs..." << std::endl;

    const auto& reblog_index = maps_.get_index<reblog_header_index, by_hash_account>();

    bfs::ifstream in(in_dump_dir_ / "reblogs");
    read_header(in);

    operation_header op;
    while (read_op_header(in, op)) {
        auto reblog_offset = uint64_t(in.tellg());

        cyberway::golos::reblog_operation rop;
        fc::raw::unpack(in, rop);

        auto reblog_itr = reblog_index.find(std::make_tuple(op.hash, rop.account));
        if (reblog_itr != reblog_index.end()) {
            maps_.modify(*reblog_itr, [&](auto& reblog) {
                reblog.op_num = op.num;
                reblog.offset = reblog_offset;
            });
            continue;
        }

        maps_.create<reblog_header>([&](auto& reblog) {
            reblog.hash = op.hash;
            reblog.account = rop.account;
            reblog.op_num = op.num;
            reblog.offset = reblog_offset;
        });
    }
}

void genesis_ee_builder::process_delete_reblogs() {
    std::cout << "-> Reading delete reblogs..." << std::endl;

    const auto& reblog_index = maps_.get_index<reblog_header_index, by_hash_account>();

    bfs::ifstream in(in_dump_dir_ / "delete_reblogs");
    read_header(in);

    operation_header op;
    while (read_op_header(in, op)) {
        cyberway::golos::delete_reblog_operation drop;
        fc::raw::unpack(in, drop);

        auto reblog_itr = reblog_index.find(std::make_tuple(op.hash, drop.account));
        if (op.num > reblog_itr->op_num) {
            maps_.remove(*reblog_itr);
        }
    }
}

void genesis_ee_builder::process_follows() {
    std::cout << "-> Reading follows..." << std::endl;

    const auto& follow_index = maps_.get_index<follow_header_index, by_pair>();

    bfs::ifstream in(in_dump_dir_ / "follows");
    read_header(in);

    operation_header op;
    while (read_op_header(in, op)) {
        cyberway::golos::follow_operation fop;
        fc::raw::unpack(in, fop);

        bool ignores = false;
        if (fop.what & (1 << ignore)) {
            ignores = true;
        }

        auto follow_itr = follow_index.find(std::make_tuple(fop.follower, fop.following));
        if (follow_itr != follow_index.end()) {
            if (fop.what == 0) {
                maps_.remove(*follow_itr);
                continue;
            }

            maps_.modify(*follow_itr, [&](auto& follow) {
                follow.ignores = ignores;
            });
            continue;
        }

        if (fop.what == 0) {
            continue;
        }

        maps_.create<follow_header>([&](auto& follow) {
            follow.follower = fop.follower;
            follow.following = fop.following;
            follow.ignores = ignores;
        });
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
    process_follows();
}

// TODO: Move to common library

account_name generate_name(string n) {
    // TODO: replace with better function
    // TODO: remove dots from result (+append trailing to length of 12 characters)
    uint64_t h = std::hash<std::string>()(n);
    return account_name(h & 0xFFFFFFFFFFFFFFF0);
}

void genesis_ee_builder::build_votes(std::vector<vote_info>& votes, uint64_t msg_hash, operation_number msg_created) {
    const auto& vote_index = maps_.get_index<vote_header_index, by_hash_voter>();

    auto vote_itr = vote_index.lower_bound(msg_hash);
    for (; vote_itr != vote_index.end() && vote_itr->hash == msg_hash; ++vote_itr) {
        if (vote_itr->op_num < msg_created) {
            continue;
        }

        votes.emplace_back([&](auto& v) {
            v.voter = generate_name(vote_itr->voter);
            v.weight = vote_itr->weight;
            v.time = vote_itr->timestamp;
        }, 0);
    }
}

void genesis_ee_builder::build_reblogs(std::vector<reblog_info>& reblogs, uint64_t msg_hash, operation_number msg_created, bfs::ifstream& dump_reblogs) {
    const auto& reblog_idx = maps_.get_index<reblog_header_index, by_hash_account>();

    auto reblog_itr = reblog_idx.lower_bound(msg_hash);
    for (; reblog_itr != reblog_idx.end() && reblog_itr->hash == msg_hash; ++reblog_itr) {
        if (reblog_itr->op_num < msg_created) {
            continue;
        }

        dump_reblogs.seekg(reblog_itr->offset);
        cyberway::golos::reblog_operation rop;
        fc::raw::unpack(dump_reblogs, rop);

        reblogs.emplace_back([&](auto& r) {
            r.account = generate_name(reblog_itr->account);
            r.title = rop.title;
            r.body = rop.body;
            r.time = rop.timestamp;
        }, 0);
    }

}

void genesis_ee_builder::build_messages() {
    std::cout << "-> Writing messages..." << std::endl;

    out_.messages.start_section(gls_post_account_name, N(message), "message_info");

    bfs::ifstream dump_comments(in_dump_dir_ / "comments");
    bfs::ifstream dump_reblogs(in_dump_dir_ / "reblogs");

    const auto& comment_index = maps_.get_index<comment_header_index, by_id>();

    for (const auto& comment : comment_index) {
        dump_comments.seekg(comment.offset);
        cyberway::golos::comment_operation cop;
        fc::raw::unpack(dump_comments, cop);

        out_.messages.emplace<comment_info>([&](auto& c) {
            c.parent_author = generate_name(cop.parent_author);
            c.parent_permlink = cop.parent_permlink;
            c.author = generate_name(cop.author);
            c.permlink = cop.permlink;
            c.title = cop.title;
            c.body = cop.body;
            c.tags = cop.tags;
            c.language = cop.language;
            c.net_rshares = comment.net_rshares;
            c.author_reward = asset(comment.author_reward, symbol(GLS));
            c.benefactor_reward = asset(comment.benefactor_reward, symbol(GLS));
            c.curator_reward = asset(comment.curator_reward, symbol(GLS));
            build_votes(c.votes, comment.hash, comment.last_delete_op);
            build_reblogs(c.reblogs, comment.hash, comment.last_delete_op, dump_reblogs);
        });
    }
}

void genesis_ee_builder::build_transfers() {
    std::cout << "-> Writing transfers..." << std::endl;

    out_.transfers.start_section(config::token_account_name, N(transfer), "transfer");

    uint32_t transfer_count = 0;

    bfs::ifstream in(in_dump_dir_ / "transfers");
    read_header(in);

    operation_header op;
    while (read_op_header(in, op)) {
        cyberway::golos::transfer_operation top;
        fc::raw::unpack(in, top);

        transfer_count++;

        out_.transfers.emplace<transfer_info>([&](auto& t) {
            t.from = generate_name(top.from);
            t.to = generate_name(top.to);
            t.quantity = top.amount;
            t.memo = top.memo;
        });
    }
}

void genesis_ee_builder::build_pinblocks() {
    std::cout << "-> Writing pinblocks..." << std::endl;

    const auto& follow_index = maps_.get_index<follow_header_index, by_id>();

    out_.pinblocks.start_section(gls_social_account_name, N(pin), "pin");

    for (const auto& follow : follow_index) {
        if (follow.ignores) {
            continue;
        }
        out_.pinblocks.emplace<pin_info>([&](auto& p) {
            p.pinner = generate_name(follow.follower);
            p.pinning = generate_name(follow.following);
        });
    }

    out_.pinblocks.start_section(gls_social_account_name, N(block), "block");

    for (const auto& follow : follow_index) {
        if (!follow.ignores) {
            continue;
        }
        out_.pinblocks.emplace<block_info>([&](auto& b) {
            b.blocker = generate_name(follow.follower);
            b.blocking = generate_name(follow.following);
        });
    }
}

void genesis_ee_builder::build(const bfs::path& out_dir) {
    std::cout << "Writing genesis to " << out_dir << "..." << std::endl;

    out_.start(out_dir, fc::sha256());

    build_messages();
    build_transfers();
    build_pinblocks();

    out_.finalize();
}

} } // cyberway::genesis
