#pragma once

#include <apifiny/apifiny.hpp>

class [[apifiny::contract]] test_api_multi_index : public apifiny::contract {
public:
   using apifiny::contract::contract;

   [[apifiny::action("s1g")]]
   void idx64_general();

   [[apifiny::action("s1store")]]
   void idx64_store_only();

   [[apifiny::action("s1check")]]
   void idx64_check_without_storing();

   [[apifiny::action("s1findfail1")]]
   void idx64_require_find_fail();

   [[apifiny::action("s1findfail2")]]
   void idx64_require_find_fail_with_msg();

   [[apifiny::action("s1findfail3")]]
   void idx64_require_find_sk_fail();

   [[apifiny::action("s1findfail4")]]
   void idx64_require_find_sk_fail_with_msg();

   [[apifiny::action("s1pkend")]]
   void idx64_pk_iterator_exceed_end();

   [[apifiny::action("s1skend")]]
   void idx64_sk_iterator_exceed_end();

   [[apifiny::action("s1pkbegin")]]
   void idx64_pk_iterator_exceed_begin();

   [[apifiny::action("s1skbegin")]]
   void idx64_sk_iterator_exceed_begin();

   [[apifiny::action("s1pkref")]]
   void idx64_pass_pk_ref_to_other_table();

   [[apifiny::action("s1skref")]]
   void idx64_pass_sk_ref_to_other_table();

   [[apifiny::action("s1pkitrto")]]
   void idx64_pass_pk_end_itr_to_iterator_to();

   [[apifiny::action("s1pkmodify")]]
   void idx64_pass_pk_end_itr_to_modify();

   [[apifiny::action("s1pkerase")]]
   void idx64_pass_pk_end_itr_to_erase();

   [[apifiny::action("s1skitrto")]]
   void idx64_pass_sk_end_itr_to_iterator_to();

   [[apifiny::action("s1skmodify")]]
   void idx64_pass_sk_end_itr_to_modify();

   [[apifiny::action("s1skerase")]]
   void idx64_pass_sk_end_itr_to_erase();

   [[apifiny::action("s1modpk")]]
   void idx64_modify_primary_key();

   [[apifiny::action("s1exhaustpk")]]
   void idx64_run_out_of_avl_pk();

   [[apifiny::action("s1skcache")]]
   void idx64_sk_cache_pk_lookup();

   [[apifiny::action("s1pkcache")]]
   void idx64_pk_cache_sk_lookup();

   [[apifiny::action("s2g")]]
   void idx128_general();

   [[apifiny::action("s2store")]]
   void idx128_store_only();

   [[apifiny::action("s2check")]]
   void idx128_check_without_storing();

   [[apifiny::action("s2autoinc")]]
   void idx128_autoincrement_test();

   [[apifiny::action("s2autoinc1")]]
   void idx128_autoincrement_test_part1();

   [[apifiny::action("s2autoinc2")]]
   void idx128_autoincrement_test_part2();

   [[apifiny::action("s3g")]]
   void idx256_general();

   [[apifiny::action("sdg")]]
   void idx_double_general();

   [[apifiny::action("sldg")]]
   void idx_long_double_general();

};
