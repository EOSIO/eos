#pragma once

#include <eosio/eosio.hpp>

class [[eosio::contract]] test_api_db : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action("pg")]]
   void primary_i64_general();

   [[eosio::action("pl")]]
   void primary_i64_lowerbound();

   [[eosio::action("pu")]]
   void primary_i64_upperbound();

   [[eosio::action("s1g")]]
   void idx64_general();

   [[eosio::action("s1l")]]
   void idx64_lowerbound();

   [[eosio::action("s1u")]]
   void idx64_upperbound();

   [[eosio::action("tia")]]
   void test_invalid_access( eosio::name code, uint64_t val, uint32_t index, bool store );

   [[eosio::action("sdnancreate")]]
   void idx_double_nan_create_fail();

   [[eosio::action("sdnanmodify")]]
   void idx_double_nan_modify_fail();

   [[eosio::action("sdnanlookup")]]
   void idx_double_nan_lookup_fail( uint32_t lookup_type );

   [[eosio::action("sk32align")]]
   void misaligned_secondary_key256_tests();

};
