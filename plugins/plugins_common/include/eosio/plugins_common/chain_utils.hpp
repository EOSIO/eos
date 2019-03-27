#pragma once

#include <functional>

#include <eosio/chain/controller.hpp>

namespace eosio {
     std::function<fc::optional<chain::abi_serializer> (const chain::account_name &)> make_resolver(chain::chaindb_controller& chain_db, const fc::microseconds& max_serialization_time);

} // namespace eosio
