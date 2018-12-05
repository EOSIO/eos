/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>

using namespace eosio;

namespace snapshot_test {

   struct [[eosio::contract("snapshot_test"), eosio::table]] main_record {
      uint64_t    id;
      double      index_f64  = 0.0;
      long double index_f128 = 0.0L;
      uint64_t    index_i64  = 0ULL;
      uint128_t   index_i128 = 0ULL;
      checksum256 index_i256 = checksum256();

      auto primary_key()const { return id; }

      auto get_index_f64()const  { return index_f64 ; }
      auto get_index_f128()const { return index_f128; }
      auto get_index_i64()const  { return index_i64 ; }
      auto get_index_i128()const { return index_i128; }
      const checksum256& get_index_i256 ()const { return index_i256; }

      EOSLIB_SERIALIZE( main_record, (id)(index_f64)(index_f128)(index_i64)(index_i128)(index_i256) )
   };

   struct [[eosio::contract("snapshot_test"), eosio::action]] increment {
      increment(): value(0) {}
      increment(uint32_t v): value(v) {}

      uint32_t value;

      EOSLIB_SERIALIZE( increment, (value) )
   };

   using multi_index_type = eosio::multi_index<"data"_n, main_record,
      indexed_by< "byf"_n,    const_mem_fun<main_record, double             ,&main_record::get_index_f64 >>,
      indexed_by< "byff"_n,   const_mem_fun<main_record, long double        ,&main_record::get_index_f128>>,
      indexed_by< "byi"_n,    const_mem_fun<main_record, uint64_t           ,&main_record::get_index_i64 >>,
      indexed_by< "byii"_n,   const_mem_fun<main_record, uint128_t          ,&main_record::get_index_i128>>,
      indexed_by< "byiiii"_n, const_mem_fun<main_record, const checksum256& ,&main_record::get_index_i256>>
      >;

   static void exec( uint64_t self, uint32_t value ) {
      multi_index_type data( name{self}, self );
      auto current = data.begin();
      if( current == data.end() ) {
         data.emplace( name{self}, [&]( auto& r ) {
            r.id         = value;
            r.index_f64  = value;
            r.index_f128 = value;
            r.index_i64  = value;
            r.index_i128 = value;
            r.index_i256.data()[0] = value;
         });

      } else {
         data.modify( current, name{self}, [&]( auto& r ) {
            r.index_f64  += value;
            r.index_f128 += value;
            r.index_i64  += value;
            r.index_i128 += value;
            r.index_i256.data()[0] += value;
         });
      }
   }

} /// multi_index_test

namespace multi_index_test {
   extern "C" {
      /// The apply method implements the dispatch of events to this contract
      void apply( uint64_t self, uint64_t code, uint64_t action ) {
         require_auth(code);
         eosio_assert( action == "increment"_n.value, "unsupported action" );
         snapshot_test::exec( self, unpack_action_data<snapshot_test::increment>().value );
      }
   }
}
