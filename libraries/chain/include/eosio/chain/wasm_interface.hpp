#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>

namespace Runtime {
   struct MemoryInstance;
}

namespace eosio { namespace chain {

   class apply_context;
   class intrinsics_accessor;

   /**
    * @class wasm_cache
    *
    * This class manages compilation, re-use and memory sanitation for WASM contracts
    * As the same code can be running on many threads in parallel, some contracts will have multiple
    * copies.
    */
   class wasm_cache {
      public:
         wasm_cache();
         ~wasm_cache();

         /**
          * an opaque code entry used in the wasm_interface class
          */
         struct entry;

         /**
          * Checkout the desired code from the cache.  Code is idenfied via a digest of the wasm binary
          *
          * @param code_id - a digest of the wasm_binary bytes
          * @param wasm_binary - a pointer to the wasm_binary bytes
          * @param wasm_binary_size - the size of the wasm_binary bytes array
          * @return an entry which can be immediately used by the wasm_interface to execute contract code
          */
         entry &checkout( const digest_type& code_id, const char* wasm_binary, size_t wasm_binary_size );

         /**
          * Return an entry to the cache so that future checkouts may retrieve it
          *
          * @param code_id - a digest of the wasm_binary bytes
          * @param code - the entry which should be considered invalid post-call
          */
         void checkin( const digest_type& code_id, entry& code );

         /**
          * RAII wrapper to make sure that the cache entries are returned regardless of exceptions etc
          */
         struct scoped_entry {
            explicit scoped_entry(const digest_type& code_id, entry& code, wasm_cache& cache)
            :code_id(code_id)
            ,code(code)
            ,cache(cache)
            {}

            ~scoped_entry() {
               cache.checkin(code_id, code);
            }

            operator entry&() {
               return code;
            }

            digest_type code_id;
            entry&      code;
            wasm_cache& cache;
         };

         /**
          * Checkout the desired code from the cache.  Code is idenfied via a digest of the wasm binary
          * this method will wrap the code in an RAII construct so that it will automatically
          * return to the cache when it falls out of scope
          *
          * @param code_id - a digest of the wasm_binary bytes
          * @param wasm_binary - a pointer to the wasm_binary bytes
          * @param wasm_binary_size - the size of the wasm_binary bytes array
          * @return an entry which can be immediately used by the wasm_interface to execute contract code
          */
         scoped_entry checkout_scoped(const digest_type& code_id, const char* wasm_binary, size_t wasm_binary_size) {
            return scoped_entry(code_id, checkout(code_id, wasm_binary, wasm_binary_size), *this);
         }


      private:
         unique_ptr<struct wasm_cache_impl> _my;
   };

   /**
    * @class wasm_interface
    *
    * EOS.IO uses the wasm-jit library to evaluate web assembly code. This library relies
    * upon a singlton thread-local interface which means there can be only one instance 
    * per thread. 
    *
    */
   class wasm_interface {
      public:
         static wasm_interface& get();

          /**
          * Calls the init() method on the currently loaded code
          *
          * @param context - the interface by which the contract can interact
          * with blockchain state.
          */
         void init( wasm_cache::entry& code, apply_context& context );

         /**
          * Calls the apply() method on the currently loaded code
          *
          * @param context - the interface by which the contract can interact
          * with blockchain state.
          */
         void apply( wasm_cache::entry& code, apply_context& context );

         /**
          */
         void error( wasm_cache::entry& code, apply_context& context );

      private:
         wasm_interface();
         unique_ptr<struct wasm_interface_impl> my;
         friend class eosio::chain::intrinsics_accessor;
   };

} } // eosio::chain
