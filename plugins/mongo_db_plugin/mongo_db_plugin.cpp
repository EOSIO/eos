/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/mongo_db_plugin/mongo_db_plugin.hpp>
#include <eosio/chain/eosio_contract.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <boost/chrono.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <queue>

#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

namespace fc { class variant; }

namespace eosio {

using chain::account_name;
using chain::action_name;
using chain::block_id_type;
using chain::permission_name;
using chain::transaction;
using chain::signed_transaction;
using chain::signed_block;
using chain::block_trace;
using chain::transaction_id_type;
using chain::packed_transaction;

static appbase::abstract_plugin& _mongo_db_plugin = app().register_plugin<mongo_db_plugin>();

class mongo_db_plugin_impl {
public:
   mongo_db_plugin_impl();
   ~mongo_db_plugin_impl();

   fc::optional<boost::signals2::scoped_connection> accepted_block_connection;
   fc::optional<boost::signals2::scoped_connection> irreversible_block_connection;
   fc::optional<boost::signals2::scoped_connection> accepted_transaction_connection;
   fc::optional<boost::signals2::scoped_connection> applied_transaction_connection;

   void accepted_block( const chain::block_state_ptr& );
   void applied_irreversible_block(const chain::block_state_ptr&);
   void accepted_transaction(const chain::transaction_metadata_ptr&);
   void applied_transaction(const chain::transaction_trace_ptr&);
   void process_accepted_transaction(const chain::transaction_metadata_ptr&);
   void _process_accepted_transaction(const chain::transaction_metadata_ptr&);
   void process_applied_transaction(const chain::transaction_trace_ptr&);
   void _process_applied_transaction(const chain::transaction_trace_ptr&);
   void process_block(const chain::block_state_ptr&);
   void _process_block(const chain::block_state_ptr&);
   void process_irreversible_block(const chain::block_state_ptr&);
   void _process_irreversible_block(const chain::block_state_ptr&);

   void init();
   void wipe_database();

   static abi_def eos_abi; // cached for common use

   bool configured{false};
   bool wipe_database_on_startup{false};

   std::string db_name;
   mongocxx::instance mongo_inst;
   mongocxx::client mongo_conn;
   mongocxx::collection accounts;

   size_t queue_size = 0;
   size_t processed = 0;
   std::deque<chain::transaction_metadata_ptr> transaction_metadata_queue;
   std::deque<chain::transaction_metadata_ptr> transaction_metadata_process_queue;
   std::deque<chain::transaction_trace_ptr> transaction_trace_queue;
   std::deque<chain::transaction_trace_ptr> transaction_trace_process_queue;
   std::deque<chain::block_state_ptr> block_state_queue;
   std::deque<chain::block_state_ptr> block_state_process_queue;
   std::deque<chain::block_state_ptr> irreversible_block_state_queue;
   std::deque<chain::block_state_ptr> irreversible_block_state_process_queue;
   // transaction.id -> actions
   std::map<std::string, std::vector<chain::action>> reversible_actions;
   boost::mutex mtx;
   boost::condition_variable condition;
   boost::thread consume_thread;
   boost::atomic<bool> done{false};
   boost::atomic<bool> startup{true};
   fc::optional<chain::chain_id_type> chain_id;
   abi_serializer abi;

   void consume_blocks();

   static const account_name newaccount;
   static const account_name transfer;
   static const account_name setabi;

   static const std::string block_states_col;
   static const std::string blocks_col;
   static const std::string trans_col;
   static const std::string trans_traces_col;
   static const std::string actions_col;
   static const std::string accounts_col;
};

const account_name mongo_db_plugin_impl::newaccount = "newaccount";
const account_name mongo_db_plugin_impl::transfer = "transfer";
const account_name mongo_db_plugin_impl::setabi = "setabi";

const std::string mongo_db_plugin_impl::block_states_col = "block_states";
const std::string mongo_db_plugin_impl::blocks_col = "blocks";
const std::string mongo_db_plugin_impl::trans_col = "transactions";
const std::string mongo_db_plugin_impl::trans_traces_col = "transaction_traces";
const std::string mongo_db_plugin_impl::actions_col = "actions";
const std::string mongo_db_plugin_impl::accounts_col = "accounts";


void mongo_db_plugin_impl::accepted_transaction( const chain::transaction_metadata_ptr& t ) {
   try {
      if (startup) {
         // on startup we don't want to queue, instead push back on caller
         process_accepted_transaction(t);
      } else {
         boost::mutex::scoped_lock lock(mtx);
         if (transaction_metadata_queue.size() > queue_size) {
            lock.unlock();
            condition.notify_one();
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            lock.lock();
         }
         transaction_metadata_queue.emplace_back(t);
         lock.unlock();
         condition.notify_one();
      }
   } catch (fc::exception& e) {
      elog("FC Exception while accepted_transaction ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while accepted_transaction ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while accepted_transaction");
   }
}

void mongo_db_plugin_impl::applied_transaction( const chain::transaction_trace_ptr& t ) {
   try {
      if (startup) {
         // on startup we don't want to queue, instead push back on caller
         process_applied_transaction(t);
      } else {
         boost::mutex::scoped_lock lock(mtx);
         if (transaction_trace_queue.size() > queue_size) {
            lock.unlock();
            condition.notify_one();
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            lock.lock();
         }
         transaction_trace_queue.emplace_back(t);
         lock.unlock();
         condition.notify_one();
      }
   } catch (fc::exception& e) {
      elog("FC Exception while applied_transaction ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while applied_transaction ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while applied_transaction");
   }
}

void mongo_db_plugin_impl::applied_irreversible_block( const chain::block_state_ptr& bs ) {
   try {
      if (startup) {
         // on startup we don't want to queue, instead push back on caller
         process_irreversible_block(bs);
      } else {
         boost::mutex::scoped_lock lock(mtx);
         if (irreversible_block_state_queue.size() > queue_size) {
            lock.unlock();
            condition.notify_one();
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            lock.lock();
         }
         irreversible_block_state_queue.push_back(bs);
         lock.unlock();
         condition.notify_one();
      }
   } catch (fc::exception& e) {
      elog("FC Exception while applied_irreversible_block ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while applied_irreversible_block ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while applied_irreversible_block");
   }
}

void mongo_db_plugin_impl::accepted_block( const chain::block_state_ptr& bs ) {
   try {
      if (startup) {
         // on startup we don't want to queue, instead push back on caller
         process_block(bs);
      } else {
         boost::mutex::scoped_lock lock(mtx);
         if (block_state_queue.size() > queue_size) {
            lock.unlock();
            condition.notify_one();
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            lock.lock();
         }
         block_state_queue.emplace_back(bs);
         lock.unlock();
         condition.notify_one();
      }
   } catch (fc::exception& e) {
      elog("FC Exception while accepted_block ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while accepted_block ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while accepted_block");
   }
}

void mongo_db_plugin_impl::consume_blocks() {
   try {
      while (true) {
         boost::mutex::scoped_lock lock(mtx);
         while ( transaction_metadata_queue.empty() &&
                 transaction_trace_queue.empty() &&
                 block_state_queue.empty() &&
                 irreversible_block_state_queue.empty() &&
                 !done ) {
            condition.wait(lock);
         }

         // capture for processing
         size_t transaction_metadata_size = transaction_metadata_queue.size();
         if (transaction_metadata_size > 0) {
            transaction_metadata_process_queue = move(transaction_metadata_queue);
            transaction_metadata_queue.clear();
         }
         size_t transaction_trace_size = transaction_trace_queue.size();
         if (transaction_trace_size > 0) {
            transaction_trace_process_queue = move(transaction_trace_queue);
            transaction_trace_queue.clear();
         }
         size_t block_state_size = block_state_queue.size();
         if (block_state_size > 0) {
            block_state_process_queue = move(block_state_queue);
            block_state_queue.clear();
         }
         size_t irreversible_block_size = irreversible_block_state_queue.size();
         if (irreversible_block_size > 0) {
            irreversible_block_state_process_queue = move(irreversible_block_state_queue);
            irreversible_block_state_queue.clear();
         }

         lock.unlock();

         // warn if queue size greater than 75%
         if( transaction_metadata_size > (queue_size * 0.75) ||
             transaction_trace_size > (queue_size * 0.75) ||
             block_state_size > (queue_size * 0.75) ||
             irreversible_block_size > (queue_size * 0.75)) {
            wlog("queue size: ${q}", ("q", transaction_metadata_size + transaction_trace_size + block_state_size + irreversible_block_size));
         } else if (done) {
            ilog("draining queue, size: ${q}", ("q", transaction_metadata_size + transaction_trace_size + block_state_size + irreversible_block_size));
         }

         // process transactions
         while (!transaction_metadata_process_queue.empty()) {
            const auto& t = transaction_metadata_process_queue.front();
            process_accepted_transaction(t);
            transaction_metadata_process_queue.pop_front();
         }

         while (!transaction_trace_process_queue.empty()) {
            const auto& t = transaction_trace_process_queue.front();
            process_applied_transaction(t);
            transaction_trace_process_queue.pop_front();
         }

         // process blocks
         while (!block_state_process_queue.empty()) {
            const auto& bs = block_state_process_queue.front();
            process_block(bs);
            block_state_process_queue.pop_front();
         }

         // process irreversible blocks
         while (!irreversible_block_state_process_queue.empty()) {
            const auto& bs = irreversible_block_state_process_queue.front();
            process_irreversible_block(bs);
            irreversible_block_state_process_queue.pop_front();
         }

         if( transaction_metadata_size == 0 &&
             transaction_trace_size == 0 &&
             block_state_size == 0 &&
             irreversible_block_size == 0 &&
             done ) {
            break;
         }
      }
      ilog("mongo_db_plugin consume thread shutdown gracefully");
   } catch (fc::exception& e) {
      elog("FC Exception while consuming block ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while consuming block ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while consuming block");
   }
}

namespace {

   auto find_account(mongocxx::collection& accounts, const account_name& name) {
      using bsoncxx::builder::stream::document;
      document find_acc{};
      find_acc << "name" << name.to_string();
      auto account = accounts.find_one(find_acc.view());
      if (!account) {
         FC_THROW("Unable to find account ${n}", ("n", name));
      }
      return *account;
   }

   auto find_transaction(mongocxx::collection& trans, const string& id) {
      using bsoncxx::builder::stream::document;
      document find_trans{};
      find_trans << "trans_id" << id;
      return trans.find_one(find_trans.view());
   }

   auto find_block(mongocxx::collection& blocks, const string& id) {
      using bsoncxx::builder::stream::document;
      document find_block{};
      find_block << "block_id" << id;
      auto block = blocks.find_one(find_block.view());
      if (!block) {
         FC_THROW("Unable to find block ${id}", ("id", id));
      }
      return *block;
   }


   void update_account(bsoncxx::builder::basic::document& act_doc, mongocxx::collection& accounts, const chain::action& act) {
      using bsoncxx::builder::basic::kvp;
      using bsoncxx::builder::basic::make_document;
      using bsoncxx::builder::stream::document;
      using bsoncxx::builder::stream::open_document;
      using bsoncxx::builder::stream::close_document;
      using bsoncxx::builder::stream::finalize;
      using namespace bsoncxx::types;

      if (act.account != chain::config::system_account_name)
         return;

      if (act.name == mongo_db_plugin_impl::newaccount) {
         auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
         auto newaccount = act.data_as<chain::newaccount>();

         // create new account
         bsoncxx::builder::stream::document doc{};
         doc << "name" << newaccount.name.to_string()
             << "createdAt" << b_date{now};
         if (!accounts.insert_one(doc.view())) {
            elog("Failed to insert account ${n}", ("n", newaccount.name));
         }

      } else if (act.name == mongo_db_plugin_impl::setabi) {
         auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
         auto setabi = act.data_as<chain::setabi>();
         auto from_account = find_account(accounts, setabi.account);

         document update_from{};
         try {
            const abi_def& abi_def = fc::raw::unpack<chain::abi_def>( setabi.abi );
            const string json_str = fc::json::to_string( abi_def );

            // the original keys from document 'view' are kept, "data" here is not replaced by "data" of add_data
            act_doc.append( kvp( "data", make_document( kvp( "account", setabi.account.to_string()),
                                                        kvp( "abi_def", bsoncxx::from_json( json_str )))));

            update_from << "$set" << open_document
                        << "abi" << bsoncxx::from_json( json_str )
                        << "updatedAt" << b_date{now}
                        << close_document;

            if( !accounts.update_one( document{} << "_id" << from_account.view()["_id"].get_oid() << finalize,
                                      update_from.view())) {
               elog( "Failed to udpdate account ${n}", ("n", setabi.account));
            }
         } catch(fc::exception& e) {
            // if unable to unpack abi_def then just don't save the abi
            // users are not required to use abi_def as their abi
         }
      }
   }

   void add_data(bsoncxx::builder::basic::document& act_doc, mongocxx::collection& accounts, const chain::action& act) {
      using bsoncxx::builder::basic::kvp;
      try {
         update_account( act_doc, accounts, act );
      } catch (...) {
         ilog( "Unable to update account for ${s}::${n}", ("s", act.account)( "n", act.name ));
      }
      try {
         auto from_account = find_account( accounts, act.account );
         abi_def abi;
         if( from_account.view().find( "abi" ) != from_account.view().end()) {
            try {
               abi = fc::json::from_string( bsoncxx::to_json( from_account.view()["abi"].get_document())).as<abi_def>();
            } catch (...) {
               ilog( "Unable to convert account abi to abi_def for ${s}::${n}", ("s", act.account)( "n", act.name ));
            }
         }
         string json;
         try {
            abi_serializer abis;
            abis.set_abi( abi );
            auto v = abis.binary_to_variant( abis.get_action_type( act.name ), act.data );
            json = fc::json::to_string( v );

            const auto& value = bsoncxx::from_json( json );
            act_doc.append( kvp( "data", value ));
            return;
         } catch (std::exception& e) {
            elog( "Unable to convert EOS JSON to MongoDB JSON: ${e}", ("e", e.what()));
            elog( "  EOS JSON: ${j}", ("j", json));
         }
      } catch (fc::exception& e) {
         if( act.name != "onblock" ) {
            ilog( "Unable to convert action.data to ABI: ${s}::${n}, what: ${e}",
                  ("s", act.account)( "n", act.name )( "e", e.to_detail_string()));
         }
      } catch (std::exception& e) {
         ilog( "Unable to convert action.data to ABI: ${s}::${n}, std what: ${e}",
               ("s", act.account)( "n", act.name )( "e", e.what()));
      } catch (...) {
         ilog( "Unable to convert action.data to ABI: ${s}::${n}, unknown exception",
               ("s", act.account)( "n", act.name ));
      }
      // if anything went wrong just store raw hex_data
      act_doc.append( kvp( "hex_data", fc::variant( act.data ).as_string()));
   }

void verify_last_block(mongocxx::collection& blocks, const std::string& prev_block_id) {
     mongocxx::options::find opts;
     opts.sort(bsoncxx::from_json(R"xxx({ "_id" : -1 })xxx"));
     auto last_block = blocks.find_one({}, opts);
     if (!last_block) {
        FC_THROW("No blocks found in database");
     }
     const auto id = last_block->view()["block_id"].get_utf8().value.to_string();
     if (id != prev_block_id) {
        FC_THROW("Did not find expected block ${pid}, instead found ${id}", ("pid", prev_block_id)("id", id));
     }
  }

   void verify_no_blocks(mongocxx::collection& blocks) {
      if (blocks.count(bsoncxx::from_json("{}")) > 0) {
         FC_THROW("Existing blocks found in database");
      }
   }
}

void mongo_db_plugin_impl::process_accepted_transaction( const chain::transaction_metadata_ptr& t ) {
   try {
      _process_accepted_transaction(t);
   } catch (fc::exception& e) {
      elog("FC Exception while processing transaction metadata: ${e}", ("e", e.to_detail_string()));
   } catch (std::exception& e) {
      elog("STD Exception while processing tranasction metadata: ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while processing transaction metadata");
   }
}

void mongo_db_plugin_impl::process_applied_transaction( const chain::transaction_trace_ptr& t ) {
   try {
      _process_applied_transaction(t);
   } catch (fc::exception& e) {
      elog("FC Exception while processing transaction trace: ${e}", ("e", e.to_detail_string()));
   } catch (std::exception& e) {
      elog("STD Exception while processing tranasction trace: ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while processing transaction trace");
   }
}

void mongo_db_plugin_impl::process_irreversible_block(const chain::block_state_ptr& bs) {
  try {
     _process_irreversible_block(bs);
  } catch (fc::exception& e) {
     elog("FC Exception while processing block: ${e}", ("e", e.to_detail_string()));
  } catch (std::exception& e) {
     elog("STD Exception while processing block: ${e}", ("e", e.what()));
  } catch (...) {
     elog("Unknown exception while processing block");
  }
}

void mongo_db_plugin_impl::process_block(const chain::block_state_ptr& bs) {
   try {

      if (processed == 0) {
         auto blocks = mongo_conn[db_name][blocks_col]; // Blocks
         auto block_num = bs->block->block_num();
         if (wipe_database_on_startup) {
            // verify on start we have no previous blocks
            verify_no_blocks(blocks);
            FC_ASSERT(block_num == 2, "Expected start of block, instead received block_num: ${bn}", ("bn", block_num));
         } else {
            // verify on restart we have previous block
            const auto prev_block_id_str = bs->block->previous.str();
            verify_last_block(blocks, prev_block_id_str);
         }
      }

      _process_block(bs);

      ++processed;
   } catch (fc::exception& e) {
      elog("FC Exception while processing block trace ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while processing block trace ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while processing trace block");
   }
}

void mongo_db_plugin_impl::_process_accepted_transaction( const chain::transaction_metadata_ptr& t ) {
   using namespace bsoncxx::types;
   using bsoncxx::builder::basic::kvp;
   using bsoncxx::builder::basic::make_document;
   using bsoncxx::builder::basic::make_array;
   using bsoncxx::builder::basic::sub_array;

   auto trans = mongo_conn[db_name][trans_col];
   auto actions = mongo_conn[db_name][actions_col];
   accounts = mongo_conn[db_name][accounts_col];
   auto trans_doc = bsoncxx::builder::basic::document{};

   auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

   const auto trans_id = t->id;
   const auto trans_id_str = trans_id.str();
   const auto& trx = t->trx;
   const chain::transaction_header& trx_header = trx;

   const auto existing = find_transaction(trans, trans_id_str);

   bool actions_to_write = false;
   mongocxx::options::bulk_write bulk_opts;
   bulk_opts.ordered(false);
   mongocxx::bulk_write bulk_actions{bulk_opts};

   int32_t act_num = 0;
   auto process_action = [&](const std::string& trans_id_str, const chain::action& act, bsoncxx::builder::basic::array& act_array) -> auto {
      auto act_doc = bsoncxx::builder::basic::document();
      act_doc.append(kvp("action_num", b_int32{act_num}),
                     kvp("trans_id", trans_id_str));
      act_doc.append(kvp("account", act.account.to_string()));
      act_doc.append(kvp("name", act.name.to_string()));
      act_doc.append(kvp("authorization", [&act](bsoncxx::builder::basic::sub_array subarr) {
         for (const auto& auth : act.authorization) {
            subarr.append([&auth](bsoncxx::builder::basic::sub_document subdoc) {
               subdoc.append(kvp("actor", auth.actor.to_string()),
                             kvp("permission", auth.permission.to_string()));
            });
         }
      }));
      add_data(act_doc, accounts, act);
      act_array.append(act_doc);
      mongocxx::model::insert_one insert_op{act_doc.view()};
      bulk_actions.append(insert_op);
      ++act_num;
      actions_to_write = true;
      return act_num;
   };

   if (existing) {
      FC_ASSERT("Should only get accept transaction once.");
   } else {
      trans_doc.append( kvp( "trans_id", trans_id_str ),
                        kvp( "irreversible", b_bool{false} ) );

      string signing_keys_json;
      if( t->signing_keys.valid() ) {
         signing_keys_json = fc::json::to_string( t->signing_keys->second );
      } else {
         auto signing_keys = trx.get_signature_keys(*chain_id, false, false);
         if( !signing_keys.empty() ) {
            signing_keys_json = fc::json::to_string( signing_keys );
         }
      }
      string trx_header_json = fc::json::to_string( trx_header );

      try {
         const auto& trx_header_value = bsoncxx::from_json( trx_header_json );
         trans_doc.append( kvp( "transaction_header", trx_header_value ));
         if( !signing_keys_json.empty()) {
            const auto& keys_value = bsoncxx::from_json( signing_keys_json );
            trans_doc.append( kvp( "signing_keys", keys_value ));
         }
      } catch (std::exception& e) {
         elog( "Unable to convert transaction JSON to MongoDB JSON: ${e}", ("e", e.what()));
         elog( "  JSON: ${j}", ("j", trx_header_json));
         elog( "  JSON: ${j}", ("j", signing_keys_json));
      }

      if( !trx.actions.empty()) {
         bsoncxx::builder::basic::array action_array;
         for( const auto& act : trx.actions ) {
            process_action( trans_id_str, act, action_array );
         }
         trans_doc.append( kvp( "actions", action_array ) );
      }

      act_num = 0;
      if( !trx.context_free_actions.empty()) {
         bsoncxx::builder::basic::array action_array;
         for( const auto& cfa : trx.context_free_actions ) {
            process_action( trans_id_str, cfa, action_array );
         }
         trans_doc.append( kvp( "context_free_actions", action_array ) );
      }

      string trx_extensions_json = fc::json::to_string( trx.transaction_extensions );
      string trx_signatures_json = fc::json::to_string( trx.signatures );
      string trx_context_free_data_json = fc::json::to_string( trx.context_free_data );

      try {
         if (!trx_extensions_json.empty()) {
            const auto& trx_extensions_value = bsoncxx::from_json( trx_extensions_json );
            trans_doc.append( kvp( "transaction_extensions", trx_extensions_value ));
         } else {
            trans_doc.append( kvp( "transaction_extensions", make_array() ));
         }

         if (!trx_signatures_json.empty()) {
            const auto& trx_signatures_value = bsoncxx::from_json( trx_signatures_json );
            trans_doc.append( kvp( "signatures", trx_signatures_value ));
         } else {
            trans_doc.append( kvp( "signatures", make_array() ));
         }

         if (!trx_context_free_data_json.empty()) {
            const auto& trx_context_free_data_value = bsoncxx::from_json( trx_context_free_data_json );
            trans_doc.append( kvp( "context_free_data", trx_context_free_data_value ));
         } else {
            trans_doc.append( kvp( "context_free_data", make_array() ));
         }
      } catch (std::exception& e) {
         elog( "Unable to convert transaction JSON to MongoDB JSON: ${e}", ("e", e.what()));
         elog( "  JSON: ${j}", ("j", trx_extensions_json));
         elog( "  JSON: ${j}", ("j", trx_signatures_json));
         elog( "  JSON: ${j}", ("j", trx_context_free_data_json));
      }

      trans_doc.append( kvp( "createdAt", b_date{now} ));

      if( !trans.insert_one( trans_doc.view())) {
         elog( "Failed to insert trans ${id}", ("id", trans_id));
      }

      if (actions_to_write) {
         auto result = actions.bulk_write(bulk_actions);
         if (!result) {
            elog("Bulk actions insert failed for transaction: ${id}", ("id", trans_id_str));
         }
      }

   }
}

void mongo_db_plugin_impl::_process_applied_transaction( const chain::transaction_trace_ptr& t ) {
   using namespace bsoncxx::types;
   using bsoncxx::builder::basic::kvp;

   auto trans_traces = mongo_conn[db_name][trans_traces_col];
   auto trans_traces_doc = bsoncxx::builder::basic::document{};

   auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

   const auto trans_id = t->id;
   const auto trans_id_str = trans_id.str();

   const auto existing = find_transaction(trans_traces, trans_id_str);

   if (existing) {
      FC_ASSERT("Should only get applied transaction once.");
   } else {
      string json = fc::json::to_string( t );
      try {
         const auto& value = bsoncxx::from_json( json );
         trans_traces_doc.append( kvp( "trans_id", trans_id_str ),
                                  kvp( "trace", value ));
         trans_traces_doc.append( kvp( "createdAt", b_date{now} ));
      } catch (std::exception& e) {
         elog( "Unable to convert transaction JSON to MongoDB JSON: ${e}", ("e", e.what()));
         elog( "  JSON: ${j}", ("j", json));
      }

      if( !trans_traces.insert_one( trans_traces_doc.view())) {
         elog( "Failed to insert trans ${id}", ("id", trans_id));
      }
   }
}

void mongo_db_plugin_impl::_process_block(const chain::block_state_ptr& bs) {
   using namespace bsoncxx::types;
   using namespace bsoncxx::builder;
   using bsoncxx::builder::basic::kvp;

   mongocxx::options::bulk_write bulk_opts;
   bulk_opts.ordered(false);
   mongocxx::bulk_write bulk_trans{bulk_opts};

   auto block_states = mongo_conn[db_name][block_states_col];
   auto blocks = mongo_conn[db_name][blocks_col];
   auto trans = mongo_conn[db_name][trans_col];

   auto block_doc = bsoncxx::builder::basic::document{};
   auto block_state_doc = bsoncxx::builder::basic::document{};
   const auto block_id = bs->id;
   const auto block_id_str = block_id.str();
   const auto prev_block_id_str = bs->block->previous.str();
   auto block_num = bs->block->block_num();

   auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

   const chain::block_header_state& bhs = *bs;

   auto json = fc::json::to_string( bhs );
   try {
      const auto& value = bsoncxx::from_json(json);
      block_state_doc.append(kvp( "block_num", b_int32{static_cast<int32_t>(block_num)} ),
                             kvp( "block_id", block_id_str ),
                             kvp( "block_header_state", value ),
                             kvp( "validated", b_bool{bs->validated} ),
                             kvp( "in_current_chain", b_bool{bs->in_current_chain} ),
                             kvp( "createdAt", b_date{now} ));
   } catch (std::exception& e) {
      elog("Unable to convert block_state JSON to MongoDB JSON: ${e}", ("e", e.what()));
      elog("  JSON: ${j}", ("j", json));
   }

   if (!block_states.insert_one(block_state_doc.view())) {
      elog("Failed to insert block_state ${bid}", ("bid", block_id));
   }

   fc::variant pretty_output;
   abi_serializer::to_variant( *bs->block, pretty_output, [&]( account_name n ){ return optional<abi_serializer>(); });
   json = fc::json::to_string( pretty_output );

   try {
      const auto& value = bsoncxx::from_json(json);
      block_doc.append(kvp( "block_num", b_int32{static_cast<int32_t>(block_num)} ),
                       kvp( "block_id", block_id_str ),
                       kvp( "irreversible", b_bool{false} ),
                       kvp( "block", value ),
                       kvp( "createdAt", b_date{now} ));
   } catch (std::exception& e) {
      elog("Unable to convert block JSON to MongoDB JSON: ${e}", ("e", e.what()));
      elog("  JSON: ${j}", ("j", json));
   }

   if (!blocks.insert_one(block_doc.view())) {
      elog("Failed to insert block ${bid}", ("bid", block_id));
   }

/* todo
   int32_t msg_num = -1;
   bool actions_to_write = false;
   auto process_action = [&](const std::string& trans_id_str, mongocxx::bulk_write& bulk_msgs, const chain::action& msg) -> auto {
      auto msg_oid = bsoncxx::oid{};
      auto msg_doc = bsoncxx::builder::basic::document{};
      msg_doc.append(kvp("_id", b_oid{msg_oid}),
                     kvp("action_id", b_int32{msg_num}),
                     kvp("transaction_id", trans_id_str));
      msg_doc.append(kvp("authorization", [&msg](bsoncxx::builder::basic::sub_array subarr) {
         for (const auto& auth : msg.authorization) {
            subarr.append([&auth](bsoncxx::builder::basic::sub_document subdoc) {
               subdoc.append(kvp("actor", auth.actor.to_string()),
                             kvp("permission", auth.permission.to_string()));
            });
         }
      }));
      msg_doc.append(kvp("handler_account_name", msg.account.to_string()));
      msg_doc.append(kvp("name", msg.name.to_string()));
      add_data(msg_doc, accounts, msg);
      msg_doc.append(kvp("createdAt", b_date{now}));
      mongocxx::model::insert_one insert_msg{msg_doc.view()};
      bulk_msgs.append(insert_msg);
      actions_to_write = true;
      ++msg_num;
      return msg_oid;
   };

   bool action_traces_to_write = false;
   auto process_action_trace = [&](const std::string& trans_id_str,
                                   mongocxx::bulk_write& bulk_acts,
                                   const chain::action_trace& act,
                                   const auto& msg_oid)
   {
      auto act_oid = bsoncxx::oid{};
      auto act_doc = bsoncxx::builder::basic::document{};
      act_doc.append(kvp("_id", b_oid{act_oid}),
                     kvp("transaction_id", trans_id_str),
                     kvp("receiver", act.receipt.receiver.to_string()),
                     kvp("action", b_oid{msg_oid}),
                     kvp("console", act.console));
      act_doc.append(kvp("data_access", [&act](bsoncxx::builder::basic::sub_array subarr) {
         for (const auto& data : act.data_access) {
            subarr.append([&data](bsoncxx::builder::basic::sub_document subdoc) {
               subdoc.append(kvp("type", data.type == chain::data_access_info::read ? "read" : "write"),
                             kvp("code", data.code.to_string()),
                             kvp("scope", data.scope.to_string()),
                             kvp("sequence", b_int64{static_cast<int64_t>(data.sequence)}));
            });
         }
      }));
      act_doc.append(kvp("createdAt", b_date{now}));
      mongocxx::model::insert_one insert_act{act_doc.view()};
      bulk_acts.append(insert_act);
      action_traces_to_write = true;
   };

   int32_t trx_num = 0;
   std::map<chain::transaction_id_type, std::string> trx_status_map;
   bool transactions_in_block = false;

   auto process_trx = [&](const chain::transaction& trx) -> auto {
      auto txn_oid = bsoncxx::oid{};
      auto doc = bsoncxx::builder::basic::document{};
      auto trx_id = trx.id();
      const auto trans_id_str = trx_id.str();
      doc.append(kvp("_id", txn_oid),
                 kvp("transaction_id", trans_id_str),
                 kvp("sequence_num", b_int32{trx_num}),
                 kvp("block_id", block_id_str),
                 kvp("ref_block_num", b_int32{static_cast<int32_t >(trx.ref_block_num)}),
                 kvp("ref_block_prefix", b_int32{static_cast<int32_t >(trx.ref_block_prefix)}),
                 kvp("status", trx_status_map[trx_id]),
                 kvp("expiration",
                     b_date{std::chrono::milliseconds{std::chrono::seconds{trx.expiration.sec_since_epoch()}}}),
                 kvp("pending", b_bool{true})
      );
      doc.append(kvp("createdAt", b_date{now}));

      if (!trx.actions.empty()) {
         mongocxx::bulk_write bulk_msgs{bulk_opts};
         msg_num = 0;
         for (const auto& msg : trx.actions) {
            process_action(trans_id_str, bulk_msgs, msg);
         }
         auto result = msgs.bulk_write(bulk_msgs);
         if (!result) {
            elog("Bulk action insert failed for block: ${bid}, transaction: ${trx}",
                 ("bid", block_id)("trx", trx_id));
         }
      }
      transactions_in_block = true;
      return doc;
   };
*/
   mongocxx::bulk_write bulk_msgs{bulk_opts};
   mongocxx::bulk_write bulk_acts{bulk_opts};


   /* todo
   trx_num = 1000000;
   for (const auto& rt: bt.region_traces) {
      for (const auto& ct: rt.cycle_traces) {
         for (const auto& st: ct.shard_traces) {
            for (const auto& trx_trace: st.transaction_traces) {
               std::string trx_status = (trx_trace.status == chain::transaction_receipt::executed) ? "executed" :
                                        (trx_trace.status == chain::transaction_receipt::soft_fail) ? "soft_fail" :
                                        (trx_trace.status == chain::transaction_receipt::hard_fail) ? "hard_fail" :
                                        "unknown";
               trx_status_map[trx_trace.id] = trx_status;

               for (const auto& req : trx_trace.deferred_transaction_requests) {
                  if ( req.contains<chain::deferred_transaction>() ) {
                     auto trx = req.get<chain::deferred_transaction>();
                     auto doc = process_trx(trx);
                     doc.append(kvp("type", "deferred"),
                                kvp("sender_id", b_int64{static_cast<int64_t>(trx.sender_id)}),
                                kvp("sender", trx.sender.to_string()),
                                kvp("execute_after", b_date{std::chrono::milliseconds{
                                         std::chrono::seconds{trx.execute_after.sec_since_epoch()}}}));
                     mongocxx::model::insert_one insert_op{doc.view()};
                     bulk_trans.append(insert_op);
                     ++trx_num;
                  } else {
                     auto cancel = req.get<chain::deferred_reference>();
                     auto doc = bsoncxx::builder::basic::document{};
                     doc.append(kvp("type", "cancel_deferred"),
                                kvp("sender_id", b_int64{static_cast<int64_t>(cancel.sender_id)}),
                                kvp("sender", cancel.sender.to_string())
                     );
                  }
               }
               if (!trx_trace.action_traces.empty()) {
                  actions_to_write = true;
                  msg_num = 1000000;
                  for (const auto& act_trace : trx_trace.action_traces) {
                     const auto& msg = act_trace.act;
                     auto msg_oid = process_action(trx_trace.id.str(), bulk_msgs, msg);
                     if (trx_trace.status == chain::transaction_receipt::executed) {
                        if (act_trace.receiver == chain::config::system_account_name) {
                           reversible_actions[trx_trace.id.str()].emplace_back(msg);
                        }
                     }
                     process_action_trace(trx_trace.id.str(), bulk_acts, act_trace, msg_oid);
                  }
               }

               // TODO: handle canceled_deferred
            }
         }
      }
   }

   trx_num = 0;
   for (const auto& packed_trx : block.input_transactions) {
      const signed_transaction& trx = packed_trx.get_signed_transaction();
      auto doc = process_trx(trx);
      doc.append(kvp("type", "input"));
      doc.append(kvp("signatures", [&trx](bsoncxx::builder::basic::sub_array subarr) {
         for (const auto& sig : trx.signatures) {
            subarr.append(fc::variant(sig).as_string());
         }
      }));
      mongocxx::model::insert_one insert_op{doc.view()};
      bulk_trans.append(insert_op);
      ++trx_num;
   }

   for (const auto& implicit_trx : bt.implicit_transactions ){
      auto doc = process_trx(implicit_trx);
      doc.append(kvp("type", "implicit"));
      mongocxx::model::insert_one insert_op{doc.view()};
      bulk_trans.append(insert_op);
      ++trx_num;
   }

   if (actions_to_write) {
      auto result = msgs.bulk_write(bulk_msgs);
      if (!result) {
         elog("Bulk actions insert failed for block: ${bid}", ("bid", block_id));
      }
   }
   if (action_traces_to_write) {
      auto result = action_traces.bulk_write(bulk_acts);
      if (!result) {
         elog("Bulk action traces insert failed for block: ${bid}", ("bid", block_id));
      }
   }
   if (transactions_in_block) {
      auto result = trans.bulk_write(bulk_trans);
      if (!result) {
         elog("Bulk transaction insert failed for block: ${bid}", ("bid", block_id));
      }
   }
    */
}

void mongo_db_plugin_impl::_process_irreversible_block(const chain::block_state_ptr& bs)
{
   using namespace bsoncxx::types;
   using namespace bsoncxx::builder;
   using bsoncxx::builder::basic::make_document;
   using bsoncxx::builder::basic::kvp;

   auto blocks = mongo_conn[db_name][blocks_col]; // Blocks
   auto trans = mongo_conn[db_name][trans_col]; // Transactions

   const auto block_id = bs->block->id();
   const auto block_id_str = block_id.str();
   const auto block_num = bs->block->block_num();

   // genesis block 1 is not signaled to accepted_block
   if (block_num < 2) return;

   auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

   auto ir_block = find_block(blocks, block_id_str);

   auto update_doc = make_document( kvp( "$set", make_document( kvp( "irreversible", b_bool{true} ),
                                                                kvp( "validated", b_bool{bs->validated} ),
                                                                kvp( "in_current_chain", b_bool{bs->in_current_chain} ),
                                                                kvp( "updatedAt", b_date{now}))));

   blocks.update_one( make_document( kvp( "_id", ir_block.view()["_id"].get_oid())), update_doc.view());

   bool transactions_in_block = false;
   mongocxx::options::bulk_write bulk_opts;
   bulk_opts.ordered(false);
   auto bulk = trans.create_bulk_write(bulk_opts);

   for (const auto& receipt : bs->block->transactions) {
      string trans_id_str;
      if( receipt.trx.contains<packed_transaction>()) {
         const auto& pt = receipt.trx.get<packed_transaction>();
         // get id via get_raw_transaction() as packed_transaction.id() mutates inernal transaction state
         const auto& raw = pt.get_raw_transaction();
         const auto& id = fc::raw::unpack<transaction>(raw).id();
         trans_id_str = id.str();
      } else {
         const auto& id = receipt.trx.get<transaction_id_type>();
         trans_id_str = id.str();
      }

      auto ir_trans = find_transaction(trans, trans_id_str);

      if (ir_trans) {
         auto update_doc = make_document( kvp( "$set", make_document( kvp( "irreversible", b_bool{true} ),
                                                                      kvp( "block_id", block_id_str),
                                                                      kvp( "block_num", b_int32{static_cast<int32_t>(block_num)} ),
                                                                      kvp( "updatedAt", b_date{now}))));

         mongocxx::model::update_one update_op{ make_document(kvp("_id", ir_trans->view()["_id"].get_oid())), update_doc.view()};
         bulk.append(update_op);

         transactions_in_block = true;
      }
   }

   if (transactions_in_block) {
      auto result = trans.bulk_write(bulk);
      if (!result) {
         elog("Bulk transaction insert failed for block: ${bid}", ("bid", block_id));
      }
   }


/* todo
   for (const auto& r: block.regions) {
      for (const auto& cs: r.cycles_summary) {
         for (const auto& ss: cs) {
            for (const auto& trx_receipt: ss.transactions) {
               const auto trans_id_str = trx_receipt.id.str();
               auto ir_trans = find_transaction(trans, trans_id_str);

               document update_trans{};
               update_trans << "$set" << open_document << "pending" << b_bool{false}
                            << "updatedAt" << b_date{now}
                            << close_document;

               trans.update_one(document{} << "_id" << ir_trans.view()["_id"].get_oid() << finalize,
                                update_trans.view());

               // actions are irreversible, so update account document
               if (ir_trans.view()["status"].get_utf8().value.to_string() == "executed") {
                  for (const auto& msg : reversible_actions[trans_id_str]) {
                     update_account(msg);
                  }
               }
               reversible_actions.erase(trans_id_str);
            }
         }
      }
   }
*/
}

mongo_db_plugin_impl::mongo_db_plugin_impl()
: mongo_inst{}
, mongo_conn{}
{
}

mongo_db_plugin_impl::~mongo_db_plugin_impl() {
   try {
      done = true;
      condition.notify_one();

      consume_thread.join();
   } catch (std::exception& e) {
      elog("Exception on mongo_db_plugin shutdown of consume thread: ${e}", ("e", e.what()));
   }
}

void mongo_db_plugin_impl::wipe_database() {
   ilog("mongo db wipe_database");

   auto block_states = mongo_conn[db_name][block_states_col];
   auto blocks = mongo_conn[db_name][blocks_col];
   auto trans = mongo_conn[db_name][trans_col];
   auto trans_traces = mongo_conn[db_name][trans_traces_col];
   auto actions = mongo_conn[db_name][actions_col];
   accounts = mongo_conn[db_name][accounts_col];

   block_states.drop();
   blocks.drop();
   trans.drop();
   trans_traces.drop();
   actions.drop();
   accounts.drop();
}

void mongo_db_plugin_impl::init() {
   using namespace bsoncxx::types;
   // Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   // See native_contract_chain_initializer::prepare_database()

   accounts = mongo_conn[db_name][accounts_col];
   bsoncxx::builder::stream::document doc{};
   if (accounts.count(doc.view()) == 0) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      doc << "name" << name(chain::config::system_account_name).to_string()
          << "createdAt" << b_date{now};

      if (!accounts.insert_one(doc.view())) {
         elog("Failed to insert account ${n}", ("n", name(chain::config::system_account_name).to_string()));
      }

      // blocks indexes
      auto blocks = mongo_conn[db_name][blocks_col]; // Blocks
      blocks.create_index(bsoncxx::from_json(R"xxx({ "block_num" : 1 })xxx"));
      blocks.create_index(bsoncxx::from_json(R"xxx({ "block_id" : 1 })xxx"));

      // accounts indexes
      accounts.create_index(bsoncxx::from_json(R"xxx({ "name" : 1 })xxx"));

      // transactions indexes
      auto trans = mongo_conn[db_name][trans_col]; // Transactions
      trans.create_index(bsoncxx::from_json(R"xxx({ "trans_id" : 1 })xxx"));

      auto actions = mongo_conn[db_name][actions_col];
      actions.create_index(bsoncxx::from_json(R"xxx({ "trans_id" : 1 })xxx"));
   }
}

////////////
// mongo_db_plugin
////////////

mongo_db_plugin::mongo_db_plugin()
:my(new mongo_db_plugin_impl)
{
}

mongo_db_plugin::~mongo_db_plugin()
{
}

void mongo_db_plugin::set_program_options(options_description& cli, options_description& cfg)
{
   cfg.add_options()
         ("mongodb-queue-size,q", bpo::value<uint>()->default_value(256),
         "The queue size between nodeos and MongoDB plugin thread.")
         ("mongodb-uri,m", bpo::value<std::string>(),
         "MongoDB URI connection string, see: https://docs.mongodb.com/master/reference/connection-string/."
               " If not specified then plugin is disabled. Default database 'EOS' is used if not specified in URI.")
         ;
}

void mongo_db_plugin::plugin_initialize(const variables_map& options)
{
   if (options.count("mongodb-uri")) {
      ilog("initializing mongo_db_plugin");
      my->configured = true;

      if (options.at("replay-blockchain").as<bool>() || options.at("hard-replay-blockchain").as<bool>()) {
         ilog("Replay requested: wiping mongo database on startup");
         my->wipe_database_on_startup = true;
      }
      if (options.at("delete-all-blocks").as<bool>()) {
         ilog("Resync requested: wiping mongo database on startup");
         my->wipe_database_on_startup = true;
      }

      if (options.count("mongodb-queue-size")) {
         auto size = options.at("mongodb-queue-size").as<uint>();
         my->queue_size = size;
      }

      std::string uri_str = options.at("mongodb-uri").as<std::string>();
      ilog("connecting to ${u}", ("u", uri_str));
      mongocxx::uri uri = mongocxx::uri{uri_str};
      my->db_name = uri.database();
      if (my->db_name.empty())
         my->db_name = "EOS";
      my->mongo_conn = mongocxx::client{uri};

      // hook up to signals on controller
      chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
      FC_ASSERT(chain_plug);
      auto& chain = chain_plug->chain();
      my->chain_id.emplace(chain.get_chain_id());

      my->accepted_block_connection.emplace(chain.accepted_block.connect( [&]( const chain::block_state_ptr& bs ){
         my->accepted_block( bs );
      }));
      my->irreversible_block_connection.emplace(chain.irreversible_block.connect( [&]( const chain::block_state_ptr& bs ){
         my->applied_irreversible_block( bs );
      }));
      my->accepted_transaction_connection.emplace(chain.accepted_transaction.connect( [&]( const chain::transaction_metadata_ptr& t ){
         my->accepted_transaction(t);
      }));
      my->applied_transaction_connection.emplace(chain.applied_transaction.connect( [&]( const chain::transaction_trace_ptr& t ){
         my->applied_transaction(t);
      }));

      if (my->wipe_database_on_startup) {
         my->wipe_database();
      }
      my->init();
   } else {
      wlog("eosio::mongo_db_plugin configured, but no --mongodb-uri specified.");
      wlog("mongo_db_plugin disabled.");
   }
}

void mongo_db_plugin::plugin_startup()
{
   if (my->configured) {
      ilog("starting db plugin");

      my->consume_thread = boost::thread([this] { my->consume_blocks(); });

      // chain_controller is created and has resynced or replayed if needed
      my->startup = false;
   }
}

void mongo_db_plugin::plugin_shutdown()
{
   my->accepted_block_connection.reset();
   my->irreversible_block_connection.reset();
   my->accepted_transaction_connection.reset();
   my->applied_transaction_connection.reset();

   my.reset();
}

} // namespace eosio
