#include <eosio/blockchain/generic_table.hpp>

namespace eosio { namespace blockchain {

map<uint16_t,abstract_table*>& get_abstract_table_map() {
   static map<uint16_t,abstract_table*> tbls;
   return tbls;
}

abstract_table& get_abstract_table( uint16_t table_type )
{
   const auto& tables = get_abstract_table_map();
   auto itr = tables.find(table_type);
   FC_ASSERT( itr != tables.end(), "unknown table type" );
   return *itr->second;
}

abstract_table& get_abstract_table( const table& t ) {
   return get_abstract_table( t._type );
}

} } /// eosio::blockchain
