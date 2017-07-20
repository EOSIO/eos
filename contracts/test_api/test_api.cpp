#include "test_api.hpp"

#include "test_types.hpp"
#include "test_message.hpp"

extern "C" {

    void init()  {

    }

   void apply( unsigned long long code, unsigned long long action ) {
      
      //test_types
      WASM_TEST_HANDLER(test_types, 1);
      WASM_TEST_HANDLER(test_types, 2);
      WASM_TEST_HANDLER(test_types, 3);
      WASM_TEST_HANDLER(test_types, 4);
      
      //test_message
      WASM_TEST_HANDLER(test_message, 1);
      WASM_TEST_HANDLER(test_message, 2);
      WASM_TEST_HANDLER(test_message, 3);
      WASM_TEST_HANDLER(test_message, 4);
      WASM_TEST_HANDLER(test_message, 5);
      WASM_TEST_HANDLER(test_message, 6);
      WASM_TEST_HANDLER(test_message, 7);
      WASM_TEST_HANDLER(test_message, 8);
      WASM_TEST_HANDLER(test_message, 9);

      //test_print
      WASM_TEST_HANDLER(test_print, 1);
      WASM_TEST_HANDLER(test_print, 2);
      WASM_TEST_HANDLER(test_print, 3);
      WASM_TEST_HANDLER(test_print, 4);


      //test_math
   }

}
