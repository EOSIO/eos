#include "table_abi_test.hpp"

extern "C" {
   void init()  {

   }
   
   table1 get_table1()
   {
      // avoiding literal not being large enough to hold 128 bit uint
      uint128_t i128_tmp = 0xffffffffffffffff;
      i128_tmp += (uint128_t)(0xf);
      return table1 { 64, i128_tmp, -63 };
   }

   table2 get_table2()
   {
      // avoiding literal not being large enough to hold 128 bit uint
      uint128_t i128_tmp = 0xffffffffffffffff;
      i128_tmp += (uint128_t)(0xf0);
      return table2 { i128_tmp, ++i128_tmp, 98, -63 };
   }

   table3 get_table3()
   {
      // avoiding literal not being large enough to hold 128 bit uint
      uint128_t i128_tmp = 0xffffffffffffffff;
      i128_tmp += (uint128_t)(0xf00);
      return table3 { 64, -65, 66, i128_tmp, -63 };
   }

   struct string_table
   {
      char str[8] = "key str";
      char tmp[11] = "stored str";
   };

   string_table get_string_table()
   {
      return string_table();
   }

   void store_valid_i64(uint64_t code)
   {
      table1 tmp = get_table1();
      uint32_t err = store_i64( code, N(table1), &tmp, sizeof(tmp) );
      assert(err == 1, "should have successfully stored i64");

      table1 tmp2;
      tmp2.key = tmp.key;
      err = load_i64( code, code, N(table1), &tmp2, sizeof(tmp2) );
      assert(err == sizeof(tmp2), "should have successfully loaded i64");
      assert(tmp.key == tmp2.key, "should have retrieved same key");
      assert(tmp.value1 == tmp2.value1, "should have retrieved same value1");
      assert(tmp.value2 == tmp2.value2, "should have retrieved same value2");
   }

   void store_valid_i128i128(uint64_t code)
   {
      table2 tmp = get_table2();
      uint32_t err = store_i128i128( code, N(table2), &tmp, sizeof(tmp) );
      assert(err == 1, "should have successfully stored i128i128");

      table2 tmp2;
      tmp2.key1 = tmp.key1;
      tmp2.key2 = tmp.key2;
      err = load_primary_i128i128( code, code, N(table2), &tmp2, sizeof(tmp2) );
      assert(err == sizeof(tmp2), "should have successfully loaded i128i128");
      assert(tmp.key1 == tmp2.key1, "should have retrieved same key1");
      assert(tmp.key2 == tmp2.key2, "should have retrieved same key2");
      assert(tmp.value1 == tmp2.value1, "should have retrieved same value1");
      assert(tmp.value2 == tmp2.value2, "should have retrieved same value2");
   }

   void store_valid_i64i64i64(uint64_t code)
   {
      table3 tmp = get_table3();
      uint32_t err = store_i64i64i64( code, N(table3), &tmp, sizeof(tmp) );
      assert(err == 1, "should have successfully stored i64i64i64");

      table3 tmp2;
      tmp2.key1 = tmp.key1;
      tmp2.key2 = tmp.key2;
      tmp2.key3 = tmp.key3;
      err = load_primary_i64i64i64( code, code, N(table3), &tmp2, sizeof(tmp2) );
      assert(err == sizeof(tmp2), "should have successfully loaded i64i64i64");
      assert(tmp.key1 == tmp2.key1, "should have retrieved same key1");
      assert(tmp.key2 == tmp2.key2, "should have retrieved same key2");
      assert(tmp.key3 == tmp2.key3, "should have retrieved same key3");
      assert(tmp.value1 == tmp2.value1, "should have retrieved same value1");
      assert(tmp.value2 == tmp2.value2, "should have retrieved same value2");
   }

   void store_valid_str(uint64_t code)
   {
      string_table st = get_string_table();
      uint32_t err = store_str( code, N(strkey), st.str, sizeof(st.str), st.tmp, sizeof(st.tmp));
      assert(err == 1, "should have successfully stored str");

      char tmp[11] = {};
      err = load_str( code, code, N(strkey), st.str, sizeof(st.str), tmp, sizeof(tmp));
      for (uint32_t i = 0; i < 11; ++i)
         assert(st.tmp[i] == tmp[i], "should have loaded the same string that was stored");
   }

   void store_i64_table_as_str(uint64_t code)
   {
      char str[] = "key str";
      table1 tmp = get_table1();
      store_str( code, N(table1), str, sizeof(str), (char*)&tmp, sizeof(tmp));
   }

   void store_i128i128_table_as_i64(uint64_t code)
   {
      table2 tmp = get_table2();
      store_i64( code, N(table2), &tmp, sizeof(tmp) );
   }

   void store_i64i64i64_table_as_i128i128(uint64_t code)
   {
      table3 tmp = get_table3();
      store_i128i128( code, N(table3), &tmp, sizeof(tmp) );
   }

   void store_str_table_as_i64i64i64(uint64_t code)
   {
      table3 tmp = get_table3();
      store_i64i64i64( code, N(strkey), &tmp, sizeof(tmp) );
   }

   void load_i64_table_as_str(uint64_t code)
   {
      char str[] = "key str";
      table1 tmp = get_table1();
      load_str( code, code, N(table1), str, sizeof(str), (char*)&tmp, sizeof(tmp));
   }

   void load_i128i128_table_as_i64(uint64_t code)
   {
      table2 tmp = get_table2();
      load_i64( code, code, N(table2), &tmp, sizeof(tmp) );
   }

   void load_i64i64i64_table_as_i128i128(uint64_t code)
   {
      table3 tmp = get_table3();
      load_primary_i128i128( code, code, N(table3), &tmp, sizeof(tmp) );
   }

   void load_str_table_as_i64i64i64(uint64_t code)
   {
      table3 tmp = get_table3();
      load_primary_i64i64i64( code, code, N(strkey), &tmp, sizeof(tmp) );
   }

   void apply( uint64_t code, uint64_t action ) {
      if( code == N(storei) ) {
         if( action == N(transfer) ) {
            store_valid_i64(code);
            return;
         }
      } else if( code == N(storeii) ) {
         if( action == N(transfer) ) {
            store_valid_i128i128(code);
            return;
         }
      } else if( code == N(storeiii) ) {
         if( action == N(transfer) ) {
            store_valid_i64i64i64(code);
            return;
         }
      } else if( code == N(storestr) ) {
         if( action == N(transfer) ) {
            store_valid_str(code);
            return;
         }
      } else if( code == N(strnoti) ) {
         if( action == N(transfer) ) {
            store_i64_table_as_str(code);
            return;
         }
      } else if( code == N(inotii) ) {
         if( action == N(transfer) ) {
            store_i128i128_table_as_i64(code);
            return;
         }
      } else if( code == N(iinotiii) ) {
         if( action == N(transfer) ) {
            store_i64i64i64_table_as_i128i128(code);
            return;
         }
      } else if( code == N(iiinotstr) ) {
         if( action == N(transfer) ) {
            store_str_table_as_i64i64i64(code);
            return;
         }
      } else if( code == N(ldstrnoti) ) {
         if( action == N(transfer) ) {
            load_i64_table_as_str(code);
            return;
         }
      } else if( code == N(ldinotii) ) {
         if( action == N(transfer) ) {
            load_i128i128_table_as_i64(code);
            return;
         }
      } else if( code == N(ldiinotiii) ) {
         if( action == N(transfer) ) {
            load_i64i64i64_table_as_i128i128(code);
            return;
         }
      } else if( code == N(ldiiinotstr) ) {
         if( action == N(transfer) ) {
            load_str_table_as_i64i64i64(code);
            return;
         }
      }

      eosio::print("don't know code=", code, " action=", action, " \n");
      assert(0, "unknown code");
   }
}
