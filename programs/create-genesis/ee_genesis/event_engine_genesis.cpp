#include "event_engine_genesis.hpp"

namespace cyberway { namespace genesis { namespace ee {

#define ABI_VERSION "cyberway::abi/1.0"

static abi_def create_messages_abi() {
    abi_def abi;
    abi.version = ABI_VERSION;

    abi.structs.emplace_back( struct_def {
        "vote_info", "", {
            {"voter", "name"},
            {"weight", "int16"},
            {"time", "time_point_sec"},
            {"rshares", "int64"}
        }
    });

    abi.structs.emplace_back( struct_def {
        "reblog_info", "", {
            {"account", "name"},
            {"title", "string"},
            {"body", "string"},
            {"time", "time_point_sec"}
        }
    });

    abi.structs.emplace_back( struct_def {
        "message_info", "", {
            {"parent_author", "name"},
            {"parent_permlink", "string"},
            {"author", "name"},
            {"permlink", "string"},
            {"created", "time_point_sec"},
            {"last_update", "time_point_sec"},
            {"title", "string"},
            {"body", "string"},
            {"tags", "string[]"},
            {"language", "string"},
            {"net_rshares", "int64"},
            {"author_reward", "asset"},
            {"benefactor_reward", "asset"},
            {"curator_reward", "asset"},
            {"votes", "vote_info[]"},
            {"reblogs", "reblog_info[]"},
        }
    });

    return abi;
}

static abi_def create_transfers_abi() {
    abi_def abi;
    abi.version = ABI_VERSION;

    abi.structs.emplace_back( struct_def {
        "transfer", "", {
            {"from", "name"},
            {"to", "name"},
            {"quantity", "asset"},
            {"memo", "string"},
            {"time", "time_point_sec"},
        }
    });

    return abi;
}

static abi_def create_balance_convert_abi() {
    abi_def abi;
    abi.version = ABI_VERSION;
    abi.structs.emplace_back( struct_def {
        "balance_convert", "", {
            {"owner",  "name"},
            {"amount", "asset"},
            {"memo",   "string"},
        }
    });
    return abi;
}

static abi_def create_pinblocks_abi() {
    abi_def abi;
    abi.version = ABI_VERSION;

    abi.structs.emplace_back( struct_def {
        "pin", "", {
            {"pinner", "name"},
            {"pinning", "name"},
        }
    });

    abi.structs.emplace_back( struct_def {
        "block", "", {
            {"blocker", "name"},
            {"blocking", "name"},
        }
    });

    return abi;
}

static abi_def create_accounts_abi() {
    abi_def abi;
    abi.version = ABI_VERSION;

    abi.structs.emplace_back( struct_def {
        "domain_info", "", {
            {"owner", "name"},
            {"linked_to", "name"},
            {"name", "string"}
        }
    });

    abi.structs.emplace_back( struct_def {
        "account_info", "", {
            {"creator", "name"},
            {"owner", "name"},
            {"name", "string"},
            {"created", "time_point_sec"},
            {"last_update", "time_point_sec"},
            {"reputation", "int64"},
            {"balance", "asset"},
            {"balance_in_sys", "asset"},
            {"vesting_shares", "asset"},
            {"json_metadata", "string"},
        }
    });

    return abi;
}

static abi_def create_funds_abi() {
    abi_def abi;
    abi.version = ABI_VERSION;

    abi.structs.emplace_back( struct_def {
        "currency_stats", "", {
            {"supply", "asset"},
            {"max_supply", "asset"},
            {"issuer", "name"}
        }
    });

    abi.structs.emplace_back( struct_def {
        "balance_event", "", {
            {"account", "name"},
            {"balance", "asset"},
            {"payments", "asset"}
        }
    });

    return abi;
}

void event_engine_genesis::start(const bfs::path& ee_directory, const fc::sha256& hash) {
    using ser_info = std::tuple<string, abi_def>;
    const std::map<ee_ser_type, ser_info> infos = {
        {ee_ser_type::messages,  {"messages.dat",   create_messages_abi()}},
        {ee_ser_type::transfers, {"transfers.dat",  create_transfers_abi()}},
        {ee_ser_type::pinblocks, {"pinblocks.dat",  create_pinblocks_abi()}},
        {ee_ser_type::accounts,  {"accounts.dat",   create_accounts_abi()}},
        {ee_ser_type::funds,     {"funds.dat",      create_funds_abi()}},
        {ee_ser_type::balance_conversions, {"balance_conversions.dat", create_balance_convert_abi()}}
    };
    for (const auto& i: infos) {
        const auto& info = i.second;
        serializers[i.first].start(ee_directory / std::get<0>(info), hash, std::get<1>(info));
    }
}

void event_engine_genesis::finalize() {
    for (auto& s: serializers) {
        s.second.finalize();
    }
}

} } } // cyberway::genesis::ee
