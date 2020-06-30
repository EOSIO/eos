#pragma once

#include <eosio/chain/trace.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/block.hpp>
#include <utility>

namespace eosio { namespace trace_api {

   struct authorization_trace_v0 {
      chain::name account;
      chain::name permission;
   };

   struct action_trace_v0 {
      uint64_t                            global_sequence = {};
      chain::name                         receiver = {};
      chain::name                         account = {};
      chain::name                         action = {};
      std::vector<authorization_trace_v0> authorization = {};
      chain::bytes                        data = {};
   };

  struct transaction_trace_v0 {
      using status_type = chain::transaction_receipt_header::status_enum;

      chain::transaction_id_type    id = {};
      std::vector<action_trace_v0>  actions = {};
   };

  struct transaction_trace_v1 : public transaction_trace_v0 {
     fc::enum_type<uint8_t,status_type>         status = {};
     uint32_t                                   cpu_usage_us = 0;
     fc::unsigned_int                           net_usage_words;
     std::vector<chain::signature_type>         signatures = {};
     chain::transaction_header                  trx_header = {};
  };

  struct block_trace_v0 {
     chain::block_id_type               id = {};
     uint32_t                           number = {};
     chain::block_id_type               previous_id = {};
     chain::block_timestamp_type        timestamp = chain::block_timestamp_type(0);
     chain::name                        producer = {};
     std::vector<transaction_trace_v0>  transactions = {};
  };

  struct block_trace_v1 : public block_trace_v0 {
     chain::checksum256_type            transaction_mroot = {};
     chain::checksum256_type            action_mroot = {};
     uint32_t                           schedule_version = {};
     std::vector<transaction_trace_v1>  transactions_v1 = {};
  };

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// The purpose to re-design the structures is to decouple the structures so as to reduce the versions and functions bind to the versions.
// // Let me give a example to explain why we doing so.
// For example, if use the pattern of structure inside structure method, 
// and supposing we will have 3 different member sets separately for block, transaction, action. and each carrier version may carry each payload version.
//    struct   block_vx{ 
//                          block_memberset1 or block_memberset2 or block_memberset3;        
//                            struct transaction_vy
//                            {
//                                 transa_memberset1 or transa_memberset2 or transamember3;
//                                 struct action_vz
//                                 {
//                                    action_memberset1 or action_memberset2 or memberset3;
//                                 }
//                            }    
//                          } 
//   final the  action_vz has 3 structures, transaction_vy has 9 structures,  block_vx has 27 structures which 
//    =  3x3x3 versions to express all kinds of data.  and need 27 functions to bind to the block_vx struct type. and 9 functions bind to 
//      transaction_vy type, and 3 function bind to action_vz type.  totally 27+9+3 = 39 structures and 39 funcitons.
//   
//   after decouple
//   struct block_warpper 
//   {
//       uint32_t block_memberset_version;    // also 3 versions.
//       std::variant<block_memberset_v1, block_memberset_v2,block_memberset_v3>  block_memberset;
//       std::vector<transa_wrapper> transactions;
//   }
//    
//   struct transa_warpper
//   {
//       uint32_t transa_memberset_version;    // also 3 versions.
//       std::variant<transa_memberset_v1, transa_memberset_v2, transa_memberset_v3> transaction;
//       std::vector<action_wrapper> actions; 
//   }
//   struct action_all_version 
//   {
//       uint32_t action_memberset_version;    // also 3 versions.
//       std::variant<action_memberset_v1, action_memberset_v2, action_memberset_v3>  action;
//   }
//
//   In this way, we need 3+3+3 = 9 structures, and also the wrapper structures, totally 12 structures, and 12 functions bind to the structures. 
//   
//   You can see the benefit is save many sturcture and functions, and also if payload fork more version don't cause carrier fork more version.
//


struct authorization_trace_mbset_v0 {
      chain::name account;
      chain::name permission;
};

struct action_trace_mbset_v1{
      uint64_t                            global_sequence = {};
      chain::name                         receiver = {};
      chain::name                         account = {};
      chain::name                         action = {};
      std::vector<authorization_trace_wrapper> authorization = {};
      chain::bytes                        data = {};
      chain::bytes                        return_value = {};
};

struct transaction_trace_mbset_v2{
      sing status_type = chain::transaction_receipt_header::status_enum;
      chain::transaction_id_type    id = {};
      fc::enum_type<uint8_t,status_type>         status = {};
      uint32_t                                   cpu_usage_us = 0;
      fc::unsigned_int                           net_usage_words;
      std::vector<chain::signature_type>         signatures = {};
      chain::transaction_header                  trx_header = {};
};


struct block_trace_mbset_v2{
     chain::block_id_type               id = {};
     uint32_t                           number = {};
     chain::block_id_type               previous_id = {};
     chain::block_timestamp_type        timestamp = chain::block_timestamp_type(0);
     chain::name                        producer = {};

     chain::checksum256_type            transaction_mroot = {};
     chain::checksum256_type            action_mroot = {};
     uint32_t                           schedule_version = {};
     uint32_t                           transaction_version = {};
};

struct block_trace_wrapper
{
     uint32_t block_trace_mbset_ver;
     std::variant<block_trace_mbset_v2> block_mbset;
     std::vector<transaction_trace_wrapper> transactions;
};

struct transaction_trace_wrapper
{
     uint32_t transaction_trace_mbset_ver;
     std::variant<transaction_trace_mbset_v2> transaction_mbset;
     std::vector<action_trace_wrapper> actions;
};

struct action_trace_wrapper
{
     uint32_t action_trace_mbset_ver;
     std::variant<action_trace_mbset_v1> action;
};


struct authorization_trace_wrapper
{
     uint32_t authorization_trace_mbset_ver;
     std::variant<authorization_trace_mbset_v0> authorization;
}


  struct cache_trace {
      chain::transaction_trace_ptr        trace;
      chain::packed_transaction_ptr       trx;
  };

} }

FC_REFLECT(eosio::trace_api::authorization_trace_v0, (account)(permission))
FC_REFLECT(eosio::trace_api::action_trace_v0, (global_sequence)(receiver)(account)(action)(authorization)(data))
FC_REFLECT(eosio::trace_api::transaction_trace_v0, (id)(actions))
FC_REFLECT_DERIVED(eosio::trace_api::transaction_trace_v1, (eosio::trace_api::transaction_trace_v0), (status)(cpu_usage_us)(net_usage_words)(signatures)(trx_header))
FC_REFLECT(eosio::trace_api::block_trace_v0, (id)(number)(previous_id)(timestamp)(producer)(transactions))
FC_REFLECT_DERIVED(eosio::trace_api::block_trace_v1, (eosio::trace_api::block_trace_v0), (transaction_mroot)(action_mroot)(schedule_version)(transactions_v1))
