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

string get_scope_name(const table_id_object& t_id) {
   string scope_name;
   if(t_id.code == N(eosio.token) && t_id.table == N(stat)) {
      scope_name = symbol(uint64_t(t_id.scope)<<8).name();
   } else {
      scope_name = string(t_id.scope);
   }
   return scope_name;   
}

string get_primary_key_name(const table_id_object& t_id, const key_value_object& obj) {
   string pk;
   if(t_id.code == N(eosio.token) && ( t_id.table == N(accounts) || t_id.table == N(stat)) ) {
      pk = symbol(uint64_t(obj.primary_key)<<8).name();
   } else {
      pk = fc::to_string(obj.primary_key);
   }
   return pk;
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

      // Save snapshot when block is irreversible
      m_irreversible_block_connection.emplace(chain.irreversible_block.connect([this, snapshot_at_block, snapshot_to, &db](const chain::block_state_ptr& b) {
         if( b->id == snapshot_at_block ) {
            try {
               fc::rename( snapshot_to+".tmp", snapshot_to );
               ilog("Snapshot saved to ${file}", ("file", snapshot_to));
            } catch ( fc::exception e ) {
               wlog( "Failed to save snapshot: ${ex}", ("ex",e) );
               return;
            }
         }
      }));

      // Take snapshot after block is applied
      m_accepted_block_connection.emplace(chain.accepted_block.connect([this, snapshot_at_block, snapshot_to, &db, &config, &chain](const chain::block_state_ptr& b) {

         if( b->id == snapshot_at_block ) {

            ilog("Taking snapshot on block ${n} (${h})...",("n",snapshot_at_block)("h",b->block_num));

            std::ofstream out;
            try {
               out.open( snapshot_to + ".tmp" );
               // dump chain id
               out << "{\"chain_id\":" << fc::json::to_string(chain.get_chain_id());

               // dump gensis
               out << ",\"genesis\":" << fc::json::to_string(config.genesis);

               // dump accounts with permissions
               out << ",\"accounts\":{";
               bool first = true;
               const auto& account_idx = db.get_index<account_index, by_name>();
               const auto& permissions_idx = db.get_index<permission_index,by_owner>();
               for (auto accnt = account_idx.begin(); accnt != account_idx.end(); ++accnt) {
                  fc::variant v;
                  fc::to_variant(*accnt, v);

                  fc::mutable_variant_object mvo(v);

                  abi_def abi;
                  if( abi_serializer::to_abi(accnt->abi, abi) ) {
                     fc::variant vabi;
                     fc::to_variant(abi, vabi);
                     mvo["abi"] = vabi;
                  }

                  vector<permission> permissions;
                  auto perm = permissions_idx.lower_bound( boost::make_tuple( accnt->name ) );
                  while( perm != permissions_idx.end() && perm->owner == accnt->name ) {
                     struct name parent;

                     // Don't lookup parent if null
                     if( perm->parent._id ) {
                        const auto* p = db.template find<permission_object,by_id>( perm->parent );
                        if( p ) {
                           FC_ASSERT(perm->owner == p->owner, "Invalid parent");
                           parent = p->name;
                        }
                     }

                     permissions.push_back( permission{ perm->name, parent, perm->auth.to_authority() } );
                     ++perm;
                  }

                  mvo["permissions"] = permissions;

                  if(first) { first = false; } else { out << ","; }
                  out << "\"" << string(accnt->name) << "\":" << fc::json::to_string(mvo);
               }
               out << "}";

               // dump all tables
               out << ",\"tables\":{";

               account_name   last_code(0);
               scope_name     last_scope(0);

               iterate_all_tables(db, [&](const table_id_object& t_id) {

                  const auto& code_account = db.get<account_object,by_name>( t_id.code );
                  abi_def abi;
                  auto valid_abi = abi_serializer::to_abi(code_account.abi, abi);

                  abi_serializer abis(abi);

                  if( last_code != t_id.code ) {
                     if( last_code != account_name(0) ) {
                        out << "}},";
                     }
                     out << "\"" << string(t_id.code) << "\":{";
                     out << "\"" << get_scope_name(t_id) << "\":{";
                     out << "\"" << string(t_id.table) << "\":{";
                     last_code  = t_id.code;
                     last_scope = t_id.scope;
                  }
                  else
                  if ( last_scope != t_id.scope ) {
                     if( last_scope != scope_name(0) ) {
                        out << "},";
                     }

                     out << "\"" << get_scope_name(t_id) << "\":{";
                     out << "\"" << string(t_id.table) << "\":{";
                     last_scope = t_id.scope;
                  }
                  else 
                  {
                     if(first) { first = false; } else { out << ","; }
                     out << "\"" << string(t_id.table) << "\":{";
                  }

                  bool first_row = true;
                  vector<char> data;

                  string table_type;
                  if( valid_abi ) 
                     table_type = abis.get_table_type(t_id.table);

                  iterate_all_rows(db, t_id, [&](const key_value_object& row) {
                     if(first_row == true) { first_row = false; } else { out << ","; }

                     auto pk = get_primary_key_name(t_id, row);
                     out << "\"" << pk << "\":";

                     if( !valid_abi || !table_type.size()) {
                        out << "{\"hex_data\":\"" << fc::to_hex(row.value.data(), row.value.size()) << "\"}";
                     } else {
                        data.resize( row.value.size() );
                        memcpy(data.data(), row.value.data(), row.value.size());
                        out << "{\"data\":" 
                            << fc::json::to_string( abis.binary_to_variant(table_type, data) ) 
                            << "}";
                     }
                  });

                  out << "}";

               });

               // close scope/code
               out << "}}";

               // close tables
               out << "}";

               // close main json object
               out << "}";

               out.close();
            } catch ( ... ) {
               try { fc::remove( snapshot_to+".tmp" ); } catch (...) {}
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
