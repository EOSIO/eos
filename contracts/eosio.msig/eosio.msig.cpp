#include <eosio.msig/eosio.msig.hpp>

namespace eosio {

void multisig::propose( account_name proposer, 
                        name proposal_name,
                        transaction  trx,
                        vector<permission_level> requested) {
   eosio_assert( trx.expiration > now(), "transaction expired" );
   eosio_assert( trx.actions.size() > 0, "transaction must have at least one action" );

   proposals proptable( _self, proposer );

   proptable.emplace( proposer, [&]( auto& prop ) {
      prop.proposal_name       = proposal_name;
      prop.packed_transaction  = pack( trx );
      prop.requested_approvals = std::move(requested);
   });
}

void multisig::approve( account_name proposer, name proposal_name, permission_level level ) {
   //require_auth( level );

   proposals proptable( _self, proposer );
   const auto& prop = proptable.get( proposal_name );

   auto itr = std::find( prop.requested_approvals.begin(), prop.requested_approvals.end(), level );
   eosio_assert( itr != prop.requested_approvals.end(), "no approval previously granted" );
 
   proptable.modify( prop, proposer, [&]( auto& mprop ) {
      mprop.provided_approvals.push_back( level );
      mprop.requested_approvals.erase( itr );
   });
}

void multisig::unapprove( account_name proposer, name proposal_name, permission_level level ) {
   //require_auth( level );

   proposals proptable( _self, proposer );
   const auto& prop = proptable.get( proposal_name );
   auto itr = std::find( prop.provided_approvals.begin(), prop.provided_approvals.end(), level );
   eosio_assert( itr != prop.provided_approvals.end(), "no approval previously granted" );

   proptable.modify( prop, proposer, [&]( auto& mprop ) {
      mprop.requested_approvals.push_back(level);
      mprop.provided_approvals.erase(itr);
   });
}

void multisig::cancel( account_name proposer, name proposal_name, account_name canceler ) {
   require_auth( canceler );

   proposals proptable( _self, proposer );
   const auto& prop = proptable.get( proposal_name );

   if( canceler != proposer ) {
      eosio_assert( unpack<transaction>( prop.packed_transaction ).expiration < now(), "cannot cancel until expiration" );
   }

   proptable.erase(prop);
}

void multisig::exec( account_name proposer, name proposal_name, account_name executer ) {
   require_auth(executer);

   proposals proptable( _self, proposer );
   const auto& prop = proptable.get( proposal_name );

   proptable.erase(prop);
}

} /// namespace eosio

EOSIO_ABI( eosio::multisig, (propose)(approve)(unapprove)(cancel)(exec) )
