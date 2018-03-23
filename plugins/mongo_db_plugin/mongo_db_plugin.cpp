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
using chain::transaction;
using chain::signed_transaction;
using chain::signed_block;
using chain::block_trace;
using chain::transaction_id_type;

static appbase::abstract_plugin& _mongo_db_plugin = app().register_plugin<mongo_db_plugin>();

class mongo_db_plugin_impl {
public:
   mongo_db_plugin_impl();
   ~mongo_db_plugin_impl();

   void applied_block(const block_trace&);
   void applied_irreversible_block(const signed_block&);
   void process_block(const block_trace&, const signed_block&);
   void _process_block(const block_trace&, const signed_block&);
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
   std::deque<signed_block> signed_block_queue;
   std::deque<signed_block> signed_block_process_queue;
   std::deque<std::pair<block_trace, signed_block>> block_trace_queue;
   std::deque<std::pair<block_trace, signed_block>> block_trace_process_queue;
   // transaction.id -> actions
   std::map<std::string, std::vector<chain::action>> reversible_actions;
   boost::mutex mtx;
   boost::condition_variable condtion;
   boost::thread consume_thread;
   boost::atomic<bool> done{false};
   boost::atomic<bool> startup{true};

   void consume_blocks();

   void update_account(const chain::action& msg);

   static const account_name newaccount;
   static const account_name transfer;
   static const account_name setabi;

   static const std::string blocks_col;
   static const std::string trans_col;
   static const std::string actions_col;
   static const std::string action_traces_col;
   static const std::string accounts_col;
};

const account_name mongo_db_plugin_impl::newaccount = "newaccount";
const account_name mongo_db_plugin_impl::transfer = "transfer";
const account_name mongo_db_plugin_impl::setabi = "setabi";

const std::string mongo_db_plugin_impl::blocks_col = "Blocks";
const std::string mongo_db_plugin_impl::trans_col = "Transactions";
const std::string mongo_db_plugin_impl::actions_col = "Actions";
const std::string mongo_db_plugin_impl::action_traces_col = "ActionTraces";
const std::string mongo_db_plugin_impl::accounts_col = "Accounts";


void mongo_db_plugin_impl::applied_irreversible_block(const signed_block& block) {
   try {
      if (startup) {
         // on startup we don't want to queue, instead push back on caller
         process_irreversible_block(block);
      } else {
         boost::mutex::scoped_lock lock(mtx);
         signed_block_queue.push_back(block);
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

void mongo_db_plugin_impl::applied_block(const block_trace& bt) {
   try {
      if (startup) {
         // on startup we don't want to queue, instead push back on caller
         process_block(bt, bt.block);
      } else {
         boost::mutex::scoped_lock lock(mtx);
         block_trace_queue.emplace_back(std::make_pair(bt, bt.block));
         lock.unlock();
         condtion.notify_one();
      }
   } catch (fc::exception& e) {
      elog("FC Exception while applied_block ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while applied_block ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while applied_block");
   }
}

void mongo_db_plugin_impl::consume_blocks() {
   try {
      while (true) {
         boost::mutex::scoped_lock lock(mtx);
         while (signed_block_queue.empty() && block_trace_queue.empty() && !done) {
            condtion.wait(lock);
         }
         // capture blocks for processing
         size_t block_trace_size = block_trace_queue.size();
         if (block_trace_size > 0) {
            block_trace_process_queue = move(block_trace_queue);
            block_trace_queue.clear();
         }
         size_t signed_block_size = signed_block_queue.size();
         if (signed_block_size > 0) {
            signed_block_process_queue = move(signed_block_queue);
            signed_block_queue.clear();
         }

         lock.unlock();

         // warn if queue size greater than 75%
         if (signed_block_size > (queue_size * 0.75) || block_trace_size > (queue_size * 0.75)) {
            wlog("queue size: ${q}", ("q", signed_block_size + block_trace_size + 1));
         } else if (done) {
            ilog("draining queue, size: ${q}", ("q", signed_block_size + block_trace_size + 1));
         }

         // process block traces
         while (!block_trace_process_queue.empty()) {
            const auto& bt_pair = block_trace_process_queue.front();
            process_block(bt_pair.first, bt_pair.second);
            block_trace_process_queue.pop_front();
         }

         // process blocks
         while (!signed_block_process_queue.empty()) {
            const signed_block& block = signed_block_process_queue.front();
            process_irreversible_block(block);
            signed_block_process_queue.pop_front();
         }

         if (signed_block_size == 0 && block_trace_size == 0 && done) break;
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

   auto find_transaction(mongocxx::collection& transactions, const string& id) {
      using bsoncxx::builder::stream::document;
      document find_trans{};
      find_trans << "transaction_id" << id;
      auto transaction = transactions.find_one(find_trans.view());
      if (!transaction) {
         FC_THROW("Unable to find transaction ${id}", ("id", id));
      }
      return *transaction;
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

  void add_data(bsoncxx::builder::basic::document& msg_doc,
                mongocxx::collection& accounts,
                const chain::action& msg)
  {
     using bsoncxx::builder::basic::kvp;
     try {
        auto from_account = find_account(accounts, msg.account);
        abi_def abi;
        if (from_account.view().find("abi") != from_account.view().end()) {
           abi = fc::json::from_string(bsoncxx::to_json(from_account.view()["abi"].get_document())).as<abi_def>();
        }
        abi_serializer abis;
        if (msg.account == chain::config::system_account_name) {
           abi = chain::contracts::chain_initializer::eos_contract_abi(abi);
        }
        abis.set_abi(abi);
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

void mongo_db_plugin_impl::process_block(const block_trace& bt, const signed_block& block) {
   try {
      _process_block(bt, block);
   } catch (fc::exception& e) {
      elog("FC Exception while processing block trace ${e}", ("e", e.to_string()));
   } catch (std::exception& e) {
      elog("STD Exception while processing block trace ${e}", ("e", e.what()));
   } catch (...) {
      elog("Unknown exception while processing trace block");
   }
}

void mongo_db_plugin_impl::_process_block(const block_trace& bt, const signed_block& block) {
   // note bt.block is invalid at this point since it is a reference to internal chainbase block
   using namespace bsoncxx::types;
   using namespace bsoncxx::builder;
   using bsoncxx::builder::basic::kvp;

   mongocxx::options::bulk_write bulk_opts;
   bulk_opts.ordered(false);
   mongocxx::bulk_write bulk_trans{bulk_opts};

   auto blocks = mongo_conn[db_name][blocks_col]; // Blocks
   auto trans = mongo_conn[db_name][trans_col]; // Transactions
   auto msgs = mongo_conn[db_name][actions_col]; // Actions
   auto action_traces = mongo_conn[db_name][action_traces_col]; // ActionTraces

   auto block_doc = bsoncxx::builder::basic::document{};
   const auto block_id = block.id();
   const auto block_id_str = block_id.str();
   const auto prev_block_id_str = block.previous.str();
   auto block_num = block.block_num();

   if (processed == 0) {
      if (wipe_database_on_startup) {
         // verify on start we have no previous blocks
         verify_no_blocks(blocks);
         FC_ASSERT(block_num < 2, "Expected start of block, instead received block_num: ${bn}", ("bn", block_num));
      } else {
         // verify on restart we have previous block
         verify_last_block(blocks, prev_block_id_str);
      }
   }

   auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

   block_doc.append(kvp("block_num", b_int32{static_cast<int32_t>(block_num)}),
                    kvp("block_id", block_id_str),
                    kvp("prev_block_id", prev_block_id_str),
                    kvp("timestamp", b_date{std::chrono::milliseconds{
                          std::chrono::seconds{block.timestamp.operator fc::time_point().sec_since_epoch()}}}),
                    kvp("transaction_merkle_root", block.transaction_mroot.str()),
                    kvp("producer_account_id", block.producer.to_string()),
                    kvp("pending", b_bool{true}));
   block_doc.append(kvp("createdAt", b_date{now}));

   if (!blocks.insert_one(block_doc.view())) {
      elog("Failed to insert block ${bid}", ("bid", block_id));
   }

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
                     kvp("receiver", act.receiver.to_string()),
                     kvp("action", b_oid{msg_oid}),
                     kvp("console", act.console),
                     kvp("region_id", b_int64{act.region_id}),
                     kvp("cycle_index", b_int64{act.cycle_index}));
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

   mongocxx::bulk_write bulk_msgs{bulk_opts};
   mongocxx::bulk_write bulk_acts{bulk_opts};
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
               
               for (const auto& trx : trx_trace.deferred_transactions) {
                  auto doc = process_trx(trx);
                  doc.append(kvp("type", "deferred"),
                             kvp("sender_id", b_int64{trx.sender_id}),
                             kvp("sender", trx.sender.to_string()),
                             kvp("execute_after", b_date{std::chrono::milliseconds{
                                   std::chrono::seconds{trx.execute_after.sec_since_epoch()}}}));
                  mongocxx::model::insert_one insert_op{doc.view()};
                  bulk_trans.append(insert_op);
                  ++trx_num;
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

   ++processed;
}

void mongo_db_plugin_impl::_process_irreversible_block(const signed_block& block)
{
   using namespace bsoncxx::types;
   using namespace bsoncxx::builder;
   using bsoncxx::builder::basic::kvp;
   using bsoncxx::builder::stream::document;
   using bsoncxx::builder::stream::open_document;
   using bsoncxx::builder::stream::close_document;
   using bsoncxx::builder::stream::finalize;


   auto blocks = mongo_conn[db_name][blocks_col]; // Blocks
   auto trans = mongo_conn[db_name][trans_col]; // Transactions

   const auto block_id = block.id();
   const auto block_id_str = block_id.str();

   auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
         std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});

   auto ir_block = find_block(blocks, block_id_str);

   document update_block{};
   update_block << "$set" << open_document << "pending" << b_bool{false}
               << "updatedAt" << b_date{now}
               << close_document;

   blocks.update_one(document{} << "_id" << ir_block.view()["_id"].get_oid() << finalize, update_block.view());

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

      abi_serializer abis;
      auto eosio_account = find_account(accounts, msg.account);
      auto abi = fc::json::from_string(bsoncxx::to_json(eosio_account.view()["abi"].get_document())).as<abi_def>();
      abis.set_abi(abi);
      auto transfer = abis.binary_to_variant(abis.get_action_type(msg.name), msg.data);
      auto from_name = transfer["from"].as<name>().to_string();
      auto to_name = transfer["to"].as<name>().to_string();
      auto from_account = find_account(accounts, from_name);
      auto to_account = find_account(accounts, to_name);

      asset from_balance = asset::from_string(from_account.view()["eos_balance"].get_utf8().value.to_string());
      asset to_balance = asset::from_string(to_account.view()["eos_balance"].get_utf8().value.to_string());
      auto asset_quantity = transfer["quantity"].as<asset>();
      edump((from_balance)(to_balance)(asset_quantity));
      from_balance -= asset_quantity;
      to_balance += asset_quantity;

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

   } else if (msg.name == newaccount) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      auto newaccount = msg.data_as<chain::contracts::newaccount>();

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
      auto setabi = msg.data_as<chain::contracts::setabi>();
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

      consume_thread.join();
   } catch (std::exception& e) {
      elog("Exception on mongo_db_plugin shutdown of consume thread: ${e}", ("e", e.what()));
   }
}

void mongo_db_plugin_impl::wipe_database() {
   ilog("mongo db wipe_database");

   accounts = mongo_conn[db_name][accounts_col]; // Accounts
   auto blocks = mongo_conn[db_name][blocks_col]; // Blocks
   auto trans = mongo_conn[db_name][trans_col]; // Transactions
   auto msgs = mongo_conn[db_name][actions_col]; // Actions
   auto action_traces = mongo_conn[db_name][action_traces_col]; // ActionTraces

   blocks.drop();
   trans.drop();
   accounts.drop();
   msgs.drop();
   action_traces.drop();
}

void mongo_db_plugin_impl::init() {
   using namespace bsoncxx::types;
   // Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   // See native_contract_chain_initializer::prepare_database()

   accounts = mongo_conn[db_name][accounts_col]; // Accounts
   bsoncxx::builder::stream::document doc{};
   if (accounts.count(doc.view()) == 0) {
      auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()});
      doc << "name" << name(chain::config::system_account_name).to_string()
          << "eos_balance" << asset().to_string()
          << "staked_balance" << asset().to_string()
          << "unstaking_balance" << asset().to_string()
          << "createdAt" << b_date{now}
          << "updatedAt" << b_date{now};

      if (!accounts.insert_one(doc.view())) {
         elog("Failed to insert account ${n}", ("n", name(chain::config::system_account_name).to_string()));
      }

      // Blocks indexes
      auto blocks = mongo_conn[db_name][blocks_col]; // Blocks
      blocks.create_index(bsoncxx::from_json(R"xxx({ "block_num" : 1 })xxx"));
      blocks.create_index(bsoncxx::from_json(R"xxx({ "block_id" : 1 })xxx"));

      // Accounts indexes
      accounts.create_index(bsoncxx::from_json(R"xxx({ "name" : 1 })xxx"));

      // Transactions indexes
      auto trans = mongo_conn[db_name][trans_col]; // Transactions
      trans.create_index(bsoncxx::from_json(R"xxx({ "transaction_id" : 1 })xxx"));

      // Action indexes
      auto msgs = mongo_conn[db_name][actions_col]; // Messages
      msgs.create_index(bsoncxx::from_json(R"xxx({ "action_id" : 1 })xxx"));
      msgs.create_index(bsoncxx::from_json(R"xxx({ "transaction_id" : 1 })xxx"));

      // ActionTraces indexes
      auto action_traces = mongo_conn[db_name][action_traces_col]; // ActionTraces
      action_traces.create_index(bsoncxx::from_json(R"xxx({ "transaction_id" : 1 })xxx"));
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
      chain_plug->chain_config().applied_block_callbacks.emplace_back(
            [my = my](const chain::block_trace& bt) { my->applied_block(bt); });
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

      my->consume_thread = boost::thread([this] { my->consume_blocks(); });

      // chain_controller is created and has resynced or replayed if needed
      my->startup = false;
   }
}

void mongo_db_plugin::plugin_shutdown()
{
   my.reset();
}

} // namespace eosio
