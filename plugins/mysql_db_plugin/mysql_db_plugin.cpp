/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/mysql_db_plugin/mysql_db_plugin.hpp>
#include <eosio/chain/eosio_contract.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>

#include <fc/io/json.hpp>
#include <fc/utf8.hpp>
#include <fc/variant.hpp>

#include <boost/chrono.hpp>
#include <boost/signals2/connection.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <queue>

#include <future>

/**
 * table classes
 * */
#include "accounts_table.h"
#include "transactions_table.h"
#include "blocks_table.h"
#include "actions_table.h"


/**
 * libzdb connection pool unity tests. 
 */
#define BSIZE 2048

namespace fc { class variant; }

namespace eosio {

using chain::account_name;
using chain::action_name;
using chain::block_id_type;
using chain::permission_name;
using chain::transaction;
using chain::signed_transaction;
using chain::signed_block;
using chain::transaction_id_type;
using chain::packed_transaction;

static appbase::abstract_plugin& _mysql_db_plugin = app().register_plugin<mysql_db_plugin>();

class mysql_db_plugin_impl {
public:
   mysql_db_plugin_impl();
   ~mysql_db_plugin_impl();

   fc::optional<boost::signals2::scoped_connection> accepted_block_connection;
   fc::optional<boost::signals2::scoped_connection> irreversible_block_connection;
   fc::optional<boost::signals2::scoped_connection> accepted_transaction_connection;
   fc::optional<boost::signals2::scoped_connection> applied_transaction_connection;

   void consume_accepted_transactions();
   void consume_applied_transactions();
   void consume_blocks();

   void accepted_block( const chain::block_state_ptr& );
   void applied_irreversible_block(const chain::block_state_ptr&);
   void accepted_transaction(const chain::transaction_metadata_ptr&);
   void applied_transaction(const chain::transaction_trace_ptr&);
   void process_accepted_transaction(const chain::transaction_metadata_ptr&);
   void _process_accepted_transaction(const chain::transaction_metadata_ptr&);
   void process_applied_transaction(const chain::transaction_trace_ptr&);
   void _process_applied_transaction(const chain::transaction_trace_ptr&);
   void process_accepted_block( const chain::block_state_ptr& );
   void _process_accepted_block( const chain::block_state_ptr& );
   void process_irreversible_block(const chain::block_state_ptr&);
   void _process_irreversible_block(const chain::block_state_ptr&);

   void init(const std::string& uri, uint32_t block_num_start);
   void wipe_database();
   bool is_started();

   bool configured{false};
   bool wipe_database_on_startup{false};
   uint32_t start_block_num = 0;
   bool start_block_reached = false;

/**
 * mysql db connection definition
 **/
   std::shared_ptr<connection_pool> m_connection_pool;
   std::unique_ptr<accounts_table> m_accounts_table;
   std::unique_ptr<actions_table> m_actions_table;
   std::unique_ptr<blocks_table> m_blocks_table;
   std::unique_ptr<transactions_table> m_transactions_table;
   std::string system_account;
   uint32_t m_block_num_start;

   size_t queue_size = 0;
   std::deque<chain::transaction_metadata_ptr> transaction_metadata_queue;
   std::deque<chain::transaction_metadata_ptr> transaction_metadata_process_queue;
   std::deque<chain::transaction_trace_ptr> transaction_trace_queue;
   std::deque<chain::transaction_trace_ptr> transaction_trace_process_queue;
   std::deque<chain::block_state_ptr> block_state_queue;
   std::deque<chain::block_state_ptr> block_state_process_queue;
   std::deque<chain::block_state_ptr> irreversible_block_state_queue;
   std::deque<chain::block_state_ptr> irreversible_block_state_process_queue;
   boost::mutex mtx;
   boost::condition_variable condition;
   boost::thread consume_thread_accepted_trans;
   boost::thread consume_thread_applied_trans;
   boost::thread consume_thread_blocks;
   boost::atomic<bool> done{false};
   boost::atomic<bool> startup{true};
   fc::optional<chain::chain_id_type> chain_id;
   fc::microseconds abi_serializer_max_time;

   static const account_name newaccount;
   static const account_name setabi;
};

namespace {

template<typename Queue, typename Entry>
void queue(boost::mutex& mtx, boost::condition_variable& condition, Queue& queue, const Entry& e, size_t queue_size) {
   int sleep_time = 100;
   size_t last_queue_size = 0;
   boost::mutex::scoped_lock lock(mtx);
   if (queue.size() > queue_size) {
      lock.unlock();
      condition.notify_one();
      if (last_queue_size < queue.size()) {
         sleep_time += 100;
      } else {
         sleep_time -= 100;
         if (sleep_time < 0) sleep_time = 100;
      }
      last_queue_size = queue.size();
      boost::this_thread::sleep_for(boost::chrono::milliseconds(sleep_time));
      lock.lock();
   }
   queue.emplace_back(e);
   lock.unlock();
   condition.notify_one();
}

}

void mysql_db_plugin_impl::accepted_transaction( const chain::transaction_metadata_ptr& t ) {
   try {
      queue( mtx, condition, transaction_metadata_queue, t, queue_size );
   } catch (fc::exception& e) {
      elog("FC Exception while accepted_transaction ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while accepted_transaction ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while accepted_transaction");
   }
}

void mysql_db_plugin_impl::applied_transaction( const chain::transaction_trace_ptr& t ) {
   try {
      queue( mtx, condition, transaction_trace_queue, t, queue_size );
   } catch (fc::exception& e) {
      elog("FC Exception while applied_transaction ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while applied_transaction ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while applied_transaction");
   }
}

void mysql_db_plugin_impl::applied_irreversible_block( const chain::block_state_ptr& bs ) {
   try {
      queue( mtx, condition, irreversible_block_state_queue, bs, queue_size );
   } catch (fc::exception& e) {
      elog("FC Exception while applied_irreversible_block ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while applied_irreversible_block ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while applied_irreversible_block");
   }
}

void mysql_db_plugin_impl::accepted_block( const chain::block_state_ptr& bs ) {
   try {
      queue( mtx, condition, block_state_queue, bs, queue_size );
   } catch (fc::exception& e) {
      elog("FC Exception while accepted_block ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while accepted_block ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while accepted_block");
   }
}

void mysql_db_plugin_impl::consume_accepted_transactions() {
   try {
      while (true) {
         boost::mutex::scoped_lock lock(mtx);
         while ( transaction_metadata_queue.empty() &&
                 !done ) {
            condition.wait(lock);
         }

         // capture for processing
         size_t transaction_metadata_size = transaction_metadata_queue.size();
         if (transaction_metadata_size > 0) {
            transaction_metadata_process_queue = move(transaction_metadata_queue);
            transaction_metadata_queue.clear();
         }
         
         lock.unlock();

         // warn if queue size greater than 75%
         if( transaction_metadata_size > (queue_size * 0.75)) {
            wlog("queue size: ${q}", ("q", transaction_metadata_size));
         } else if (done) {
            ilog("draining queue, size: ${q}", ("q", transaction_metadata_size));
         }

         // process transactions
         while (!transaction_metadata_process_queue.empty()) {
            const auto& t = transaction_metadata_process_queue.front();
            process_accepted_transaction(t);
            transaction_metadata_process_queue.pop_front();
         }

         if( transaction_metadata_size == 0 &&
             done ) {
            break;
         }
      }
      ilog("mysql_db_plugin consume thread shutdown gracefully");
   } catch (fc::exception& e) {
      elog("FC Exception while consuming block ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while consuming block ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while consuming block");
   }
}

void mysql_db_plugin_impl::consume_applied_transactions() {
   try {
      while (true) {
         boost::mutex::scoped_lock lock(mtx);
         while ( transaction_trace_queue.empty() &&
                 !done ) {
            condition.wait(lock);
         }

         // capture for processing
         size_t transaction_trace_size = transaction_trace_queue.size();
         if (transaction_trace_size > 0) {
            transaction_trace_process_queue = move(transaction_trace_queue);
            transaction_trace_queue.clear();
         }
         
         lock.unlock();

         // warn if queue size greater than 75%
         if( transaction_trace_size > (queue_size * 0.75)) {
            wlog("queue size: ${q}", ("q", transaction_trace_size));
         } else if (done) {
            ilog("draining queue, size: ${q}", ("q", transaction_trace_size));
         }

         // process transactions
         while (!transaction_trace_process_queue.empty()) {
            const auto& t = transaction_trace_process_queue.front();
            process_applied_transaction(t);
            transaction_trace_process_queue.pop_front();
         }

         if( transaction_trace_size == 0 &&
             done ) {
            break;
         }
      }
      ilog("mysql_db_plugin consume thread shutdown gracefully");
   } catch (fc::exception& e) {
      elog("FC Exception while consuming block ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while consuming block ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while consuming block");
   }
}

void mysql_db_plugin_impl::consume_blocks() {
   try {
      while (true) {
         boost::mutex::scoped_lock lock(mtx);
         while ( block_state_queue.empty() &&
                 irreversible_block_state_queue.empty() &&
                 !done ) {
            condition.wait(lock);
         }

         // capture for processing
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
         if( block_state_size > (queue_size * 0.75) ||
             irreversible_block_size > (queue_size * 0.75)) {
            wlog("queue size: ${q}", ("q", block_state_size + irreversible_block_size));
         } else if (done) {
            ilog("draining queue, size: ${q}", ("q", block_state_size + irreversible_block_size));
         }

         // process blocks
         while (!block_state_process_queue.empty()) {
            const auto& bs = block_state_process_queue.front();
            process_accepted_block( bs );
            block_state_process_queue.pop_front();
         }

         // process irreversible blocks
         while (!irreversible_block_state_process_queue.empty()) {
            const auto& bs = irreversible_block_state_process_queue.front();
            process_irreversible_block(bs);
            irreversible_block_state_process_queue.pop_front();
         }

         if( block_state_size == 0 &&
             irreversible_block_size == 0 &&
             done ) {
            break;
         }
      }
      ilog("mysql_db_plugin consume thread shutdown gracefully");
   } catch (fc::exception& e) {
      elog("FC Exception while consuming block ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while consuming block ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while consuming block");
   }
}

void mysql_db_plugin_impl::process_accepted_transaction( const chain::transaction_metadata_ptr& t ) {
   try {
      // always call since we need to capture setabi on accounts even if not storing transactions
      _process_accepted_transaction(t);
   } catch (fc::exception& e) {
      elog("FC Exception while processing accepted transaction metadata: ${e}", ("e", e.to_detail_string()));
   } catch (std::exception& e) {
      elog("STD Exception while processing accepted tranasction metadata: ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while processing accepted transaction metadata");
   }
}

void mysql_db_plugin_impl::process_applied_transaction( const chain::transaction_trace_ptr& t ) {
   try {
      if( start_block_reached ) {
         _process_applied_transaction( t );
      }
   } catch (fc::exception& e) {
      elog("FC Exception while processing applied transaction trace: ${e}", ("e", e.to_detail_string()));
   } catch (std::exception& e) {
      elog("STD Exception while processing applied transaction trace: ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while processing applied transaction trace");
   }
}

void mysql_db_plugin_impl::process_irreversible_block(const chain::block_state_ptr& bs) {
  try {
     if( start_block_reached ) {
        _process_irreversible_block( bs );
     }
  } catch (fc::exception& e) {
     elog("FC Exception while processing irreversible block: ${e}", ("e", e.to_detail_string()));
  } catch (std::exception& e) {
     elog("STD Exception while processing irreversible block: ${e}", ("e", e.what()));
  } catch (...) {
     elog("Unknown exception while processing irreversible block");
  }
}

void mysql_db_plugin_impl::process_accepted_block( const chain::block_state_ptr& bs ) {
   try {
      if( !start_block_reached ) {
         if( bs->block_num >= start_block_num ) {
            start_block_reached = true;
         }
      }
      if( start_block_reached ) {
         _process_accepted_block( bs );
      }
   } catch (fc::exception& e) {
      elog("FC Exception while processing accepted block trace ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while processing accepted block trace ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while processing accepted block trace");
   }
}

void mysql_db_plugin_impl::_process_accepted_transaction( const chain::transaction_metadata_ptr& t ) {
   auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

   const auto trx_id = t->id;
   const auto trx_id_str = trx_id.str();
   const auto& trx = t->trx;
   const chain::transaction_header& trx_header = trx;

   bool actions_to_write = false;
   
   int32_t act_num = 0;
   auto process_action = [&](const std::string& trx_id_str, const chain::action& act, const  bool cfa) -> auto {
      try {
         m_accounts_table->update_account( act );
      } catch (...) {
         ilog( "Unable to update account for ${s}::${n}::${t}::${e}", ("s", act.account)( "n", act.name )( "t", trx_header )( "e", trx_header.expiration ));
      }
      if( start_block_reached ) {
         m_actions_table->add(act, trx_id, trx_header.expiration, act_num);   
         actions_to_write = true;
      }
       ++act_num;
       return act_num;
   };

   if( start_block_reached ) {
      string signing_keys_json;
      if( t->signing_keys.valid()) {
         signing_keys_json = fc::json::to_string( t->signing_keys->second );
      } else {
         auto signing_keys = trx.get_signature_keys( *chain_id, false, false );
         if( !signing_keys.empty()) {
            signing_keys_json = fc::json::to_string( signing_keys );
         }
      }
      string trx_header_json = fc::json::to_string( trx_header );
   }

   if( !trx.actions.empty()) {
      for( const auto& act : trx.actions ) {
         process_action( trx_id_str, act, false );
      }
   }

   if( start_block_reached ) {
      act_num = 0;
      if( !trx.context_free_actions.empty()) {
         for( const auto& cfa : trx.context_free_actions ) {
            process_action( trx_id_str, cfa, true );
         }
      }
   }
}

void mysql_db_plugin_impl::_process_applied_transaction( const chain::transaction_trace_ptr& t ) {
   auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

}

void mysql_db_plugin_impl::_process_accepted_block( const chain::block_state_ptr& block ) {
   try {      
      m_blocks_table->add(block->block);
      for (const auto &transaction : block->trxs) {
            m_transactions_table->add(block->block_num, transaction->trx);
      }
    } catch (const std::exception &ex) {
        elog("${e}", ("e", ex.what())); // prevent crash
    }
}

void mysql_db_plugin_impl::_process_irreversible_block(const chain::block_state_ptr& block)
{
   const auto block_id = block->block->id();
   const auto block_id_str = block_id.str();
   const auto block_num = block->block->block_num();

   // genesis block 1 is not signaled to accepted_block
   if (block_num < 2) return;

   auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

   try {      
      m_blocks_table->process_irreversible(block->block);
   } catch (const std::exception &ex) {
        elog("${e}", ("e", ex.what())); // prevent crash
    }
}

mysql_db_plugin_impl::mysql_db_plugin_impl(){}
mysql_db_plugin_impl::~mysql_db_plugin_impl() {
   if (!startup) {
      try {
         ilog( "mysql_db_plugin shutdown in process please be patient this can take a few minutes" );
         done = true;
         condition.notify_one();

         consume_thread_accepted_trans.join();
         consume_thread_applied_trans.join();
         consume_thread_blocks.join();

      } catch( std::exception& e ) {
         elog( "Exception on mysql_db_plugin shutdown of consume thread: ${e}", ("e", e.what()));
      }
   }
}

void mysql_db_plugin_impl::wipe_database() {
   ilog("drop tables");

   // drop tables
   m_actions_table->drop();
   m_transactions_table->drop();
   m_blocks_table->drop();
   m_accounts_table->drop();

   ilog("create tables");
   // create Tables
   m_accounts_table->create();
   m_blocks_table->create();
   m_transactions_table->create();
   m_actions_table->create();

   m_accounts_table->add(system_account);
}

bool mysql_db_plugin_impl::is_started()
{
   return m_accounts_table->exist(system_account);
}

void mysql_db_plugin_impl::init(const std::string& uri, uint32_t block_num_start) {

   m_connection_pool = std::make_shared<connection_pool>(uri);
   m_accounts_table = std::make_unique<accounts_table>(m_connection_pool);
   m_blocks_table = std::make_unique<blocks_table>(m_connection_pool);
   m_transactions_table = std::make_unique<transactions_table>(m_connection_pool);
   m_actions_table = std::make_unique<actions_table>(m_connection_pool);
   m_block_num_start = block_num_start;
   system_account = chain::name(chain::config::system_account_name).to_string();

   if( wipe_database_on_startup ) {
      wipe_database();
   }

   ilog("starting mysql db plugin thread");

   consume_thread_accepted_trans = boost::thread([this] { consume_accepted_transactions(); });
   consume_thread_applied_trans = boost::thread([this] { consume_applied_transactions(); });
   consume_thread_blocks = boost::thread([this] { consume_blocks(); });

   startup = false;
}

mysql_db_plugin::mysql_db_plugin()
:my(new mysql_db_plugin_impl())
{

}

mysql_db_plugin::~mysql_db_plugin()
{
      
}

void mysql_db_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("mysqldb-queue-size,q", bpo::value<uint32_t>()->default_value(256),
         "The target queue size between nodeos and mySQL DB plugin thread.")
         ("mysqldb-wipe", bpo::bool_switch()->default_value(false),
         "Required with --replay-blockchain, --hard-replay-blockchain, or --delete-all-blocks to wipe mysql db."
         "This option required to prevent accidental wipe of mysql db.")
         ("mysqldb-block-start", bpo::value<uint32_t>()->default_value(0),
         "If specified then only abi data pushed to mysqldb until specified block is reached.")
         ("mysqldb-uri,m", bpo::value<std::string>(),
         "mysqlDB URI connection string."
               " If not specified then plugin is disabled. Default database 'EOS' is used if not specified in URI."
               " Example: mysql://localhost:3306/EOS?user=root&password=root")
         ;
}

void mysql_db_plugin::plugin_initialize(const variables_map& options) {
   try {
      if( options.count( "mysqldb-uri" )) {
         ilog( "initializing mysql_db_plugin" );
         my->configured = true;

         if( options.at( "replay-blockchain" ).as<bool>() || options.at( "hard-replay-blockchain" ).as<bool>() || options.at( "delete-all-blocks" ).as<bool>() ) {
            if( options.at( "mysqldb-wipe" ).as<bool>()) {
               ilog( "Wiping mysql database on startup" );
               my->wipe_database_on_startup = true;
            } else if( options.count( "mysqldb-block-start" ) == 0 ) {
               EOS_ASSERT( false, chain::plugin_config_exception, "--mysqldb-wipe required with --replay-blockchain, --hard-replay-blockchain, or --delete-all-blocks"
                                 " --mysqldb-wipe will remove all EOS collections from mysqldb." );
            }
         }

         if( options.count( "abi-serializer-max-time-ms") == 0 ) {
            EOS_ASSERT(false, chain::plugin_config_exception, "--abi-serializer-max-time-ms required as default value not appropriate for parsing full blocks");
         }
         my->abi_serializer_max_time = app().get_plugin<chain_plugin>().get_abi_serializer_max_time();

         if( options.count( "mysqldb-queue-size" )) {
            my->queue_size = options.at( "mysqldb-queue-size" ).as<uint32_t>();
         }
         if( options.count( "mysqldb-block-start" )) {
            my->start_block_num = options.at( "mysqldb-block-start" ).as<uint32_t>();
         }
         if( my->start_block_num == 0 ) {
            my->start_block_reached = true;
         }

         // TODO : create mysql db connection pool
         std::string uri_str = options.at( "mysqldb-uri" ).as<std::string>();
         ilog( "connecting to ${u}", ("u", uri_str));
         
         // hook up to signals on controller
         chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
         EOS_ASSERT( chain_plug, chain::missing_chain_plugin_exception, ""  );
         auto& chain = chain_plug->chain();
         my->chain_id.emplace( chain.get_chain_id());

         my->accepted_block_connection.emplace( chain.accepted_block.connect( [&]( const chain::block_state_ptr& bs ) {
            my->accepted_block( bs );
         } ));
         my->irreversible_block_connection.emplace(
               chain.irreversible_block.connect( [&]( const chain::block_state_ptr& bs ) {
                  my->applied_irreversible_block( bs );
               } ));
         my->accepted_transaction_connection.emplace(
               chain.accepted_transaction.connect( [&]( const chain::transaction_metadata_ptr& t ) {
                  my->accepted_transaction( t );
               } ));
         my->applied_transaction_connection.emplace(
               chain.applied_transaction.connect( [&]( const chain::transaction_trace_ptr& t ) {
                  my->applied_transaction( t );
               } ));
         my->init(uri_str,my->start_block_num);
      } else {
         wlog( "eosio::mysql_db_plugin configured, but no --mysqldb-uri specified." );
         wlog( "mysql_db_plugin disabled." );
      }
   } FC_LOG_AND_RETHROW()
}

void mysql_db_plugin::plugin_startup() {
   // Make the magic happen
}

void mysql_db_plugin::plugin_shutdown() {
   // OK, that's enough magic
   my->accepted_block_connection.reset();
   my->irreversible_block_connection.reset();
   my->accepted_transaction_connection.reset();
   my->applied_transaction_connection.reset();

   my.reset();
}

}
