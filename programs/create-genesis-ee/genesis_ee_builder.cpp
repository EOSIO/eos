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

bool genesis_ee_builder::read_op_header(bfs::fstream& in, operation_header& op) {
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

    bfs::fstream in(in_dump_dir_ / "comments");
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

    bfs::fstream in(in_dump_dir_ / "delete_comments");
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

    bfs::fstream in(in_dump_dir_ / "total_comment_rewards");
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

    bfs::fstream in(in_dump_dir_ / "votes");
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

    bfs::fstream in(in_dump_dir_ / "reblogs");
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

    bfs::fstream in(in_dump_dir_ / "delete_reblogs");
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

    bfs::fstream in(in_dump_dir_ / "follows");
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

variants genesis_ee_builder::build_votes(uint64_t msg_hash, operation_number msg_created) {
    variants votes;

    const auto& vote_index = maps_.get_index<vote_header_index, by_hash_voter>();

    auto vote_itr = vote_index.lower_bound(msg_hash);
    for (; vote_itr != vote_index.end() && vote_itr->hash == msg_hash; ++vote_itr) {
        if (vote_itr->op_num < msg_created) {
            continue;
        }

        auto vote = mvo
            ("voter", generate_name(vote_itr->voter))
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
            ("account", generate_name(reblog_itr->account))
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

    for (const auto& comment : comment_index) {
        dump_comments.seekg(comment.offset);
        cyberway::golos::comment_operation cop;
        fc::raw::unpack(dump_comments, cop);

        auto votes = build_votes(comment.hash, comment.last_delete_op);

        auto reblogs = build_reblogs(comment.hash, comment.last_delete_op, dump_reblogs);

        out_.messages.insert(mvo
            ("parent_author", generate_name(cop.parent_author))
            ("parent_permlink", cop.parent_permlink)
            ("author", generate_name(cop.author))
            ("permlink", cop.permlink)
            ("title", cop.title)
            ("body", cop.body)
            ("tags", cop.tags)
            ("net_rshares", comment.net_rshares)
            ("author_reward", asset(comment.author_reward, symbol(GLS)))
            ("benefactor_reward", asset(comment.benefactor_reward, symbol(GLS)))
            ("curator_reward", asset(comment.curator_reward, symbol(GLS)))
            ("votes", votes)
            ("reblogs", reblogs)
        );
    }
}

void genesis_ee_builder::build_transfers() {
    std::cout << "-> Writing transfers..." << std::endl;

    out_.transfers.start_section(config::token_account_name, N(transfer), "transfer", 0);

    uint32_t transfer_count = 0;

    bfs::fstream in(in_dump_dir_ / "transfers");
    read_header(in);

    operation_header op;
    while (read_op_header(in, op)) {
        cyberway::golos::transfer_operation top;
        fc::raw::unpack(in, top);

        transfer_count++;

        out_.transfers.insert(mvo
            ("from", generate_name(top.from))
            ("to", generate_name(top.to))
            ("quantity", top.amount)
            ("memo", top.memo)
        );
    }

    out_.transfers.finish_section(transfer_count);
}

void genesis_ee_builder::build_pinblocks() {
    std::cout << "-> Writing pinblocks..." << std::endl;

    const auto& follow_index = maps_.get_index<follow_header_index, by_id>();

    out_.pinblocks.start_section(gls_social_account_name, N(pin), "pin", 0);

    uint32_t pin_count = 0;
    for (const auto& follow : follow_index) {
        if (follow.ignores) {
            continue;
        }
        auto pin = mvo
            ("pinner", generate_name(follow.follower))
            ("pinning", generate_name(follow.following));
        pin_count++;
        out_.pinblocks.insert(pin);
    }

    out_.pinblocks.finish_section(pin_count);

    out_.pinblocks.start_section(gls_social_account_name, N(block), "block", 0);

    uint32_t block_count = 0;
    for (const auto& follow : follow_index) {
        if (!follow.ignores) {
            continue;
        }
        auto block = mvo
            ("blocker", generate_name(follow.follower))
            ("blocking", generate_name(follow.following));
        block_count++;
        out_.pinblocks.insert(block);
    }

    out_.pinblocks.finish_section(block_count);
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
