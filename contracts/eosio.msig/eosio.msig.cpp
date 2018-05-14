#include <eosio.msig/eosio.msig.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/permission.hpp>

namespace eosio {

/*
propose function manually parses input data (instead of taking parsed arguments from dispatcher)
because parsing data in the dispatcher uses too much CPU in case if proposed transaction is big

If we use dispatcher the function signature should be:

void multisig::propose( account_name proposer,
                        name proposal_name,
                        vector<permission_level> requested,
                        transaction  trx)
*/

void multisig::propose() {
   constexpr size_t max_stack_buffer_size = 512;
   size_t size = action_data_size();
   char* buffer = (char*)( max_stack_buffer_size < size ? malloc(size) : alloca(size) );
   read_action_data( buffer, size );

   account_name proposer;
   name proposal_name;
   vector<permission_level> requested;
   transaction_header trx_header;

   datastream<const char*> ds( buffer, size );
   ds >> proposer >> proposal_name >> requested;

   size_t trx_pos = ds.tellp();
   ds >> trx_header;

   require_auth( proposer );
   eosio_assert( trx_header.expiration >= eosio::time_point_sec(now()), "transaction expired" );
   //eosio_assert( trx_header.actions.size() > 0, "transaction must have at least one action" );

   proposals proptable( _self, proposer );
   eosio_assert( proptable.find( proposal_name ) == proptable.end(), "proposal with the same name exists" );

   bytes packed_requested = pack(requested);
   auto res = ::check_transaction_authorization( buffer+trx_pos, size-trx_pos,
                                                 (const char*)0, 0,
                                                 packed_requested.data(), packed_requested.size()
                                               );
   eosio_assert( res > 0, "transaction authorization failed" );

   proptable.emplace( proposer, [&]( auto& prop ) {
      prop.proposal_name       = proposal_name;
      prop.packed_transaction  = bytes( buffer+trx_pos, buffer+size );
      prop.requested_approvals = std::move(requested);
   });
}

void multisig::approve( account_name proposer, name proposal_name, permission_level level ) {
   require_auth( level );

   proposals proptable( _self, proposer );
   auto prop_it = proptable.find( proposal_name );
   eosio_assert( prop_it != proptable.end(), "proposal not found" );

   auto itr = std::find( prop_it->requested_approvals.begin(), prop_it->requested_approvals.end(), level );
   eosio_assert( itr != prop_it->requested_approvals.end(), "approval is not on the list of requested approvals" );

   proptable.modify( prop_it, proposer, [&]( auto& mprop ) {
      mprop.provided_approvals.push_back( level );
      mprop.requested_approvals.erase( itr );
   });
}

void multisig::unapprove( account_name proposer, name proposal_name, permission_level level ) {
   require_auth( level );

   proposals proptable( _self, proposer );
   auto prop_it = proptable.find( proposal_name );
   eosio_assert( prop_it != proptable.end(), "proposal not found" );
   auto itr = std::find( prop_it->provided_approvals.begin(), prop_it->provided_approvals.end(), level );
   eosio_assert( itr != prop_it->provided_approvals.end(), "no approval previously granted" );

   proptable.modify( prop_it, proposer, [&]( auto& mprop ) {
      mprop.requested_approvals.push_back(level);
      mprop.provided_approvals.erase(itr);
   });
}

void multisig::cancel( account_name proposer, name proposal_name, account_name canceler ) {
   require_auth( canceler );

   proposals proptable( _self, proposer );
   auto prop_it = proptable.find( proposal_name );
   eosio_assert( prop_it != proptable.end(), "proposal not found" );

   if( canceler != proposer ) {
      eosio_assert( unpack<transaction_header>( prop_it->packed_transaction ).expiration < eosio::time_point_sec(now()), "cannot cancel until expiration" );
   }

   proptable.erase(prop_it);
}

void multisig::exec( account_name proposer, name proposal_name, account_name executer ) {
   require_auth( executer );

   proposals proptable( _self, proposer );
   auto prop_it = proptable.find( proposal_name );
   eosio_assert( prop_it != proptable.end(), "proposal not found" );

   transaction_header trx_header;
   datastream<const char*> ds( prop_it->packed_transaction.data(), prop_it->packed_transaction.size() );
   ds >> trx_header;
   eosio_assert( trx_header.expiration >= eosio::time_point_sec(now()), "transaction expired" );

   bytes packed_provided_approvals = pack(prop_it->provided_approvals);
   auto res = ::check_transaction_authorization( prop_it->packed_transaction.data(), prop_it->packed_transaction.size(),
                                                 (const char*)0, 0,
                                                 packed_provided_approvals.data(), packed_provided_approvals.size()
                                               );
   eosio_assert( res > 0, "transaction authorization failed" );

   send_deferred( (uint128_t(proposer) << 64) | proposal_name, executer, prop_it->packed_transaction.data(), prop_it->packed_transaction.size() );

   proptable.erase(prop_it);
}

} /// namespace eosio

EOSIO_ABI( eosio::multisig, (propose)(approve)(unapprove)(cancel)(exec) )
