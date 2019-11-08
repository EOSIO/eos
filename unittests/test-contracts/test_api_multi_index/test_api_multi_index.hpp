/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/eosio.hpp>

class [[eosio::contract]] test_api_multi_index : public eosio::contract {
public:
   using eosio::contract::contract;

   [[eosio::action("s1g")]]
   void idx64_general();

   [[eosio::action("s1store")]]
   void idx64_store_only();

   [[eosio::action("s1check")]]
   void idx64_check_without_storing();

   [[eosio::action("s1findfail1")]]
   void idx64_require_find_fail();

   [[eosio::action("s1findfail2")]]
   void idx64_require_find_fail_with_msg();

   [[eosio::action("s1findfail3")]]
   void idx64_require_find_sk_fail();

   [[eosio::action("s1findfail4")]]
   void idx64_require_find_sk_fail_with_msg();

   [[eosio::action("s1pkend")]]
   void idx64_pk_iterator_exceed_end();

   [[eosio::action("s1skend")]]
   void idx64_sk_iterator_exceed_end();

   [[eosio::action("s1pkbegin")]]
   void idx64_pk_iterator_exceed_begin();

   [[eosio::action("s1skbegin")]]
   void idx64_sk_iterator_exceed_begin();

   [[eosio::action("s1pkref")]]
   void idx64_pass_pk_ref_to_other_table();

   [[eosio::action("s1skref")]]
   void idx64_pass_sk_ref_to_other_table();

   [[eosio::action("s1pkitrto")]]
   void idx64_pass_pk_end_itr_to_iterator_to();

   [[eosio::action("s1pkmodify")]]
   void idx64_pass_pk_end_itr_to_modify();

   [[eosio::action("s1pkerase")]]
   void idx64_pass_pk_end_itr_to_erase();

   [[eosio::action("s1skitrto")]]
   void idx64_pass_sk_end_itr_to_iterator_to();

   [[eosio::action("s1skmodify")]]
   void idx64_pass_sk_end_itr_to_modify();

   [[eosio::action("s1skerase")]]
   void idx64_pass_sk_end_itr_to_erase();

   [[eosio::action("s1modpk")]]
   void idx64_modify_primary_key();

   [[eosio::action("s1exhaustpk")]]
   void idx64_run_out_of_avl_pk();

   [[eosio::action("s1skcache")]]
   void idx64_sk_cache_pk_lookup();

   [[eosio::action("s1pkcache")]]
   void idx64_pk_cache_sk_lookup();

   [[eosio::action("s2g")]]
   void idx128_general();

   [[eosio::action("s2store")]]
   void idx128_store_only();

   [[eosio::action("s2check")]]
   void idx128_check_without_storing();

   [[eosio::action("s2autoinc")]]
   void idx128_autoincrement_test();

   [[eosio::action("s2autoinc1")]]
   void idx128_autoincrement_test_part1();

   [[eosio::action("s2autoinc2")]]
   void idx128_autoincrement_test_part2();

   [[eosio::action("s3g")]]
   void idx256_general();

   [[eosio::action("sdg")]]
   void idx_double_general();

   [[eosio::action("sldg")]]
   void idx_long_double_general();

};
