#include "simpledb.hpp"

extern "C" {
   void init()  {

   }
   
   void hack_printstr(char* str, uint32_t str_len) {
      char x = str[str_len];
      str[str_len] = 0;
      prints("-->[");
      prints(str);
      prints("]\n");
      str[str_len] = x;
   }

   void apply( uint64_t code, uint64_t action ) {
      if( code == N(simpledb) ) {
         if( action == N(insertkv1) ) {
            // eosc push message simpledb insertkv1 '{"key":"a", "value":"aa"}' -S simpledb
            // eosc get table simpledb simpledb keyvalue1
            char tmp[512];
            auto len = readMessage(tmp, 512);

            printhex(tmp, len);

            char *ptr = tmp; 
            
            uint32_t key_len = (uint32_t)*ptr; ptr+=1;
            char* key = ptr; ptr+=key_len;

            hack_printstr(key, key_len);

            char *str = ptr;
            hack_printstr(str+1, *str);

            uint32_t err = store_str( N(simpledb), N(keyvalue1), key, key_len, str, ((uint32_t)*str)+1);
         } else if( action == N(insertkv2) ) {
            // eosc push message simpledb insertkv2 '{"key":"a", "value":{"name":"aaa", "age":10}}' -S simpledb
            // eosc get table simpledb simpledb keyvalue2
            char tmp[512];
            auto len = readMessage(tmp, 512);

            printhex(tmp, len);

            char *ptr = tmp; 
            
            uint32_t key_len = (uint32_t)*ptr; ptr+=1;
            char* key = ptr; ptr+=key_len;

            hack_printstr(key, key_len);

            char* complex_data = ptr;
            uint32_t complex_data_len = len-(key_len+1);

            uint32_t err = store_str( N(simpledb), N(keyvalue2), key, key_len, complex_data, complex_data_len );

         } else if( action == N(insert1) ) {
            record1 tmp;
            readMessage(&tmp, sizeof(tmp));
            store_i64( N(simpledb), N(record1), &tmp, sizeof(tmp) );
         } else if(action == N(insert2)) {
            record2 tmp;
            readMessage(&tmp, sizeof(tmp));
            store_i128i128( N(simpledb), N(record2), &tmp, sizeof(tmp) );
         } else if(action == N(insert3)) {
            record3 tmp;
            readMessage(&tmp, sizeof(tmp));
            store_i64i64i64( N(simpledb), N(record3), &tmp, sizeof(tmp) );
         } else {
            assert(0, "unknown message");
         }
      }
   }
}
