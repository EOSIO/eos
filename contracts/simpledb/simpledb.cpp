#include "simpledb.hpp"

extern "C" {
   void init()  {

   }

   void apply( uint64_t code, uint64_t action ) {
      if( code == N(simpledb) ) {
         if( action == N(insert1) ) {
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
