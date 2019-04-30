#include "event_engine_genesis.hpp"

namespace cyberway { namespace genesis {

#define ABI_VERSION "cyberway::abi/1.0"

static abi_def create_messages_abi() {
    abi_def abi;
    abi.version = ABI_VERSION;

    abi.structs.emplace_back( struct_def {
        "vote_info", "", {
            {"voter", "name"},
            {"weight", "int16"},
            {"time", "time_point_sec"}
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
            {"title", "string"},
            {"body", "string"},
            {"tags", "string[]"},
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

void event_engine_genesis::start(const bfs::path& ee_directory, const fc::sha256& hash) {
    messages.start(ee_directory / "messages.dat", hash, create_messages_abi());
}

void event_engine_genesis::finalize() {
    messages.finalize();
}

} } // cyberway::genesis
