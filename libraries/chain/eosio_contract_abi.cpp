#include <eosio/chain/eosio_contract.hpp>

namespace eosio { namespace chain {

abi_def eosio_contract_abi(const abi_def& eosio_system_abi)
{
   abi_def eos_abi(eosio_system_abi);
   eos_abi.types.push_back( type_def{"account_name","name"} );
   eos_abi.types.push_back( type_def{"table_name","name"} );
   eos_abi.types.push_back( type_def{"share_type","int64"} );
   eos_abi.types.push_back( type_def{"onerror","bytes"} );
   eos_abi.types.push_back( type_def{"context_free_type","bytes"} );
   eos_abi.types.push_back( type_def{"weight_type","uint16"} );
   eos_abi.types.push_back( type_def{"fields","field[]"} );
   eos_abi.types.push_back( type_def{"time_point_sec","time"} );

   // TODO add ricardian contracts
   eos_abi.actions.push_back( action_def{name("setcode"), "setcode",""} );
   eos_abi.actions.push_back( action_def{name("setabi"), "setabi",""} );
   eos_abi.actions.push_back( action_def{name("linkauth"), "linkauth",""} );
   eos_abi.actions.push_back( action_def{name("unlinkauth"), "unlinkauth",""} );
   eos_abi.actions.push_back( action_def{name("updateauth"), "updateauth",""} );
   eos_abi.actions.push_back( action_def{name("deleteauth"), "deleteauth",""} );
   eos_abi.actions.push_back( action_def{name("newaccount"), "newaccount",""} );
   eos_abi.actions.push_back( action_def{name("postrecovery"), "postrecovery",""} );
   eos_abi.actions.push_back( action_def{name("passrecovery"), "passrecovery",""} );
   eos_abi.actions.push_back( action_def{name("vetorecovery"), "vetorecovery",""} );
   eos_abi.actions.push_back( action_def{name("onerror"), "onerror",""} );
   eos_abi.actions.push_back( action_def{name("onblock"), "onblock",""} );
   eos_abi.actions.push_back( action_def{name("canceldelay"), "canceldelay",""} );

   // TODO add any ricardian_clauses
   //
   // ACTION PAYLOADS


   eos_abi.structs.emplace_back( struct_def {
      "setcode", "", {
         {"account", "account_name"},
         {"vmtype", "uint8"},
         {"vmversion", "uint8"},
         {"code", "bytes"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "setabi", "", {
         {"account", "account_name"},
         {"abi", "abi_def"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "updateauth", "", {
         {"account", "account_name"},
         {"permission", "permission_name"},
         {"parent", "permission_name"},
         {"auth", "authority"}
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
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "postrecovery", "", {
         {"account", "account_name"},
         {"auth", "authority"},
         {"memo", "string"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "passrecovery", "", {
         {"account", "account_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "vetorecovery", "", {
         {"account", "account_name"},
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "canceldelay", "", {
         {"canceling_auth", "permission_level"},
         {"trx_id", "transaction_id_type"},
      }
   });

   // DATABASE RECORDS

   eos_abi.structs.emplace_back( struct_def {
      "pending_recovery", "", {
         {"account",    "name"},
         {"request_id", "uint128"},
         {"update",     "updateauth"},
         {"memo",       "string"}
      }
   });

   eos_abi.tables.emplace_back( table_def {
      "recovery", "i64", {
         "account",
      }, {
         "name"
      },
      "pending_recovery"
   });

   // abi_def fields

   eos_abi.structs.emplace_back( struct_def {
      "field", "", {
         {"name", "field_name"},
         {"type", "type_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "struct_def", "", {
         {"name", "type_name"},
         {"base", "type_name"},
         {"fields", "fields"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "permission_level", "", {
         {"actor", "account_name"},
         {"permission", "permission_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "action", "", {
         {"account", "account_name"},
         {"name", "action_name"},
         {"authorization", "permission_level[]"},
         {"data", "bytes"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "permission_level_weight", "", {
         {"permission", "permission_level"},
         {"weight", "weight_type"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "transaction_header", "", {
         {"expiration", "time_point_sec"},
         {"ref_block_num", "uint16"},
         {"ref_block_prefix", "uint32"},
         {"max_net_usage_words", "varuint32"},
         {"max_kcpu_usage", "varuint32"},
         {"delay_sec", "varuint32"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "transaction", "transaction_header", {
         {"context_free_actions", "action[]"},
         {"actions", "action[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "signed_transaction", "transaction", {
         {"signatures", "signature[]"},
         {"context_free_data", "bytes[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "key_weight", "", {
         {"key", "public_key"},
         {"weight", "weight_type"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "authority", "", {
         {"threshold", "uint32"},
         {"delay_sec", "uint32"},
         {"keys", "key_weight[]"},
         {"accounts", "permission_level_weight[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
         "clause_pair", "", {
            {"id", "string"},
            {"body", "string"}
         }
   });
   eos_abi.structs.emplace_back( struct_def {
      "type_def", "", {
         {"new_type_name", "type_name"},
         {"type", "type_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "action_def", "", {
         {"name", "action_name"},
         {"type", "type_name"},
         {"ricardian_contract", "string"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "table_def", "", {
         {"name", "table_name"},
         {"index_type", "type_name"},
         {"key_names", "field_name[]"},
         {"key_types", "type_name[]"},
         {"type", "type_name"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "abi_def", "", {
         {"types", "type_def[]"},
         {"structs", "struct_def[]"},
         {"actions", "action_def[]"},
         {"tables", "table_def[]"},
         {"ricardian_clauses", "clause_pair[]"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
      "block_header", "", {
         {"previous", "checksum256"},
         {"timestamp", "uint32"},
         {"transaction_mroot", "checksum256"},
         {"action_mroot", "checksum256"},
         {"block_mroot", "checksum256"},
         {"producer", "account_name"},
         {"schedule_version", "uint32"},
         {"new_producers", "producer_schedule?"}
      }
   });

   eos_abi.structs.emplace_back( struct_def {
         "onblock", "", {
            {"header", "block_header"}
      }
   });

   return eos_abi;
}
} } /// eosio::chain
