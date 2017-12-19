/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/db_plugin/db_plugin.hpp>
#include <eos/chain/config.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/transaction.hpp>
#include <eos/chain/types.hpp>
#include <eos/native_contract/native_contract_chain_initializer.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <queue>

#ifdef MONGODB
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#endif

namespace fc { class variant; }

namespace eosio {

using chain::account_name;
using chain::func_name;
using chain::block_id_type;
using chain::permission_name;
using chain::processed_transaction;
using chain::signed_block;
using chain::transaction_id_type;

#ifndef MONGODB
class db_plugin_impl {
public:
   db_plugin_impl() {}
};
#endif

#ifdef MONGODB
class db_plugin_impl {
public:
   db_plugin_impl();
   ~db_plugin_impl();

   void applied_irreversible_block(const signed_block&);
   void process_irreversible_block(const signed_block&);
   void _process_irreversible_block(const signed_block&);

   void init();
   void wipe_database();

   std::set<account_name> filter_on;
   static types::abi eos_abi; // cached for common use

   bool configured{false};
   bool wipe_database_on_startup{false};

   std::string db_name;
   mongocxx::instance mongo_inst;
   mongocxx::client mongo_conn;
   mongocxx::collection accounts;

   size_t queue_size = 0;
   size_t processed = 0;
   std::queue<signed_block> queue;
   boost::mutex mtx;
   boost::condition_variable condtion;
   boost::thread consum_thread;
   boost::atomic<bool> done{false};
   boost::atomic<bool> startup{true};

   void consum_blocks();

   bool is_scope_relevant(const eosio::types::vector<account_name>& scope);
   void update_account(const chain::message& msg);

   static const func_name newaccount;
   static const func_name transfer;
   static const func_name lock;
   static const func_name unlock;
   static const func_name claim;
   static const func_name setcode;

   static const std::string blocks_col;
   static const std::string trans_col;
   static const std::string msgs_col;
   static const std::string accounts_col;
};

types::abi db_plugin_impl::eos_abi;

const func_name db_plugin_impl::newaccount = "newaccount";
const func_name db_plugin_impl::transfer = "transfer";
const func_name db_plugin_impl::lock = "lock";
const func_name db_plugin_impl::unlock = "unlock";
const func_name db_plugin_impl::claim = "claim";
const func_name db_plugin_impl::setcode = "setcode";

const std::string db_plugin_impl::blocks_col = "Blocks";
const std::string db_plugin_impl::trans_col = "Transactions";
const std::string db_plugin_impl::msgs_col = "Messages";
const std::string db_plugin_impl::accounts_col = "Accounts";


void db_plugin_impl::applied_irreversible_block(const signed_block& block) {
   try {
      if (startup) {
         // on startup we don't want to queue, instead push back on caller
         process_irreversible_block(block);
      } else {
         boost::mutex::scoped_lock lock(mtx);
         queue.push(block);
         lock.unlock();
         condtion.notify_one();
      }
   } catch (fc::exception& e) {
      elog("FC Exception while applied_irreversible_block ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while applied_irreversible_block ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while applied_irreversible_block");
   }
}

void db_plugin_impl::consum_blocks() {
   try {
      signed_block block;
      size_t size = 0;
      while (true) {
         boost::mutex::scoped_lock lock(mtx);
         while (queue.empty() && !done) {
            condtion.wait(lock);
         }
         size = queue.size();
         if (size > 0) {
            block = queue.front();
            queue.pop();
            lock.unlock();
            // warn if queue size greater than 75%
            if (size > (queue_size * 0.75)) {
               wlog("queue size: ${q}", ("q", size + 1));
            } else if (done) {
               ilog("draining queue, size: ${q}", ("q", size + 1));
            }
            process_irreversible_block(block);
            continue;
         } else if (done) {
            break;
         }
      }
      ilog("db_plugin consum thread shutdown gracefully");
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

  void add_data(bsoncxx::builder::basic::document& msg_doc,
                mongocxx::collection& accounts,
                const chain::message& msg)
  {
     using bsoncxx::builder::basic::kvp;
     try {
        types::abi_serializer abis;
        if (msg.code == config::eos_contract_name) {
           abis.set_abi(db_plugin_impl::eos_abi);
        } else {
           auto from_account = find_account(accounts, msg.code);
           auto abi = fc::json::from_string(bsoncxx::to_json(from_account.view()["abi"].get_document())).as<types::abi>();
           abis.set_abi(abi);
        }
        auto v = abis.binary_to_variant(abis.get_action_type(msg.type), msg.data);
        auto json = fc::json::to_string(v);
        try {
           const auto& value = bsoncxx::from_json(json);
           msg_doc.append(kvp("data", value));
           return;
        } catch (std::exception& e) {
           elog("Unable to convert EOS JSON to MongoDB JSON: ${e}", ("e", e.what()));
           elog("  EOS JSON: ${j}", ("j", json));
        }
     } catch (fc::exception& e) {
        elog("Unable to convert message.data to ABI type: ${t}, what: ${e}", ("t", msg.type)("e", e.to_string()));
     } catch (std::exception& e) {
        elog("Unable to convert message.data to ABI type: ${t}, std what: ${e}", ("t", msg.type)("e", e.what()));
     } catch (...) {
        elog("Unable to convert message.data to ABI type: ${t}", ("t", msg.type));
     }
     // if anything went wrong just store raw hex_data
     msg_doc.append(kvp("hex_data", fc::variant(msg.data).as_string()));
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

void db_plugin_impl::process_irreversible_block(const signed_block& block) {
  try {
     _process_irreversible_block(block);
  } catch (fc::exception& e) {
     elog("FC Exception while processing block ${e}", ("e", e.to_string()));
  } catch (std::exception& e) {
     elog("STD Exception while processing block ${e}", ("e", e.what()));
  } catch (...) {
     elog("Unknown exception while processing block");
  }
}

void db_plugin_impl::_process_irreversible_block(const signed_block& block)
{
   using namespace bsoncxx::types;
   using namespace bsoncxx::builder;
   using bsoncxx::builder::basic::kvp;

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
   const auto prev_block_id_str = block.previous.str();
   auto block_num = block.block_num();

   if (processed == 0) {
      if (startup) {
         // verify on start we have no previous blocks
         verify_no_blocks(blocks);
         FC_ASSERT(block_num < 2, "Expected start of block, instead received block_num: ${bn}", ("bn", block_num));
         // Currently we are creating a 'fake' block in chain_controller::initialize_chain() since initial accounts
         // and producers are not written to the block log. If this is the fake block, indicate it as block_num 0.
         if (block_num == 1 && block.producer == config::eos_contract_name) {
            block_num = 0;
         }
      } else {
         // verify on restart we have previous block
         verify_last_block(blocks, prev_block_id_str);
      }

   }

   auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

   block_doc << "block_num" << b_int32{static_cast<int32_t>(block_num)}
       << "block_id" << block_id_str
       << "prev_block_id" << prev_block_id_str
       << "timestamp" << b_date{std::chrono::milliseconds{std::chrono::seconds{block.timestamp.sec_since_epoch()}}}
       << "transaction_merkle_root" << block.transaction_merkle_root.str()
       << "producer_account_id" << block.producer.to_string();
   auto blk_doc = block_doc << "transactions" << stream::open_array;

   int32_t trx_num = -1;
   const bool check_relevance = !filter_on.empty();
   for (const auto& cycle : block.cycles) {
      for (const auto& thread : cycle) {
         for (const auto& trx : thread.user_input) {
            ++trx_num;
            if (check_relevance && !is_scope_relevant(trx.scope))
               continue;

            auto txn_oid = bsoncxx::oid{};
            blk_doc = blk_doc << txn_oid; // add to transaction.messages array
            stream::document doc{};
            const auto trans_id_str = trx.id().str();
            auto trx_doc = doc
                  << "_id" << txn_oid
                  << "transaction_id" << trans_id_str
                  << "sequence_num" << b_int32{trx_num}
                  << "block_id" << block_id_str
                  << "ref_block_num" << b_int32{static_cast<int32_t >(trx.ref_block_num)}
                  << "ref_block_prefix" << trx.ref_block_prefix.str()
                  << "scope" << stream::open_array;
            for (const auto& account_name : trx.scope)
               trx_doc = trx_doc << account_name.to_string();
            trx_doc = trx_doc
                  << stream::close_array
                  << "read_scope" << stream::open_array;
            for (const auto& account_name : trx.read_scope)
               trx_doc = trx_doc << account_name.to_string();
            trx_doc = trx_doc
                  << stream::close_array
                  << "expiration" << b_date{std::chrono::milliseconds{std::chrono::seconds{trx.expiration.sec_since_epoch()}}}
                  << "signatures" << stream::open_array;
            for (const auto& sig : trx.signatures) {
               trx_doc = trx_doc << fc::variant(sig).as_string();
            }
            trx_doc = trx_doc
                  << stream::close_array
                  << "messages" << stream::open_array;

            mongocxx::bulk_write bulk_msgs{bulk_opts};
            int32_t i = 0;
            for (const auto& msg : trx.messages) {
               auto msg_oid = bsoncxx::oid{};
               trx_doc = trx_doc << msg_oid; // add to transaction.messages array

               auto msg_doc = bsoncxx::builder::basic::document{};
               msg_doc.append(kvp("_id", b_oid{msg_oid}),
                              kvp("message_id", b_int32{i}),
                              kvp("transaction_id", trans_id_str));
               msg_doc.append(kvp("authorization", [&msg](bsoncxx::builder::basic::sub_array subarr) {
                  for (const auto& auth : msg.authorization) {
                     subarr.append([&auth](bsoncxx::builder::basic::sub_document subdoc) {
                        subdoc.append(kvp("account", auth.account.to_string()),
                                      kvp("permission", auth.permission.to_string()));
                     });
                  }
               }));
               msg_doc.append(kvp("handler_account_name", msg.code.to_string()));
               msg_doc.append(kvp("type", msg.type.to_string()));
               add_data(msg_doc, accounts, msg);
               msg_doc.append(kvp("createdAt", b_date{now}));
               mongocxx::model::insert_one insert_msg{msg_doc.view()};
               bulk_msgs.append(insert_msg);

               // eos account update
               if (msg.code == config::eos_contract_name) {
                  try {
                     update_account(msg);
                  } catch(fc::exception& e) {
                     elog("Unable to update account ${e}", ("e", e.to_string()));
                  }
               }

               ++i;
            }

            if (!trx.messages.empty()) {
               auto result = msgs.bulk_write(bulk_msgs);
               if (!result) {
                  elog("Bulk message insert failed for block: ${bid}, transaction: ${trx}", ("bid", block_id)("trx", trx.id()));
               }
            }

            auto complete_doc = trx_doc << stream::close_array
                 << "createdAt" << b_date{now}
                 << stream::finalize;
            mongocxx::model::insert_one insert_op{complete_doc.view()};
            transactions_in_block = true;
            bulk_trans.append(insert_op);

         }
      }
   }

   auto blk_complete = blk_doc << stream::close_array
                               << "createdAt" << b_date{now}
                               << stream::finalize;

   if (!blocks.insert_one(blk_complete.view())) {
      elog("Failed to insert block ${bid}", ("bid", block_id));
   }

   if (transactions_in_block) {
      auto result = trans.bulk_write(bulk_trans);
      if (!result) {
         elog("Bulk transaction insert failed for block: ${bid}", ("bid", block_id));
      }
   }

   ++processed;
}

// For now providing some simple account processing to maintain eos_balance
void db_plugin_impl::update_account(const chain::message& msg) {
   using bsoncxx::builder::basic::kvp;
   using bsoncxx::builder::stream::document;
   using bsoncxx::builder::stream::open_document;
   using bsoncxx::builder::stream::close_document;
   using bsoncxx::builder::stream::finalize;
   using namespace bsoncxx::types;

   if (msg.code != config::eos_contract_name)
      return;

   if (msg.type == transfer) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      auto transfer = msg.as<types::transfer>();
      auto from_name = transfer.from.to_string();
      auto to_name = transfer.to.to_string();
      auto from_account = find_account(accounts, transfer.from);
      auto to_account = find_account(accounts, transfer.to);

      asset from_balance = asset::from_string(from_account.view()["eos_balance"].get_utf8().value.to_string());
      asset to_balance = asset::from_string(to_account.view()["eos_balance"].get_utf8().value.to_string());
      from_balance -= eosio::types::share_type(transfer.amount);
      to_balance += eosio::types::share_type(transfer.amount);

      document update_from{};
      update_from << "$set" << open_document << "eos_balance" << from_balance.to_string()
                  << "updatedAt" << b_date{now}
                  << close_document;
      document update_to{};
      update_to << "$set" << open_document << "eos_balance" << to_balance.to_string()
                << "updatedAt" << b_date{now}
                << close_document;

      accounts.update_one(document{} << "_id" << from_account.view()["_id"].get_oid() << finalize, update_from.view());
      accounts.update_one(document{} << "_id" << to_account.view()["_id"].get_oid() << finalize, update_to.view());

   } else if (msg.type == newaccount) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      auto newaccount = msg.as<types::newaccount>();

      // find creator to update its balance
      auto from_name = newaccount.creator.to_string();
      document find_from{};
      find_from << "name" << from_name;
      auto from_account = accounts.find_one(find_from.view());
      if (!from_account) {
         elog("Unable to find account ${n}", ("n", from_name));
         return;
      }
      // decrease creator by deposit amount
      auto from_view = from_account->view();
      asset from_balance = asset::from_string(from_view["eos_balance"].get_utf8().value.to_string());
      from_balance -= newaccount.deposit;
      document update_from{};
      update_from << "$set" << open_document << "eos_balance" << from_balance.to_string() << close_document;
      accounts.update_one(find_from.view(), update_from.view());

      // create new account with staked deposit amount
      bsoncxx::builder::stream::document doc{};
      doc << "name" << newaccount.name.to_string()
          << "eos_balance" << asset().to_string()
          << "staked_balance" << newaccount.deposit.to_string()
          << "unstaking_balance" << asset().to_string()
          << "createdAt" << b_date{now}
          << "updatedAt" << b_date{now};
      if (!accounts.insert_one(doc.view())) {
         elog("Failed to insert account ${n}", ("n", newaccount.name));
      }

   } else if (msg.type == lock) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      auto lock = msg.as<types::lock>();
      auto from_account = find_account(accounts, lock.from);
      auto to_account = find_account(accounts, lock.to);

      asset from_balance = asset::from_string(from_account.view()["eos_balance"].get_utf8().value.to_string());
      asset to_balance = asset::from_string(to_account.view()["stacked_balance"].get_utf8().value.to_string());
      from_balance -= lock.amount;
      to_balance += lock.amount;

      document update_from{};
      update_from << "$set" << open_document << "eos_balance" << from_balance.to_string()
                  << "updatedAt" << b_date{now}
                  << close_document;
      document update_to{};
      update_to << "$set" << open_document << "stacked_balance" << to_balance.to_string()
                << "updatedAt" << b_date{now}
                << close_document;

      accounts.update_one(document{} << "_id" << from_account.view()["_id"].get_oid() << finalize, update_from.view());
      accounts.update_one(document{} << "_id" << to_account.view()["_id"].get_oid() << finalize, update_to.view());

   } else if (msg.type == unlock) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      auto unlock = msg.as<types::unlock>();
      auto from_account = find_account(accounts, unlock.account);

      asset unstack_balance = asset::from_string(from_account.view()["unstacking_balance"].get_utf8().value.to_string());
      asset stack_balance = asset::from_string(from_account.view()["stacked_balance"].get_utf8().value.to_string());
      auto deltaStake = unstack_balance - unlock.amount;
      stack_balance += deltaStake;
      unstack_balance = unlock.amount;
      // TODO: proxies and last_unstaking_time

      document update_from{};
      update_from << "$set" << open_document
                  << "staked_balance" << stack_balance.to_string()
                  << "unstacking_balance" << unstack_balance.to_string()
                  << "updatedAt" << b_date{now}
                  << close_document;

      accounts.update_one(document{} << "_id" << from_account.view()["_id"].get_oid() << finalize, update_from.view());

   } else if (msg.type == claim) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      auto claim = msg.as<types::claim>();
      auto from_account = find_account(accounts, claim.account);

      asset balance = asset::from_string(from_account.view()["eos_balance"].get_utf8().value.to_string());
      asset unstack_balance = asset::from_string(from_account.view()["unstacking_balance"].get_utf8().value.to_string());
      unstack_balance -= claim.amount;
      balance += claim.amount;

      document update_from{};
      update_from << "$set" << open_document
                  << "eos_balance" << balance.to_string()
                  << "unstacking_balance" << unstack_balance.to_string()
                  << "updatedAt" << b_date{now}
                  << close_document;

      accounts.update_one(document{} << "_id" << from_account.view()["_id"].get_oid() << finalize, update_from.view());

   } else if (msg.type == setcode) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      auto setcode = msg.as<types::setcode>();
      auto from_account = find_account(accounts, setcode.account);

      document update_from{};
      update_from << "$set" << open_document
                  << "abi" << bsoncxx::from_json(fc::json::to_string(setcode.code_abi))
                  << "updatedAt" << b_date{now}
                  << close_document;

      accounts.update_one(document{} << "_id" << from_account.view()["_id"].get_oid() << finalize, update_from.view());
   }
}

bool db_plugin_impl::is_scope_relevant(const eosio::types::vector<account_name>& scope)
{
   for (const account_name& account_name : scope)
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
      condtion.notify_one();

      consum_thread.join();
   } catch (std::exception& e) {
      elog("Exception on db_plugin shutdown of consum thread: ${e}", ("e", e.what()));
   }
}

void db_plugin_impl::wipe_database() {
   ilog("db wipe_database");

   accounts = mongo_conn[db_name][accounts_col]; // Accounts
   auto trans = mongo_conn[db_name][trans_col]; // Transactions
   auto msgs = mongo_conn[db_name][msgs_col]; // Messages
   auto blocks = mongo_conn[db_name][blocks_col]; // Blocks

   accounts.drop();
   trans.drop();
   msgs.drop();
   blocks.drop();
}

void db_plugin_impl::init() {
   using namespace bsoncxx::types;
   // Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   // See native_contract_chain_initializer::prepare_database()

   eos_abi = native_contract::native_contract_chain_initializer::eos_contract_abi();

   accounts = mongo_conn[db_name][accounts_col]; // Accounts
   bsoncxx::builder::stream::document doc{};
   if (accounts.count(doc.view()) == 0) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      doc << "name" << config::eos_contract_name.to_string()
          << "eos_balance" << asset(config::initial_token_supply).to_string()
          << "staked_balance" << asset().to_string()
          << "unstaking_balance" << asset().to_string()
          << "abi" << bsoncxx::from_json(fc::json::to_string(eos_abi))
          << "createdAt" << b_date{now}
          << "updatedAt" << b_date{now};

      if (!accounts.insert_one(doc.view())) {
         elog("Failed to insert account ${n}", ("n", config::eos_contract_name.to_string()));
      }

      // Accounts indexes
      accounts.create_index(bsoncxx::from_json(R"xxx({ "name" : 1 })xxx"));

      // Transactions indexes
      auto trans = mongo_conn[db_name][trans_col]; // Transactions
      trans.create_index(bsoncxx::from_json(R"xxx({ "transaction_id" : 1 })xxx"));

      // Messages indexes
      auto msgs = mongo_conn[db_name][msgs_col]; // Messages
      msgs.create_index(bsoncxx::from_json(R"xxx({ "message_id" : 1 })xxx"));
      msgs.create_index(bsoncxx::from_json(R"xxx({ "transaction_id" : 1 })xxx"));

      // Blocks indexes
      auto blocks = mongo_conn[db_name][blocks_col]; // Blocks
      blocks.create_index(bsoncxx::from_json(R"xxx({ "block_num" : 1 })xxx"));
      blocks.create_index(bsoncxx::from_json(R"xxx({ "block_id" : 1 })xxx"));
   }
}

#endif /* MONGODB */
////////////
// db_plugin
////////////

db_plugin::db_plugin()
:my(new db_plugin_impl)
{
}

db_plugin::~db_plugin()
{
}

void db_plugin::set_program_options(options_description& cli, options_description& cfg)
{
#ifdef MONGODB
   cfg.add_options()
         ("filter-on-accounts,f", bpo::value<std::vector<std::string>>()->composing(),
          "Track only transactions whose scopes involve the listed accounts. Default is to track all transactions.")
         ("queue-size,q", bpo::value<uint>()->default_value(256),
         "The queue size between EOSd and MongoDB process thread.")
         ("mongodb-uri,m", bpo::value<std::string>(),
         "MongoDB URI connection string, see: https://docs.mongodb.com/master/reference/connection-string/."
               " If not specified then plugin is disabled. Default database 'EOS' is used if not specified in URI.")
         ;
#endif
}

void db_plugin::wipe_database() {
#ifdef MONGODB
   if (!my->startup) {
      elog("ERROR: db_plugin::wipe_database() called before configuration or after startup. Ignoring.");
   } else {
      my->wipe_database_on_startup = true;
   }
#endif
}

void db_plugin::applied_irreversible_block(const signed_block& block) {
#ifdef MONGODB
   my->applied_irreversible_block(block);
#endif
}


void db_plugin::plugin_initialize(const variables_map& options)
{
#ifdef MONGODB
   if (options.count("mongodb-uri")) {
      ilog("initializing db plugin");
      my->configured = true;
      if (options.count("filter-on-accounts")) {
         auto foa = options.at("filter-on-accounts").as<std::vector<std::string>>();
         for (auto filter_account : foa)
            my->filter_on.emplace(filter_account);
      } else if (options.count("queue-size")) {
         auto size = options.at("queue-size").as<uint>();
         my->queue_size = size;
      }

      std::string uri_str = options.at("mongodb-uri").as<std::string>();
      ilog("connecting to ${u}", ("u", uri_str));
      mongocxx::uri uri = mongocxx::uri{uri_str};
      my->db_name = uri.database();
      if (my->db_name.empty())
         my->db_name = "EOS";
      my->mongo_conn = mongocxx::client{uri};

      if (my->wipe_database_on_startup) {
         my->wipe_database();
      }
      my->init();
   } else {
      wlog("eosio::db_plugin configured, but no --mongodb-uri specified.");
      wlog("db_plugin disabled.");
   }
#endif
}

void db_plugin::plugin_startup()
{
#ifdef MONGODB
   if (my->configured) {
      ilog("starting db plugin");

      my->consum_thread = boost::thread([this] { my->consum_blocks(); });

      // chain_controller is created and has resynced or replayed if needed
      my->startup = false;
   }
#endif
}

void db_plugin::plugin_shutdown()
{
   my.reset();
}

} // namespace eosio
