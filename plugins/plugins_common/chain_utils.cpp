#include <eosio/plugins_common/chain_utils.hpp>

namespace eosio {

    std::function<fc::optional<chain::abi_serializer> (const chain::account_name&)> make_resolver(chain::chaindb_controller& chain_db, const fc::microseconds& max_serialization_time) {
       return [&](const chain::account_name &name) -> fc::optional<chain::abi_serializer> {
           const auto* accnt = chain_db.find<chain::account_object>(name);
           if (accnt != nullptr) {
              chain::abi_def abi;
              if (chain::abi_serializer::to_abi(accnt->abi, abi)) {
                 return chain::abi_serializer(abi, max_serialization_time);
              }
           }
           return fc::optional<chain::abi_serializer>();
        };
    }

} // namespace eosio
