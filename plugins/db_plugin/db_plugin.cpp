#include <eos/db_plugin/db_plugin.hpp>
#include <eos/chain/config.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/transaction.hpp>
#include <eos/chain/types.hpp>
#include <eos/native_contract/native_contract_chain_initializer.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#ifdef MONGODB
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#endif

namespace fc { class variant; }

namespace eos {

using chain::AccountName;
using chain::FuncName;
using chain::block_id_type;
using chain::PermissionName;
using chain::ProcessedTransaction;
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

   std::set<AccountName> filter_on;
   static types::Abi eos_abi; // cached for common use

   bool configured{false};
   bool wipe_database_on_startup{false};
   mongocxx::instance mongo_inst;
   mongocxx::client mongo_conn;
   mongocxx::collection accounts;

   size_t queue_size = 0;
   size_t processed = 0;
   std::unique_ptr<boost::lockfree::spsc_queue<signed_block>> queue;
   boost::thread consum_thread;
   boost::atomic<bool> startup{true};
   boost::atomic<bool> done{false};

   void consum_blocks();

   bool is_scope_relevant(const eos::types::Vector<AccountName>& scope);
   void update_account(const chain::Message& msg);

   static const FuncName newaccount;
   static const FuncName transfer;
   static const FuncName lock;
   static const FuncName unlock;
   static const FuncName claim;
   static const FuncName setcode;

   static const std::string db_name;
   static const std::string blocks_col;
   static const std::string trans_col;
   static const std::string msgs_col;
   static const std::string accounts_col;
};

types::Abi db_plugin_impl::eos_abi;

const FuncName db_plugin_impl::newaccount = "newaccount";
const FuncName db_plugin_impl::transfer = "transfer";
const FuncName db_plugin_impl::lock = "lock";
const FuncName db_plugin_impl::unlock = "unlock";
const FuncName db_plugin_impl::claim = "claim";
const FuncName db_plugin_impl::setcode = "setcode";

const std::string db_plugin_impl::db_name = "EOS";
const std::string db_plugin_impl::blocks_col = "Blocks";
const std::string db_plugin_impl::trans_col = "Transactions";
const std::string db_plugin_impl::msgs_col = "Messages";
const std::string db_plugin_impl::accounts_col = "Accounts";


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
         auto available = queue->read_available();
         // warn if queue size greater than 75%
         if (available > (queue_size * 0.75)) {
            wlog("queue size: ${q}", ("q", available + 1));
         }
         process_irreversible_block(block);
      }
   }
   while (queue->pop(block)) {
      process_irreversible_block(block);
   }
   ilog("db_plugin consum thread shutdown gracefully");
}

namespace {

   auto find_account(mongocxx::collection& accounts, const AccountName& name) {
      using bsoncxx::builder::stream::document;
      document find_acc{};
      find_acc << "name" << name.toString();
      auto account = accounts.find_one(find_acc.view());
      if (!account) {
         FC_THROW("Unable to find account ${n}", ("n", name));
      }
      return *account;
   }

  void add_data(bsoncxx::builder::basic::document& msg_doc,
                mongocxx::collection& accounts,
                const chain::Message& msg)
  {
     using bsoncxx::builder::basic::kvp;
     types::AbiSerializer abis;
     if (msg.code == config::EosContractName) {
        abis.setAbi(db_plugin_impl::eos_abi);
     } else {
        auto from_account = find_account(accounts, msg.code);
        auto abi = fc::json::from_string(from_account.view()["abi"].get_utf8().value.to_string()).as<types::Abi>();
        abis.setAbi(abi);
     }
     try {
        auto v = abis.binaryToVariant(abis.getActionType(msg.type), msg.data);
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
        elog("Unable to convert Message.data to ABI type: ${t}, what: ${e}", ("t", msg.type)("e", e.to_string()));
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

   // verify on restart we have previous block
   if (processed == 0 && !startup)
      verify_last_block(blocks, prev_block_id_str);

   block_doc << "block_num" << b_int32{static_cast<int32_t>(block.block_num())}
       << "block_id" << block_id_str
       << "prev_block_id" << prev_block_id_str
       << "timestamp" << b_date{std::chrono::milliseconds{std::chrono::seconds{block.timestamp.sec_since_epoch()}}}
       << "transaction_merkle_root" << block.transaction_merkle_root.str()
       << "producer_account_id" << block.producer.toString();
   if (!blocks.insert_one(block_doc.view())) {
      elog("Failed to insert block ${bid}", ("bid", block_id));
   }

   int32_t trx_num = -1;
   const bool check_relevance = !filter_on.empty();
   for (const auto& cycle : block.cycles) {
      for (const auto& thread : cycle) {
         for (const auto& trx : thread.user_input) {
            ++trx_num;
            if (check_relevance && !is_scope_relevant(trx.scope))
               continue;

            stream::document doc{};
            const auto trans_id_str = trx.id().str();
            auto trx_doc = doc
                  << "transaction_id" << trans_id_str
                  << "sequence_num" << b_int32{trx_num}
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
               trx_doc = trx_doc << fc::variant(sig).as_string();
            }
            trx_doc = trx_doc
                  << stream::close_array
                  << "messages" << stream::open_array;

            mongocxx::bulk_write bulk_msgs{bulk_opts};
            int32_t i = 0;
            for (const chain::Message& msg : trx.messages) {
               auto msg_oid = bsoncxx::oid{};
               trx_doc = trx_doc << msg_oid; // add to transaction.messages array

               auto msg_doc = bsoncxx::builder::basic::document{};
               msg_doc.append(kvp("_id", b_oid{msg_oid}),
                              kvp("message_id", b_int32{i}),
                              kvp("transaction_id", trans_id_str));
               msg_doc.append(kvp("authorization", [&msg](bsoncxx::builder::basic::sub_array subarr) {
                  for (const auto& auth : msg.authorization) {
                     subarr.append([&auth](bsoncxx::builder::basic::sub_document subdoc) {
                        subdoc.append(kvp("account", auth.account.toString()),
                                      kvp("permission", auth.permission.toString()));
                     });
                  }
               }));
               msg_doc.append(kvp("handler_account_name", msg.code.toString()));
               msg_doc.append(kvp("type", msg.type.toString()));
               add_data(msg_doc, accounts, msg);
               mongocxx::model::insert_one insert_msg{msg_doc.view()};
               bulk_msgs.append(insert_msg);

               // eos account update
               if (msg.code == config::EosContractName) {
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

            auto complete_doc = trx_doc << stream::close_array << stream::finalize;
            mongocxx::model::insert_one insert_op{complete_doc.view()};
            transactions_in_block = true;
            bulk_trans.append(insert_op);

         }
      }
   }
   
   if (transactions_in_block) {
      auto result = trans.bulk_write(bulk_trans);
      if (!result) {
         elog("Bulk transaction insert failed for block: ${bid}", ("bid", block_id));
      }
   }

   ++processed;
}


void db_plugin_impl::update_account(const chain::Message& msg) {
   using bsoncxx::builder::basic::kvp;
   using bsoncxx::builder::stream::document;
   using bsoncxx::builder::stream::open_document;
   using bsoncxx::builder::stream::close_document;
   using bsoncxx::builder::stream::finalize;
   using namespace bsoncxx::types;

   if (msg.code != config::EosContractName)
      return;

   if (msg.type == transfer) {
      auto transfer = msg.as<types::transfer>();
      auto from_name = transfer.from.toString();
      auto to_name = transfer.to.toString();
      auto from_account = find_account(accounts, transfer.from);
      auto to_account = find_account(accounts, transfer.to);

      Asset from_balance = Asset::fromString(from_account.view()["eos_balance"].get_utf8().value.to_string());
      Asset to_balance = Asset::fromString(to_account.view()["eos_balance"].get_utf8().value.to_string());
      from_balance -= eos::types::ShareType(transfer.amount);
      to_balance += eos::types::ShareType(transfer.amount);

      document update_from{};
      update_from << "$set" << open_document << "eos_balance" << from_balance.toString() << close_document;
      document update_to{};
      update_to << "$set" << open_document << "eos_balance" << to_balance.toString() << close_document;

      accounts.update_one(document{} << "_id" << from_account.view()["_id"].get_oid() << finalize, update_from.view());
      accounts.update_one(document{} << "_id" << to_account.view()["_id"].get_oid() << finalize, update_to.view());

   } else if (msg.type == newaccount) {
      auto newaccount = msg.as<types::newaccount>();

      // find creator to update its balance
      auto from_name = newaccount.creator.toString();
      document find_from{};
      find_from << "name" << from_name;
      auto from_account = accounts.find_one(find_from.view());
      if (!from_account) {
         elog("Unable to find account ${n}", ("n", from_name));
         return;
      }
      // decrease creator by deposit amount
      auto from_view = from_account->view();
      Asset from_balance = Asset::fromString(from_view["eos_balance"].get_utf8().value.to_string());
      from_balance -= newaccount.deposit;
      document update_from{};
      update_from << "$set" << open_document << "eos_balance" << from_balance.toString() << close_document;
      accounts.update_one(find_from.view(), update_from.view());

      // create new account with staked deposit amount
      bsoncxx::builder::stream::document doc{};
      doc << "name" << newaccount.name.toString()
          << "eos_balance" << Asset().toString()
          << "staked_balance" << newaccount.deposit.toString()
          << "unstaked_balance" << Asset().toString();
      if (!accounts.insert_one(doc.view())) {
         elog("Failed to insert account ${n}", ("n", newaccount.name));
      }

   } else if (msg.type == lock) {
      auto lock = msg.as<types::lock>();
      auto from_account = find_account(accounts, lock.from);
      auto to_account = find_account(accounts, lock.to);

      Asset from_balance = Asset::fromString(from_account.view()["eos_balance"].get_utf8().value.to_string());
      Asset to_balance = Asset::fromString(to_account.view()["stacked_balance"].get_utf8().value.to_string());
      from_balance -= lock.amount;
      to_balance += lock.amount;

      document update_from{};
      update_from << "$set" << open_document << "eos_balance" << from_balance.toString() << close_document;
      document update_to{};
      update_to << "$set" << open_document << "stacked_balance" << to_balance.toString() << close_document;

      accounts.update_one(document{} << "_id" << from_account.view()["_id"].get_oid() << finalize, update_from.view());
      accounts.update_one(document{} << "_id" << to_account.view()["_id"].get_oid() << finalize, update_to.view());

   } else if (msg.type == unlock) {
      auto unlock = msg.as<types::unlock>();
      auto from_account = find_account(accounts, unlock.account);

      Asset unstack_balance = Asset::fromString(from_account.view()["unstacking_balance"].get_utf8().value.to_string());
      Asset stack_balance = Asset::fromString(from_account.view()["stacked_balance"].get_utf8().value.to_string());
      auto deltaStake = unstack_balance - unlock.amount;
      stack_balance += deltaStake;
      unstack_balance = unlock.amount;
      // TODO: proxies and last_unstaking_time

      document update_from{};
      update_from << "$set" << open_document
                  << "staked_balance" << stack_balance.toString()
                  << "unstacking_balance" << unstack_balance.toString()
                  << close_document;

      accounts.update_one(document{} << "_id" << from_account.view()["_id"].get_oid() << finalize, update_from.view());

   } else if (msg.type == claim) {
      auto claim = msg.as<types::claim>();
      auto from_account = find_account(accounts, claim.account);

      Asset balance = Asset::fromString(from_account.view()["eos_balance"].get_utf8().value.to_string());
      Asset unstack_balance = Asset::fromString(from_account.view()["unstacking_balance"].get_utf8().value.to_string());
      unstack_balance -= claim.amount;
      balance += claim.amount;

      document update_from{};
      update_from << "$set" << open_document
                  << "eos_balance" << balance.toString()
                  << "unstacking_balance" << unstack_balance.toString()
                  << close_document;

      accounts.update_one(document{} << "_id" << from_account.view()["_id"].get_oid() << finalize, update_from.view());

   } else if (msg.type == setcode) {
      auto setcode = msg.as<types::setcode>();
      auto from_account = find_account(accounts, setcode.account);

      document update_from{};
      update_from << "$set" << open_document
                  << "abi" << fc::json::to_string(setcode.abi)
                  << close_document;

      accounts.update_one(document{} << "_id" << from_account.view()["_id"].get_oid() << finalize, update_from.view());
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

void db_plugin_impl::init() {
   // Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   // See native_contract_chain_initializer::prepare_database()

   eos_abi = native_contract::native_contract_chain_initializer::eos_contract_abi();

   accounts = mongo_conn[db_name][accounts_col]; // Accounts
   bsoncxx::builder::stream::document doc{};
   if (accounts.count(doc.view()) == 0) {
      doc << "name" << config::EosContractName.toString()
          << "eos_balance" << Asset(config::InitialTokenSupply).toString()
          << "staked_balance" << Asset().toString()
          << "unstaked_balance" << Asset().toString()
          << "abi" << fc::json::to_string(eos_abi);
      if (!accounts.insert_one(doc.view())) {
         elog("Failed to insert account ${n}", ("n", config::EosContractName.toString()));
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
         ("mongodb-uri,m", bpo::value<std::string>()->default_value("mongodb://localhost:27017"),
         "MongoDB URI connection string, see: https://docs.mongodb.com/master/reference/connection-string/")
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
   ilog("initializing db plugin");
   if(options.count("filter-on-accounts")) {
      auto foa = options.at("filter-on-accounts").as<std::vector<std::string>>();
      for(auto filter_account : foa)
         my->filter_on.emplace(filter_account);
   } else if (options.count("queue-size")) {
      auto size = options.at("queue-size").as<uint>();
      my->queue_size = size;
      my->queue = std::make_unique<boost::lockfree::spsc_queue<signed_block>>(size);
   }

   std::string uri = options.at("mongodb-uri").as<std::string>();
   ilog("connecting to ${u}", ("u", uri));
   my->mongo_conn = mongocxx::client{mongocxx::uri{uri}};

   if (my->wipe_database_on_startup) {
      my->wipe_database();
   }
   my->init();
#endif
}

void db_plugin::plugin_startup()
{
#ifdef MONGODB
   ilog("starting db plugin");

   my->consum_thread = boost::thread([this]{ my->consum_blocks(); });

   // chain_controller is created and has resynced or replayed if needed
   my->startup = false;
#endif
}

void db_plugin::plugin_shutdown()
{
   my.reset();
}

} // namespace eos
