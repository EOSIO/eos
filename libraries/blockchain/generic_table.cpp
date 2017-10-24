#include <eosio/blockchain/generic_table.hpp>

namespace eosio { namespace blockchain {

map<table_id_type,abstract_table*>& get_abstract_table_map() {
   static map<table_id_type,abstract_table*> tbls;
   return tbls;
}

abstract_table& get_abstract_table( table_id_type table_type )
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

namespace fc {
  void to_variant(const eosio::blockchain::table_name& c, fc::variant& v) { v = std::string(c.owner) + "/" + std::string(c.tbl); }
  void from_variant(const fc::variant& v, eosio::blockchain::table_name& check) { 
     const auto& str = v.get_string();
     auto pos = str.find( '/' );
     check.owner = str.substr( 0, pos );
     check.tbl   = str.substr( pos+1 );
  }
} // fc
