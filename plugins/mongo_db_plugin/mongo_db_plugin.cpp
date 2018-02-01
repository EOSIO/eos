/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/mongo_db_plugin/mongo_db_plugin.hpp>
#include <eosio/chain/contracts/chain_initializer.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

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
using chain::signed_transaction;
using chain::signed_block;
using chain::transaction_id_type;

static appbase::abstract_plugin& _mongo_db_plugin = app().register_plugin<mongo_db_plugin>();

class mongo_db_plugin_impl {
public:
   mongo_db_plugin_impl();
   ~mongo_db_plugin_impl();

   void applied_irreversible_block(const signed_block&);
   void process_irreversible_block(const signed_block&);
   void _process_irreversible_block(const signed_block&);

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
   std::queue<signed_block> queue;
   boost::mutex mtx;
   boost::condition_variable condtion;
   boost::thread consum_thread;
   boost::atomic<bool> done{false};
   boost::atomic<bool> startup{true};

   void consum_blocks();

   bool is_scope_relevant(const vector<account_name>& scope);
   void update_account(const chain::action& msg);

   static const account_name newaccount;
   static const account_name transfer;
   static const account_name lock;
   static const account_name unlock;
   static const account_name claim;
   static const account_name setcode;
   static const account_name setabi;

   static const std::string blocks_col;
   static const std::string trans_col;
   static const std::string actions_col;
   static const std::string accounts_col;
};

abi_def mongo_db_plugin_impl::eos_abi;

const account_name mongo_db_plugin_impl::newaccount = "newaccount";
const account_name mongo_db_plugin_impl::transfer = "transfer";
const account_name mongo_db_plugin_impl::lock = "lock";
const account_name mongo_db_plugin_impl::unlock = "unlock";
const account_name mongo_db_plugin_impl::claim = "claim";
const account_name mongo_db_plugin_impl::setcode = "setcode";
const account_name mongo_db_plugin_impl::setabi = "setabi";

const std::string mongo_db_plugin_impl::blocks_col = "Blocks";
const std::string mongo_db_plugin_impl::trans_col = "Transactions";
const std::string mongo_db_plugin_impl::actions_col = "Actions";
const std::string mongo_db_plugin_impl::accounts_col = "Accounts";


void mongo_db_plugin_impl::applied_irreversible_block(const signed_block& block) {
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

void mongo_db_plugin_impl::consum_blocks() {
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
      ilog("mongo_db_plugin consum thread shutdown gracefully");
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
                const chain::action& msg)
  {
     using bsoncxx::builder::basic::kvp;
     try {
        abi_serializer abis;
        if (msg.account == chain::config::system_account_name) {
           abis.set_abi(mongo_db_plugin_impl::eos_abi);
        } else {
           auto from_account = find_account(accounts, msg.account);
           auto abi = fc::json::from_string(bsoncxx::to_json(from_account.view()["abi"].get_document())).as<abi_def>();
           abis.set_abi(abi);
        }
        auto v = abis.binary_to_variant(abis.get_action_type(msg.name), msg.data);
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
        elog("Unable to convert action.data to ABI: ${s} :: ${n}, what: ${e}", ("s", msg.account)("n", msg.name)("e", e.to_string()));
     } catch (std::exception& e) {
        elog("Unable to convert action.data to ABI: ${s} :: ${n}, std what: ${e}", ("s", msg.account)("n", msg.name)("e", e.what()));
     } catch (...) {
        elog("Unable to convert action.data to ABI: ${s} :: ${n}, unknown exception", ("s", msg.account)("n", msg.name));
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

void mongo_db_plugin_impl::process_irreversible_block(const signed_block& block) {
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

void mongo_db_plugin_impl::_process_irreversible_block(const signed_block& block)
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
   auto msgs = mongo_conn[db_name][actions_col]; // Actions

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
         if (block_num == 1 && block.producer == chain::config::system_account_name) {
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
       << "timestamp" << b_date{std::chrono::milliseconds{std::chrono::seconds{block.timestamp.operator fc::time_point().sec_since_epoch()}}}
       << "transaction_merkle_root" << block.transaction_mroot.str()
       << "producer_account_id" << block.producer.to_string();
   auto blk_doc = block_doc << "transactions" << stream::open_array;

   int32_t trx_num = -1;
   for (const auto& packed_trx : block.input_transactions) {
      const signed_transaction& trx = packed_trx.get_signed_transaction();
      ++trx_num;

      auto txn_oid = bsoncxx::oid{};
      blk_doc = blk_doc << txn_oid; // add to transaction.actions array
      stream::document doc{};
      const auto trans_id_str = trx.id().str();
      auto trx_doc = doc
            << "_id" << txn_oid
            << "transaction_id" << trans_id_str
            << "sequence_num" << b_int32{trx_num}
            << "block_id" << block_id_str
            << "ref_block_num" << b_int32{static_cast<int32_t >(trx.ref_block_num)}
            << "ref_block_prefix" << b_int32{static_cast<int32_t >(trx.ref_block_prefix)}
            << "expiration" << b_date{std::chrono::milliseconds{std::chrono::seconds{trx.expiration.sec_since_epoch()}}}
            << "signatures" << stream::open_array;
      for (const auto& sig : trx.signatures) {
         trx_doc = trx_doc << fc::variant(sig).as_string();
      }
      trx_doc = trx_doc
            << stream::close_array
            << "actions" << stream::open_array;

      mongocxx::bulk_write bulk_msgs{bulk_opts};
      int32_t i = 0;
      for (const auto& msg : trx.actions) {
         auto msg_oid = bsoncxx::oid{};
         trx_doc = trx_doc << msg_oid; // add to transaction.actions array

         auto msg_doc = bsoncxx::builder::basic::document{};
         msg_doc.append(kvp("_id", b_oid{msg_oid}),
                        kvp("action_id", b_int32{i}),
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

         // eos account update
         if (msg.account == chain::config::system_account_name) {
            try {
               update_account(msg);
            } catch (fc::exception& e) {
               elog("Unable to update account ${e}", ("e", e.to_string()));
            }
         }

         ++i;
      }

      if (!trx.actions.empty()) {
         auto result = msgs.bulk_write(bulk_msgs);
         if (!result) {
            elog("Bulk action insert failed for block: ${bid}, transaction: ${trx}",
                 ("bid", block_id)("trx", trx.id()));
         }
      }

      auto complete_doc = trx_doc << stream::close_array
                                  << "createdAt" << b_date{now}
                                  << stream::finalize;
      mongocxx::model::insert_one insert_op{complete_doc.view()};
      transactions_in_block = true;
      bulk_trans.append(insert_op);

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
void mongo_db_plugin_impl::update_account(const chain::action& msg) {
   using bsoncxx::builder::basic::kvp;
   using bsoncxx::builder::stream::document;
   using bsoncxx::builder::stream::open_document;
   using bsoncxx::builder::stream::close_document;
   using bsoncxx::builder::stream::finalize;
   using namespace bsoncxx::types;

   if (msg.account != chain::config::system_account_name)
      return;

   if (msg.name == transfer) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      /* todo need to follow eosio.system transfer
      auto transfer = msg.as<chain::contracts::transfer>();
      auto from_name = transfer.from.to_string();
      auto to_name = transfer.to.to_string();
      auto from_account = find_account(accounts, transfer.from);
      auto to_account = find_account(accounts, transfer.to);

      asset from_balance = asset::from_string(from_account.view()["eos_balance"].get_utf8().value.to_string());
      asset to_balance = asset::from_string(to_account.view()["eos_balance"].get_utf8().value.to_string());
      from_balance -= asset(chain::share_type(transfer.amount));
      to_balance += asset(chain::share_type(transfer.amount));

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
*/
   } else if (msg.name == newaccount) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      auto newaccount = msg.as<chain::contracts::newaccount>();

      // create new account
      bsoncxx::builder::stream::document doc{};
      doc << "name" << newaccount.name.to_string()
          << "eos_balance" << asset().to_string()
          << "staked_balance" << asset().to_string()
          << "unstaking_balance" << asset().to_string()
          << "createdAt" << b_date{now}
          << "updatedAt" << b_date{now};
      if (!accounts.insert_one(doc.view())) {
         elog("Failed to insert account ${n}", ("n", newaccount.name));
      }

   } else if (msg.name == setabi) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      auto setabi = msg.as<chain::contracts::setabi>();
      auto from_account = find_account(accounts, setabi.account);

      document update_from{};
      update_from << "$set" << open_document
                  << "abi" << bsoncxx::from_json(fc::json::to_string(setabi.abi))
                  << "updatedAt" << b_date{now}
                  << close_document;

      accounts.update_one(document{} << "_id" << from_account.view()["_id"].get_oid() << finalize, update_from.view());
   }
}

mongo_db_plugin_impl::mongo_db_plugin_impl()
: mongo_inst{}
, mongo_conn{}
{
}

mongo_db_plugin_impl::~mongo_db_plugin_impl() {
   try {
      done = true;
      condtion.notify_one();

      consum_thread.join();
   } catch (std::exception& e) {
      elog("Exception on mongo_db_plugin shutdown of consum thread: ${e}", ("e", e.what()));
   }
}

void mongo_db_plugin_impl::wipe_database() {
   ilog("mongo db wipe_database");

   accounts = mongo_conn[db_name][accounts_col]; // Accounts
   auto trans = mongo_conn[db_name][trans_col]; // Transactions
   auto msgs = mongo_conn[db_name][actions_col]; // Actions
   auto blocks = mongo_conn[db_name][blocks_col]; // Blocks

   accounts.drop();
   trans.drop();
   msgs.drop();
   blocks.drop();
}

void mongo_db_plugin_impl::init() {
   using namespace bsoncxx::types;
   // Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   // See native_contract_chain_initializer::prepare_database()

   eos_abi = chain::contracts::chain_initializer::eos_contract_abi();

   accounts = mongo_conn[db_name][accounts_col]; // Accounts
   bsoncxx::builder::stream::document doc{};
   if (accounts.count(doc.view()) == 0) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      doc << "name" << name(chain::config::system_account_name).to_string()
          << "eos_balance" << asset(chain::config::initial_token_supply).to_string()
          << "staked_balance" << asset().to_string()
          << "unstaking_balance" << asset().to_string()
          << "abi" << bsoncxx::from_json(fc::json::to_string(eos_abi))
          << "createdAt" << b_date{now}
          << "updatedAt" << b_date{now};

      if (!accounts.insert_one(doc.view())) {
         elog("Failed to insert account ${n}", ("n", name(chain::config::system_account_name).to_string()));
      }

      // Accounts indexes
      accounts.create_index(bsoncxx::from_json(R"xxx({ "name" : 1 })xxx"));

      // Transactions indexes
      auto trans = mongo_conn[db_name][trans_col]; // Transactions
      trans.create_index(bsoncxx::from_json(R"xxx({ "transaction_id" : 1 })xxx"));

      // Messages indexes
      auto msgs = mongo_conn[db_name][actions_col]; // Messages
      msgs.create_index(bsoncxx::from_json(R"xxx({ "message_id" : 1 })xxx"));
      msgs.create_index(bsoncxx::from_json(R"xxx({ "transaction_id" : 1 })xxx"));

      // Blocks indexes
      auto blocks = mongo_conn[db_name][blocks_col]; // Blocks
      blocks.create_index(bsoncxx::from_json(R"xxx({ "block_num" : 1 })xxx"));
      blocks.create_index(bsoncxx::from_json(R"xxx({ "block_id" : 1 })xxx"));
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
         "The queue size between eosiod and MongoDB plugin thread.")
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

      if (options.at("replay-blockchain").as<bool>()) {
         ilog("Replay requested: wiping mongo database on startup");
         my->wipe_database_on_startup = true;
      }
      if (options.at("resync-blockchain").as<bool>()) {
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

      // add callback to chain_controller config
      chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
      FC_ASSERT(chain_plug);
      chain_plug->chain_config().applied_irreversible_block_callbacks.emplace_back(
            [my = my](const chain::signed_block& b) { my->applied_irreversible_block(b); });

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

      my->consum_thread = boost::thread([this] { my->consum_blocks(); });

      // chain_controller is created and has resynced or replayed if needed
      my->startup = false;
   }
}

void mongo_db_plugin::plugin_shutdown()
{
   my.reset();
}

} // namespace eosio
