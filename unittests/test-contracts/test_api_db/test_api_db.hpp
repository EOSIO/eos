#pragma once

#include <apifiny/apifiny.hpp>

class [[apifiny::contract]] test_api_db : public apifiny::contract {
public:
   using apifiny::contract::contract;

   [[apifiny::action("pg")]]
   void primary_i64_general();

   [[apifiny::action("pl")]]
   void primary_i64_lowerbound();

   [[apifiny::action("pu")]]
   void primary_i64_upperbound();

   [[apifiny::action("s1g")]]
   void idx64_general();

   [[apifiny::action("s1l")]]
   void idx64_lowerbound();

   [[apifiny::action("s1u")]]
   void idx64_upperbound();

   [[apifiny::action("tia")]]
   void test_invalid_access( apifiny::name code, uint64_t val, uint32_t index, bool store );

   [[apifiny::action("sdnancreate")]]
   void idx_double_nan_create_fail();

   [[apifiny::action("sdnanmodify")]]
   void idx_double_nan_modify_fail();

   [[apifiny::action("sdnanlookup")]]
   void idx_double_nan_lookup_fail( uint32_t lookup_type );

   [[apifiny::action("sk32align")]]
   void misaligned_secondary_key256_tests();

};
