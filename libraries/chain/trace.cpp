#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/trace.hpp>

namespace eosio { namespace chain {

action_trace::action_trace(
   const transaction_trace& trace, const action& act, account_name receiver, bool context_free,
   uint32_t action_ordinal, uint32_t creator_action_ordinal, uint32_t closest_unnotified_ancestor_action_ordinal
)
:action_ordinal( action_ordinal )
,creator_action_ordinal( creator_action_ordinal )
,closest_unnotified_ancestor_action_ordinal( closest_unnotified_ancestor_action_ordinal )
,receiver( receiver )
,act( act )
,context_free( context_free )
,trx_id( trace.id )
,block_num( trace.block_num )
,block_time( trace.block_time )
,producer_block_id( trace.producer_block_id )
{}

action_trace::action_trace(
   const transaction_trace& trace, action&& act, account_name receiver, bool context_free,
   uint32_t action_ordinal, uint32_t creator_action_ordinal, uint32_t closest_unnotified_ancestor_action_ordinal
)
:action_ordinal( action_ordinal )
,creator_action_ordinal( creator_action_ordinal )
,closest_unnotified_ancestor_action_ordinal( closest_unnotified_ancestor_action_ordinal )
,receiver( receiver )
,act( std::move(act) )
,context_free( context_free )
,trx_id( trace.id )
,block_num( trace.block_num )
,block_time( trace.block_time )
,producer_block_id( trace.producer_block_id )
{}

ram_trace::ram_trace(
   uint32_t action_id,
   const char* event_id,
   const char* family,
   const char* operation,
   const char* legacy_tag
)
:action_id( action_id )
,event_id( event_id )
,family( family )
,operation( operation )
,legacy_tag( legacy_tag )
{
   EOS_ASSERT(!is_generic(), misc_exception, "ram trace from constructor cannot be generic");
}

ram_trace::ram_trace()
:action_id( 0 )
,event_id( nullptr )
,family( nullptr )
,operation( nullptr )
,legacy_tag( nullptr ) {
   // Valid only for derived struct 'generic_ram_trace'
}

} } // eosio::chain
