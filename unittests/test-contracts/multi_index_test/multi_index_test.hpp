/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>

class [[eosio::contract]] multi_index_test : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action("test1")]]
   void test_128bit_secondary_index();

   [[eosio::action("test2")]]
   void test_256bit_secondary_index();

   struct [[eosio::table("orders")]] limit_order {
      uint64_t     id = 0;
      uint128_t    price = 0;
      uint64_t     expiration = 0;
      eosio::name  owner;

      uint64_t  primary_key()const    { return id; }
      uint64_t  get_expiration()const { return expiration; }
      uint128_t get_price()const      { return price; }

      EOSLIB_SERIALIZE( limit_order, (id)(price)(expiration)(owner) )
   };

   using orders_table = eosio::multi_index<"orders"_n, limit_order,
      eosio::indexed_by< "byexp"_n,   eosio::const_mem_fun<limit_order, uint64_t,  &limit_order::get_expiration> >,
      eosio::indexed_by< "byprice"_n, eosio::const_mem_fun<limit_order, uint128_t, &limit_order::get_price> >
   >;

   struct [[eosio::table("testkey")]] test_k256 {
      uint64_t            id = 0;
      eosio::checksum256  val;

      int64_t                   primary_key()const { return id; }
      const eosio::checksum256& get_val()const     { return val; }

      EOSLIB_SERIALIZE( test_k256, (id)(val) )
   };

   using k256_table = eosio::multi_index<"testkey"_n, test_k256,
      eosio::indexed_by< "byval"_n, eosio::const_mem_fun<test_k256, const eosio::checksum256&, &test_k256::get_val> >
   >;
};
