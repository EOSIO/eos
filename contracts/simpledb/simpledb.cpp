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

   void printhex( char* data, uint32_t datalen );

   void apply( uint64_t code, uint64_t action ) {
      if( code == N(simpledb) ) {
         if( action == N(insertalpha) ) {
            
            char tmp[512];
            auto len = readMessage(tmp, 512);

            printhex(tmp, len);

            char *ptr = tmp; 
            
            uint32_t key_len = (uint32_t)*ptr; ptr+=1;
            char* key        = ptr; ptr+=key_len;

            hack_printstr(key, key_len);

            char value_len   = (uint32_t)*ptr; ptr+=1;
            char* value      = ptr; ptr+=value_len;

            hack_printstr(value, value_len);

            store_str( N(simpledb), N(record1alpha), key, key_len, value, value_len );
         } else  if( action == N(insert1) ) {
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
