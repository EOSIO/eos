#pragma once

#include <fc/variant_object.hpp>
#include <fc/reflect/reflect.hpp>

#include <vector>

namespace eosio {

   struct EventData {
       chain::account_name code;
       chain::event_name   event;
       fc::variant         args;
       chain::bytes        data;
   };

   struct ActionData {
       chain::account_name receiver;
       chain::account_name code;
       chain::action_name  action;
       fc::variant         args;
       chain::bytes        data;
       std::vector<EventData> events;
   };

   struct TrxMetadata {
       chain::transaction_id_type id;
       bool                       accepted;
       bool                       implicit;
       bool                       scheduled;

       TrxMetadata(const chain::transaction_metadata_ptr &meta)
       : id(meta->id)
       , accepted(meta->accepted)
       , implicit(meta->implicit)
       , scheduled(meta->scheduled)
       {}
   };

   struct BaseMessage {
       enum MsgType {
           Unknown,
           AcceptBlock,
           CommitBlock,

           AcceptTrx,
           ApplyTrx,
       };

       MsgType msg_type;

       BaseMessage(MsgType type = Unknown)
       : msg_type(type)
       {}
   };

   struct AcceptTrxMessage : public BaseMessage, TrxMetadata {
       AcceptTrxMessage(BaseMessage::MsgType msg_type, const chain::transaction_metadata_ptr &trx_meta)
       : BaseMessage(msg_type)
       , TrxMetadata(trx_meta)
       {}
   };

   struct ApplyTrxMessage : public BaseMessage {
       chain::transaction_id_type         id;
       uint32_t                           block_num;
       chain::block_timestamp_type        block_time;
       fc::optional<chain::block_id_type> prod_block_id;
       std::vector<ActionData>            actions;

       ApplyTrxMessage(MsgType msg_type, const chain::transaction_trace_ptr& trace)
       : BaseMessage(msg_type)
       , id(trace->id)
       , block_num(trace->block_num)
       , block_time(trace->block_time)
       , prod_block_id(trace->producer_block_id)
       {}
   };

   struct BlockMessage : public BaseMessage {
       chain::block_id_type          id;
       uint32_t                      block_num;
       chain::block_timestamp_type   block_time;
       bool                          validated;
       bool                          in_current_chain;

       BlockMessage(MsgType msg_type, const chain::block_state_ptr& bstate)
       : BaseMessage(msg_type)
       , id(bstate->block->id())
       , block_num(bstate->block->block_num())
       , block_time(bstate->header.timestamp)
       , validated(bstate->validated)
       , in_current_chain(bstate->in_current_chain)
       {}
   };

   struct AcceptedBlockMessage : public BlockMessage {
       std::vector<TrxMetadata> trxs;
       std::vector<EventData> events;

       AcceptedBlockMessage(MsgType msg_type, const chain::block_state_ptr& bstate)
       : BlockMessage(msg_type, bstate)
       {
           for(auto const &trx: bstate->trxs) {
               TrxMetadata trx_meta(trx);
               trxs.push_back(trx_meta);
           }
       }
   };

} // namespace eosio

FC_REFLECT(eosio::EventData, (code)(event)(data)(args))
FC_REFLECT(eosio::ActionData, (receiver)(code)(action)(data)(args)(events))
FC_REFLECT(eosio::TrxMetadata, (id)(accepted)(implicit)(scheduled))

FC_REFLECT_ENUM(eosio::BaseMessage::MsgType, (Unknown)(AcceptBlock)(CommitBlock)(AcceptTrx)(ApplyTrx))
FC_REFLECT(eosio::BaseMessage, (msg_type))
FC_REFLECT_DERIVED(eosio::BlockMessage, (eosio::BaseMessage), (id)(block_num)(block_time)(validated)(in_current_chain))
FC_REFLECT_DERIVED(eosio::AcceptedBlockMessage, (eosio::BlockMessage), (trxs)(events))
FC_REFLECT_DERIVED(eosio::AcceptTrxMessage, (eosio::BaseMessage)(eosio::TrxMetadata), )
FC_REFLECT_DERIVED(eosio::ApplyTrxMessage, (eosio::BaseMessage), (id)(block_num)(block_time)(prod_block_id)(actions))
