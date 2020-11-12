#include <eosio/eosio.hpp>

namespace legacydb {

inline constexpr auto account = "legacydb"_n;

struct legacydb_contract : eosio::contract {
   using eosio::contract::contract;

   void write();
   void read();
};
EOSIO_ACTIONS(legacydb_contract, account, write, read)

} // namespace legacydb
