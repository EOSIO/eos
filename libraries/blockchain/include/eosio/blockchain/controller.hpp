#pragma once
#include <eosio/blockchain/block.hpp>

namespace eosio { namespace blockchain {

   /**
    * @class controller
    *
    * This controller coordinates the asynchrounous process of deriving
    * the current chain state, logging blocks to disk, etc.
    *
    * All unconfirmed transactions are kept in a transaction cache where
    * information regarding signatures, pack size, id, and validity is cached. 
    */
   class controller {
      public:
         typedef std::function<void( const transaction_metadata_ptr& ) > completion_handler;

         controller( io_service& ios );
         ~controller();

         /**
          * Performs an asynchronous lookup of an existing known transaction and calls the
          * completion handler.
          */
         void lookup_transaction( transaction_id_type id, completion_handler );

         /**
          *  When a new transaction is received from any source this method will attempt
          *  to add it to our internal state of known transactions. If the transaction is already
          *  in the database then it may return a differetn transaction_metadata_ptr via the completion_handler
          */
         void add_transaction( transaction_metadata_ptr trx, completion_handler c = completion_handler()  );

         /**
          * Given a block summary, this method will reconstitute the block_data from the
          * known transactions after first validating that it links to known forks.
          */
         void add_block( block_summary_ptr summary );

         /**
          * Calling this method will cause the current "pending state" to be converted into a block
          * and the block_validated() signal to be generated. This process 
          */
         void produce_block( private_key_type producer_key );

         //void add_pre_commit( ... );
         //void add_pre_commit_receipt( ... );
         //void add_commit( ... );

         /**
          *  
          */
         ///{
         signal<void(transaction_metadata_ptr)> transaction_added;
         signal<void(transaction_metadata_ptr)> transaction_validated;
         signal<void(transaction_metadata_ptr)> transaction_confirmed;

         signal<void(block_data_ptr)>           block_linked; /// block linked, but not validated
         signal<void(block_data_ptr)>           block_validated; /// block linked and validated
         signal<void(block_data_ptr)>           block_confirmed; /// declared immutable 

         /*
         signal<void(precommit)>                new_precommit;
         signal<void(precommit_receipt)>        new_precommit_receipt;
         signal<void(commit)>                   new_commit;
         */
         ///}

      private:
         io_service&         _ios;
         unique_ptr<strand>  _strand;

         /// DATABASE
             // auth scope
             // per account scope
             // block scope
             
         /// FORKDB (track what fork each producer should be on based on messages received)
         /// BLOCK LOG (committed blocks)

   };

} } /// eosio::blockchain
