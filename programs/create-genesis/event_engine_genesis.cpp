#include "event_engine_genesis.hpp"

namespace cyberway { namespace genesis {

#define ABI_VERSION "cyberway::abi/1.0"

static abi_def create_usernames_abi() {
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
        "username_info", "", {
            {"creator", "name"},
            {"owner", "name"},
            {"name", "string"}
        }
    });

    return abi;

}

static abi_def create_balances_abi() {
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

event_engine_genesis::event_engine_genesis() {}
event_engine_genesis::~event_engine_genesis() {}

void event_engine_genesis::start(const bfp::path& ee_directory, const fc::sha256& hash) {
    usernames.start(ee_directory / "usernames.dat", hash, create_usernames_abi());
    balances.start(ee_directory / "balances.dat", hash, create_balances_abi());
}

void event_engine_genesis::finalize() {
    usernames.finalize();
    balances.finalize();
}


}} // cyberway::genesis
