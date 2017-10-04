#include <eos/db_plugin/db_plugin.hpp>
#include <eos/chain/chain_controller.hpp>
#include <eos/chain/config.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/transaction.hpp>
#include <eos/chain/types.hpp>

#include <fc/crypto/sha256.hpp>
#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <eos/chain/multi_index_includes.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

namespace fc { class variant; }

namespace eos {

using chain::AccountName;
using chain::block_id_type;
using chain::PermissionName;
using chain::ProcessedTransaction;
using chain::signed_block;
using chain::transaction_id_type;

class db_plugin_impl {
public:
   db_plugin_impl();
   ~db_plugin_impl();

   void applied_irreversible_block(const signed_block&);
   void process_irreversible_block(const signed_block&);

   void wipe_database();

   bool wipe_database_on_startup{false};
   mongocxx::instance mongo_inst;
   mongocxx::client mongo_conn;
   std::set<AccountName> filter_on;
   std::unique_ptr<boost::lockfree::spsc_queue<signed_block>> queue;
   boost::thread consum_thread;
   boost::atomic<bool> startup{true};
   boost::atomic<bool> done{false};

   void consum_blocks();

   bool is_scope_relevant(const eos::types::Vector<AccountName>& scope);
   static void add(chainbase::database& db, const vector<types::KeyPermissionWeight>& keys, const AccountName& account_name, const PermissionName& permission);

//   template<typename MultiIndex, typename LookupType>
//   static void remove(chainbase::database& db, const AccountName& account_name, const PermissionName& permission)
//   {
//      const auto& idx = db.get_index<MultiIndex, LookupType>();
//      auto& mutatable_idx = db.get_mutable_index<MultiIndex>();
//      auto range = idx.equal_range( boost::make_tuple( account_name, permission ) );
//
//      for (auto acct_perm = range.first; acct_perm != range.second; ++acct_perm)
//      {
//         mutatable_idx.remove(*acct_perm);
//      }
//   }

   static void add(chainbase::database& db, const vector<types::AccountPermissionWeight>& controlling_accounts, const AccountName& account_name, const PermissionName& permission);
   static const AccountName NEW_ACCOUNT;
   static const AccountName UPDATE_AUTH;
   static const AccountName DELETE_AUTH;
   static const PermissionName OWNER;
   static const PermissionName ACTIVE;
   static const PermissionName RECOVERY;

   static const std::string db_name;
   static const std::string blocks_col;
   static const std::string trans_col;
   static const std::string msgs_col;
};

const AccountName db_plugin_impl::NEW_ACCOUNT = "newaccount";
const AccountName db_plugin_impl::UPDATE_AUTH = "updateauth";
const AccountName db_plugin_impl::DELETE_AUTH = "deleteauth";
const PermissionName db_plugin_impl::OWNER = "owner";
const PermissionName db_plugin_impl::ACTIVE = "active";
const PermissionName db_plugin_impl::RECOVERY = "recovery";

const std::string db_plugin_impl::db_name = "EOS";
const std::string db_plugin_impl::blocks_col = "Blocks";
const std::string db_plugin_impl::trans_col = "Transactions";
const std::string db_plugin_impl::msgs_col = "Messages";


void db_plugin_impl::applied_irreversible_block(const signed_block& block) {
   if (startup) {
      // on startup we don't want to queue, instead push back on caller
      process_irreversible_block(block);
   } else {
      if (!queue->push(block)) {
         // TODO what to do if full
         elog("queue is full!!!!!");
         FC_ASSERT(false, "queue is full");
      }
   }
}

void db_plugin_impl::consum_blocks() {
   signed_block block;
   while (!done) {
      while (queue->pop(block)) {
         ilog("queue size: ${q}", ("q", queue->read_available()+1));
         process_irreversible_block(block);
      }
   }
   while (queue->pop(block)) {
      process_irreversible_block(block);
   }
   ilog("db_plugin consum thread shutdown gracefully");
}

//
// Blocks
//    block_num         int
//    block_id          sha256
//    prev_block_id     sha256
//    timestamp         sec_since_epoch
//    transaction_merkle_root  sha256
//    producer_account_id      string
void db_plugin_impl::process_irreversible_block(const signed_block& block)
{
   using namespace bsoncxx::types;
   using namespace bsoncxx::builder;

   bool transactions_in_block = false;
   mongocxx::options::bulk_write bulk_opts;
   bulk_opts.ordered(false);
   mongocxx::bulk_write bulk_trans{bulk_opts};

   auto blocks = mongo_conn[db_name][blocks_col]; // Blocks
   auto trans = mongo_conn[db_name][trans_col]; // Transactions
   auto msgs = mongo_conn[db_name][msgs_col]; // Messages

   stream::document block_doc{};
   const auto block_id = block.id();
   const auto block_id_str = block_id.str();

   block_doc << "block_num" << b_int32{static_cast<int32_t>(block.block_num())}
       << "block_id" << block_id_str
       << "prev_block_id" << block.previous.str()
       << "timestamp" << b_date{std::chrono::milliseconds{std::chrono::seconds{block.timestamp.sec_since_epoch()}}}
       << "transaction_merkle_root" << block.transaction_merkle_root.str()
       << "producer_account_id" << block.producer.toString();
   if (!blocks.insert_one(block_doc.view())) {
      elog("Failed to insert block ${bid}", ("bid", block_id));
   }

   const bool check_relevance = !filter_on.empty();
   for (const auto& cycle : block.cycles) {
      for (const auto& thread : cycle) {
         for (const auto& trx : thread.user_input) {
            if (check_relevance && !is_scope_relevant(trx.scope))
               continue;

            stream::document doc{};
            const auto trans_id_str = trx.id().str();
            auto trx_doc = doc
                  << "transaction_id" << trans_id_str
                  << "block_id" << block_id_str
                  << "ref_block_num" << b_int32{static_cast<int32_t >(trx.refBlockNum)}
                  << "ref_block_prefix" << trx.refBlockPrefix.str()
                  << "scope" << stream::open_array;
            for (const auto& account_name : trx.scope)
               trx_doc = trx_doc << account_name.toString();
            trx_doc = trx_doc
                  << stream::close_array
                  << "read_scope" << stream::open_array;
            for (const auto& account_name : trx.readscope)
               trx_doc = trx_doc << account_name.toString();
            trx_doc = trx_doc
                  << stream::close_array
                  << "expiration" << b_date{std::chrono::milliseconds{std::chrono::seconds{trx.expiration.sec_since_epoch()}}}
                  << "signatures" << stream::open_array;
            for (const auto& sig : trx.signatures) {
               trx_doc = trx_doc << fc::json::to_string(sig);
            }
            trx_doc = trx_doc
                  << stream::close_array
                  << "messages" << stream::open_array;

            mongocxx::bulk_write bulk_msgs{bulk_opts};
            int32_t i = 0;
            for (const chain::Message& msg : trx.messages) {
               auto msg_oid = bsoncxx::oid{};
               trx_doc = trx_doc << msg_oid; // add to transaction.messages array
               stream::document msg_builder{};
               auto msg_doc = msg_builder
                  << "_id" << b_oid{msg_oid}
                  << "message_id" << b_int32{i}
                  << "transaction_id" << trans_id_str
                  << "authorization" << stream::open_array;
               for (const auto& auth : msg.authorization) {
                  msg_doc = msg_doc << stream::open_document
                        << "account" << auth.account.toString()
                        << "permission" << auth.permission.toString()
                        << stream::close_document;
               }
               auto msg_complete = msg_doc << stream::close_array << stream::finalize;
               mongocxx::model::insert_one insert_msg{msg_complete.view()};
               bulk_msgs.append(insert_msg);
               ++i;
            }

            if (!trx.messages.empty()) {
               auto result = msgs.bulk_write(bulk_msgs);
               if (!result) {
                  elog("Bulk message insert failed for block: ${bid}, transaction: ${trx}", ("bid", block_id)("trx", trx.id()));
               }
            }

            auto complete_doc = trx_doc << stream::close_array << stream::finalize;
            mongocxx::model::insert_one insert_op{complete_doc.view()};
            transactions_in_block = true;
            bulk_trans.append(insert_op);


            for (const chain::Message& msg : trx.messages)
            {
               if (msg.code == config::EosContractName)
               {
                  if (msg.type == NEW_ACCOUNT)
                  {
                     const auto create = msg.as<types::newaccount>();
//                     add(db, create.owner.keys, create.name, OWNER);
//                     add(db, create.active.keys, create.name, ACTIVE);
//                     add(db, create.recovery.keys, create.name, RECOVERY);
//
//                     add(db, create.owner.accounts, create.name, OWNER);
//                     add(db, create.active.accounts, create.name, ACTIVE);
//                     add(db, create.recovery.accounts, create.name, RECOVERY);
                  }
                  else if (msg.type == UPDATE_AUTH)
                  {
                     const auto update = msg.as<types::updateauth>();
//                     remove<public_key_history_multi_index, by_account_permission>(db, update.account, update.permission);
//                     add(db, update.authority.keys, update.account, update.permission);
//
//                     remove<account_control_history_multi_index, by_controlled_authority>(db, update.account, update.permission);
//                     add(db, update.authority.accounts, update.account, update.permission);
                  }
                  else if (msg.type == DELETE_AUTH)
                  {
                     const auto del = msg.as<types::deleteauth>();
//                     remove<public_key_history_multi_index, by_account_permission>(db, del.account, del.permission);
//
//                     remove<account_control_history_multi_index, by_controlled_authority>(db, del.account, del.permission);
                  }
               }
            }
         }
      }
   }


   if (transactions_in_block) {
      auto result = trans.bulk_write(bulk_trans);
      if (!result) {
         elog("Bulk transaction insert failed for block: ${bid}", ("bid", block_id));
      }
   }

}

void db_plugin_impl::add(chainbase::database& db, const vector<types::KeyPermissionWeight>& keys, const AccountName& account_name, const PermissionName& permission)
{
   for (auto pub_key_weight : keys )
   {
//      db.create<public_key_history_object>([&](public_key_history_object& obj) {
//         obj.public_key = pub_key_weight.key;
//         obj.account_name = account_name;
//         obj.permission = permission;
//      });
   }
}

void db_plugin_impl::add(chainbase::database& db, const vector<types::AccountPermissionWeight>& controlling_accounts, const AccountName& account_name, const PermissionName& permission)
{
   for (auto controlling_account : controlling_accounts )
   {
//      db.create<account_control_history_object>([&](account_control_history_object& obj) {
//         obj.controlled_account = account_name;
//         obj.controlled_permission = permission;
//         obj.controlling_account = controlling_account.permission.account;
//      });
   }
}

bool db_plugin_impl::is_scope_relevant(const eos::types::Vector<AccountName>& scope)
{
   for (const AccountName& account_name : scope)
      if (filter_on.count(account_name))
         return true;

   return false;
}

db_plugin_impl::db_plugin_impl()
: mongo_inst{}
, mongo_conn{}
{
}

db_plugin_impl::~db_plugin_impl() {
   try {
      done = true;
      consum_thread.join();
   } catch (std::exception& e) {
      elog("Exception on db_plugin shutdown of consum thread: ${e}", ("e", e.what()));
   }
}

void db_plugin_impl::wipe_database() {
   ilog("db wipe_database");
   mongo_conn[db_name].drop();
}

//////////
// db_plugin
//////////

db_plugin::db_plugin()
:my(new db_plugin_impl)
{
}

db_plugin::~db_plugin()
{
}

void db_plugin::set_program_options(options_description& cli, options_description& cfg)
{
   cfg.add_options()
         ("filter_on_accounts,f", bpo::value<std::vector<std::string>>()->composing(),
          "Track only transactions whose scopes involve the listed accounts. Default is to track all transactions.")
         ("queue_size,q", bpo::value<uint>()->default_value(1024),
         "The block queue size")
         ;
}

void db_plugin::wipe_database() {
   if (!my->startup) {
      elog("ERROR: db_plugin::wipe_database() called before configuration or after startup. Ignoring.");
   } else {
      my->wipe_database_on_startup = true;
   }
}

void db_plugin::applied_irreversible_block(const signed_block& block) {
   my->applied_irreversible_block(block);
}


void db_plugin::plugin_initialize(const variables_map& options)
{
   ilog("initializing db plugin");
   if(options.count("filter_on_accounts")) {
      auto foa = options.at("filter_on_accounts").as<std::vector<std::string>>();
      for(auto filter_account : foa)
         my->filter_on.emplace(filter_account);
   } else if (options.count("queue_size")) {
      auto size = options.at("queue_size").as<uint>();
      my->queue = std::make_unique<boost::lockfree::spsc_queue<signed_block>>(size);
   }

   // TODO: add command line for uri
   my->mongo_conn = mongocxx::client{mongocxx::uri{}};

   if (my->wipe_database_on_startup) {
      my->wipe_database();
   }
}

void db_plugin::plugin_startup()
{
   ilog("starting db plugin");
   // TODO: during chain startup we want to pushback on apply so that chainbase has to wait for db.
   // TODO: assert that last irreversible in db is one less than received (on startup only?, every so often?)
//   my->chain_plug = app().find_plugin<chain_plugin>();
//   auto& db = my->chain_plug->chain().get_mutable_database();
//   db.add_index<account_control_history_multi_index>();
//   db.add_index<account_transaction_history_multi_index>();
//   db.add_index<public_key_history_multi_index>();
//   db.add_index<transaction_history_multi_index>();

   my->consum_thread = boost::thread([this]{ my->consum_blocks(); });

   // chain_controller is created and has resynced or replayed if needed
   my->startup = false;
}

void db_plugin::plugin_shutdown()
{
   my.reset();
}

} // namespace eos
