#include <eosio.msig/eosio.msig.hpp>
#include <eosiolib/action.hpp>

namespace eosio {

void multisig::propose( account_name proposer, 
                        name proposal_name,
                        transaction  trx,
                        vector<permission_level> requested) {
   require_auth( proposer );
   eosio_assert( trx.expiration > now(), "transaction expired" );
   eosio_assert( trx.actions.size() > 0, "transaction must have at least one action" );

   proposals proptable( _self, proposer );
   eosio_assert( proptable.find( proposal_name ) == proptable.end(), "proposal with the same name exists" );

   check_auth( trx, requested );

   proptable.emplace( proposer, [&]( auto& prop ) {
      prop.proposal_name       = proposal_name;
      prop.packed_transaction  = pack( trx );
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
      eosio_assert( unpack<transaction>( prop_it->packed_transaction ).expiration < now(), "cannot cancel until expiration" );
   }

   proptable.erase(prop_it);
}

void multisig::exec( account_name proposer, name proposal_name, account_name executer ) {
   require_auth( executer );

   proposals proptable( _self, proposer );
   auto prop_it = proptable.find( proposal_name );
   eosio_assert( prop_it != proptable.end(), "proposal not found" );

   check_auth( prop_it->packed_transaction, prop_it->provided_approvals );
   send_deferred( (uint128_t(proposer) << 64) | proposal_name, executer, now(), prop_it->packed_transaction.data(), prop_it->packed_transaction.size() );

   proptable.erase(prop_it);
}

} /// namespace eosio

EOSIO_ABI( eosio::multisig, (propose)(approve)(unapprove)(cancel)(exec) )
