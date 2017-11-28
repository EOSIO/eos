/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/contracts/chain_initializer.hpp>
#include <eosio/chain/contracts/objects.hpp>
#include <eosio/chain/contracts/eos_contract.hpp>
#include <eosio/chain/contracts/types.hpp>

#include <eosio/chain/producer_object.hpp>
#include <eosio/chain/permission_object.hpp>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace eosio { namespace chain { namespace contracts {

time_point chain_initializer::get_chain_start_time() {
   return genesis.initial_timestamp;
}

chain_config chain_initializer::get_chain_start_configuration() {
   return genesis.initial_configuration;
}

producer_schedule_type chain_initializer::get_chain_start_producers() {
   producer_schedule_type result;
   std::transform(genesis.initial_producers.begin(), genesis.initial_producers.end(), result.producers.begin(),
                  [](const auto& p) { return producer_key{p.owner_name,p.block_signing_key}; });
   return result;
}

void chain_initializer::register_types(chain_controller& chain, chainbase::database& db) {
   // Install the native contract's indexes; we can't do anything until our objects are recognized
   db.add_index<staked_balance_multi_index>();
   db.add_index<producer_votes_multi_index>();
   db.add_index<proxy_vote_multi_index>();
   db.add_index<producer_schedule_multi_index>();

   db.add_index<balance_multi_index>();

#define SET_APP_HANDLER( contract, scope, action, nspace ) \
   chain._set_apply_handler( #contract, #scope, #action, &BOOST_PP_CAT(contracts::apply_, BOOST_PP_CAT(contract, BOOST_PP_CAT(_,action) ) ) )

   SET_APP_HANDLER( eosio, eosio, setproducer, eosio );
   SET_APP_HANDLER( eosio, eosio, newaccount, eosio );
   SET_APP_HANDLER( eosio, eosio, transfer, eosio );
   SET_APP_HANDLER( eosio, eosio, lock, eosio );
   SET_APP_HANDLER( eosio, eosio, claim, eosio );
   SET_APP_HANDLER( eosio, eosio, unlock, eosio );
   SET_APP_HANDLER( eosio, eosio, okproducer, eosio );
   SET_APP_HANDLER( eosio, eosio, setproxy, eosio );
   SET_APP_HANDLER( eosio, eosio, setcode, eosio );
   SET_APP_HANDLER( eosio, eosio, setabi, eosio );
   SET_APP_HANDLER( eosio, eosio, updateauth, eosio );
   SET_APP_HANDLER( eosio, eosio, deleteauth, eosio );
   SET_APP_HANDLER( eosio, eosio, linkauth, eosio );
   SET_APP_HANDLER( eosio, eosio, unlinkauth, eosio );
   SET_APP_HANDLER( eosio, eosio, nonce, eosio );
}


abi_def chain_initializer::eos_contract_abi()
{
   abi_def eos_abi;
   eos_abi.types.push_back( type_def{"account_name","name"} );
   eos_abi.types.push_back( type_def{"share_type","int64"} );
   eos_abi.actions.push_back( action_def{name("transfer"), "transfer"} );
   eos_abi.actions.push_back( action_def{name("lock"), "lock"} );
   eos_abi.actions.push_back( action_def{name("unlock"), "unlock"} );
   eos_abi.actions.push_back( action_def{name("claim"), "claim"} );
   eos_abi.actions.push_back( action_def{name("okproducer"), "okproducer"} );
   eos_abi.actions.push_back( action_def{name("setproducer"), "setproducer"} );
   eos_abi.actions.push_back( action_def{name("setproxy"), "setproxy"} );
   eos_abi.actions.push_back( action_def{name("setcode"), "setcode"} );
   eos_abi.actions.push_back( action_def{name("setabi"), "setabi"} );
   eos_abi.actions.push_back( action_def{name("linkauth"), "linkauth"} );
   eos_abi.actions.push_back( action_def{name("unlinkauth"), "unlinkauth"} );
   eos_abi.actions.push_back( action_def{name("updateauth"), "updateauth"} );
   eos_abi.actions.push_back( action_def{name("deleteauth"), "deleteauth"} );
   eos_abi.actions.push_back( action_def{name("newaccount"), "newaccount"} );
   eos_abi.actions.push_back( action_def{name("nonce"), "nonce"} );
   eos_abi.structs.emplace_back( struct_def {
      "transfer", "", {
         {"from", "account_name"},
         {"to", "account_name"},
         {"amount", "uint64"},
         {"memo", "string"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "lock", "", {
         {"from", "account_name"},
         {"to", "account_name"},
         {"amount", "share_type"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "unlock", "", {
         {"account", "account_name"},
         {"amount", "share_type"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "claim", "", {
         {"account", "account_name"},
         {"amount", "share_type"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "okproducer", "", {
         {"voter", "account_name"},
         {"producer", "account_name"},
         {"approve", "int8"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "setproducer", "", {
         {"name", "account_name"},
         {"key", "public_key"},
         {"configuration", "chain_config"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "setproxy", "", {
         {"stakeholder", "account_name"},
         {"proxy", "account_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "setcode", "", {
         {"account", "account_name"},
         {"vmtype", "uint8"},
         {"vmversion", "uint8"},
         {"code", "bytes"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "updateauth", "", {
         {"account", "account_name"},
         {"permission", "permission_name"},
         {"parent", "permission_name"},
         {"authority", "authority"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "linkauth", "", {
         {"account", "account_name"},
         {"code", "account_name"},
         {"type", "action_name"},
         {"requirement", "permission_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "unlinkauth", "", {
         {"account", "account_name"},
         {"code", "account_name"},
         {"type", "action_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "deleteauth", "", {
         {"account", "account_name"},
         {"permission", "permission_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "newaccount", "", {
         {"creator", "account_name"},
         {"name", "account_name"},
         {"owner", "authority"},
         {"active", "authority"},
         {"recovery", "authority"},
         {"deposit", "asset"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "nonce", "", {
         {"value", "name"}
      }
   });

   return eos_abi;
}

std::vector<action> chain_initializer::prepare_database( chain_controller& chain,
                                                         chainbase::database& db) {
   std::vector<action> messages_to_process;

   // Create the singleton object, producer_schedule_object
   db.create<producer_schedule_object>([](const auto&){});

   /// Create the native contract accounts manually; sadly, we can't run their contracts to make them create themselves
   auto create_native_account = [this, &db](account_name name, auto liquid_balance) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = genesis.initial_timestamp;

         if( name == config::system_account_name ) {
            a.set_abi(eos_contract_abi());
         }
      });
      const auto& owner = db.create<permission_object>([&name](permission_object& p) {
         p.owner = name;
         p.name = "owner";
         p.auth.threshold = 1;
      });
      db.create<permission_object>([&name, &owner](permission_object& p) {
         p.owner = name;
         p.parent = owner.id;
         p.name = "active";
         p.auth.threshold = 1;
      });
      db.create<balance_object>([&name, liquid_balance]( auto& b) {
         b.owner_name = name;
         b.balance = liquid_balance;
      });
      db.create<staked_balance_object>([&name](auto& sb) { sb.owner_name = name; });
   };
   create_native_account(config::system_account_name, config::initial_token_supply);

   // Queue up messages which will run contracts to create the initial accounts
   for (const auto& acct : genesis.initial_accounts) {
      action message( {{config::system_account_name, config::active_name}},
                      newaccount{ config::system_account_name, acct.name,
                                                             authority(acct.owner_key),
                                                             authority(acct.active_key),
                                                             authority(acct.owner_key),
                                                             acct.staking_balance});

      messages_to_process.emplace_back(move(message));

      if (acct.liquid_balance > 0) {
         message = action( {{config::system_account_name, config::active_name}},
                           transfer{ .from = config::system_account_name, .to = acct.name,
                                     .amount = acct.liquid_balance.amount, .memo = "Genesis Allocation"});
         messages_to_process.emplace_back(move(message));
      }
   }

   // Create initial producers
   auto create_producer = boost::adaptors::transformed([config = genesis.initial_configuration](const auto& p) {
      return action( {{p.owner_name, config::active_name}},
                     setproducer(p.owner_name, p.block_signing_key, config));
   });
   boost::copy(genesis.initial_producers | create_producer, std::back_inserter(messages_to_process));

   // Create special accounts
   auto create_special_account = [this, &db](account_name name, const auto& owner, const auto& active) {
      db.create<account_object>([this, &name](account_object& a) {
         a.name = name;
         a.creation_date = genesis.initial_timestamp;
      });
      const auto& owner_permission = db.create<permission_object>([&owner, &name](permission_object& p) {
         p.name = config::owner_name;
         p.parent = 0;
         p.owner = name;
         p.auth = move(owner);
      });
      db.create<permission_object>([&active, &owner_permission](permission_object& p) {
         p.name = config::active_name;
         p.parent = owner_permission.id;
         p.owner = owner_permission.owner;
         p.auth = move(active);
      });
   };

   auto empty_authority = authority(0, {}, {});
   auto active_producers_authority = authority(config::producers_authority_threshold, {}, {});
   for(auto& p : genesis.initial_producers) {
      active_producers_authority.accounts.push_back({{p.owner_name, config::active_name}, 1});
   }

   create_special_account(config::nobody_account_name, empty_authority, empty_authority);
   create_special_account(config::producers_account_name, empty_authority, active_producers_authority);

   return messages_to_process;
}

} } } // namespace eosio::chain::contracts
