/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/asset.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/block_log.hpp>
#include <eosio/snapshot_plugin/snapshot_plugin.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/json.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/optional.hpp>
#include <fc/reflect/reflect.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace chainbase;
using namespace fc;
using namespace std;

struct permission {
   struct name  perm_name;
   struct name  parent;
   authority    required_auth;
};
FC_REFLECT( permission, (perm_name)(parent)(required_auth) )

template<typename Function>
void iterate_all_tables(const database& db, Function f)
{
   const auto &idx = db.get_index<table_id_multi_index, by_code_scope_table>();
   auto lower = idx.begin();
   auto upper = idx.end();

   for (auto itr = lower; itr != upper; ++itr) {
      f(*itr);
   }
}

template<typename Function>
void iterate_all_rows(const database& db, const table_id_object& t_id, Function f)
{
   const auto &idx = db.get_index<key_value_index, by_scope_primary>();
   decltype(t_id.id) next_tid(t_id.id._id + 1);
   auto lower = idx.lower_bound(boost::make_tuple(t_id.id));
   auto upper = idx.lower_bound(boost::make_tuple(next_tid));

   for (auto itr = lower; itr != upper; ++itr) {
      f(*itr);
   }
}

namespace eosio {
   static appbase::abstract_plugin& _snapshot_plugin = app().register_plugin<snapshot_plugin>();

class snapshot_plugin_impl {
   public:
};

snapshot_plugin::snapshot_plugin():my(new snapshot_plugin_impl()){}
snapshot_plugin::~snapshot_plugin(){}

void snapshot_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("snapshot-at-block", bpo::value<string>()->default_value(""), "Block hash at which to take the snapshot")
         ("snapshot-to", bpo::value<string>()->default_value("snapshot.json"), "Pathname of JSON file where to store the snapshot")
         ;
}

void snapshot_plugin::plugin_initialize(const variables_map& options) {

   auto snapshot_at_block = fc::json::from_string(options.at("snapshot-at-block").as<string>()).as<block_id_type>();
   auto snapshot_to = options.at("snapshot-to").as<string>();

   if( snapshot_at_block != block_id_type() ) {

      chain_plugin* chain_plug = app().find_plugin<chain_plugin>();
      FC_ASSERT(chain_plug);
      auto& chain = chain_plug->chain();
      auto& db = chain.db();
      const auto& config = chain_plug->chain_config();

      // Take snapshot after block is applied
      m_accepted_block_connection.emplace(chain.accepted_block.connect([this, snapshot_at_block, snapshot_to, &db, &config, &chain](const chain::block_state_ptr& b) {

         if( b->id == snapshot_at_block ) {

            ilog("Taking snapshot at block ${n} (${h})...",("h",snapshot_at_block)("n",b->block_num));

            std::ofstream out;
            try {
               out.open(snapshot_to);
               fc::raw::pack(out, snapshot_version);
               fc::raw::pack(out, chain.get_chain_id());
               fc::raw::pack(out, config.genesis);
               fc::raw::pack(out, static_cast<block_header_state>(*b));

               const auto& account_idx = db.get_index<account_index, by_name>();
               uint32_t total_accounts = std::distance(account_idx.begin(), account_idx.end());
               fc::raw::pack(out, total_accounts);
               for (auto accnt = account_idx.begin(); accnt != account_idx.end(); ++accnt) {
                  fc::raw::pack(out, *accnt);
               }

               const auto& permissions_idx = db.get_index<permission_index,by_owner>();
               uint32_t total_perms = std::distance(permissions_idx.begin(), permissions_idx.end());
               fc::raw::pack(out, total_perms);
               for (auto perm = permissions_idx.begin(); perm != permissions_idx.end(); ++perm) {
                  fc::raw::pack(out, *perm);
               }

               const auto &table_idx = db.get_index<table_id_multi_index, by_code_scope_table>();
               uint32_t total_tables = std::distance(table_idx.begin(), table_idx.end());
               fc::raw::pack(out, total_tables);
               iterate_all_tables(db, [&](const table_id_object& t_id) {

                  uint32_t cnt = 0;
                  iterate_all_rows(db, t_id, [&cnt](const key_value_object& row) { cnt++; });

                  fc::raw::pack(out, t_id.id);
                  fc::raw::pack(out, t_id.code);
                  fc::raw::pack(out, t_id.scope);
                  fc::raw::pack(out, t_id.table);
                  fc::raw::pack(out, t_id.payer);
                  fc::raw::pack(out, cnt);

                  //fc::raw::pack(out, t_id);
                  //t_id.count = cnt;

                  iterate_all_rows(db, t_id, [&](const key_value_object& row) {
                     fc::raw::pack(out, row);
                  });
               });

               out.flush();
               out.close();
               ilog( "Snapshot saved in ${s}",("s",snapshot_to));
            } catch ( ... ) {
               try { fc::remove( snapshot_to); } catch (...) {}
               wlog( "Failed to take snapshot");
               return;
            }
         }

      }));
   }
}

void snapshot_plugin::plugin_startup() {
   // Make the magic happen
}

void snapshot_plugin::plugin_shutdown() {
   // OK, that's enough magic
}

}
