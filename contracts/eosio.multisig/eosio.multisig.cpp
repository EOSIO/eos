#include <eosio.multisig/eosio.multisig.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/transaction.hpp>
#include <eosiolib/action.hpp>

namespace eosio {

   flat_set<permission_level> multisig::proposal::get_grants()const {
      datastream<const char*> ds( grants.data(), grants.size() );
      flat_set<permission_level> result;
      ds >> result;
      return result;
   }

   void multisig::proposal::set_grants( const flat_set<permission_level>& grant ) {
      datastream<char*> ds( grants.data(), grants.size() );
      ds << grant;
   }


   void multisig::on( propose&& p ) {
     require_auth( p.proposer );

     checksum256 hash;
     sha256( p.packedtrx.data(), p.packedtrx.size(), &hash );

     auto trx = unpack<transaction>( p.packedtrx.data(), p.packedtrx.size() );

     eosio_assert( trx.expiration > now(), "proposed transaction already expired" );

     proposals table( _this_contract, p.proposer );
     table.emplace( p.proposer, [&]( auto& newprop ) {
        newprop.id              = hash.hash[0];
        newprop.proposer        = p.proposer;
        newprop.packedtrx       = move(p.packedtrx);
        newprop.effective_after = p.effective_after;
        newprop.grants.reserve( 64*32 ); /// reserve space so others don't need to have space
     });
   } // on proposal

   void multisig::on( const reject& p ) {
      require_auth( p.perm.actor ); /// TODO update api to allow specifying permission level
      proposals table( _this_contract, p.proposer );
      const auto& prop = table.get( p.proposalid );
      table.modify( prop, 0, [&]( auto& pr ) {
        auto g = pr.get_grants();
        auto itr = g.find( p.perm  );
        eosio_assert( itr != g.end(), "unknown permission" );
        g.erase( itr );
        pr.set_grants( g );
      });
   }

   void multisig::on( const accept& p ) {
     require_auth( p.perm.actor ); /// TODO: update api to allow specifying permisison level
     proposals table( _this_contract, p.proposer );

     const auto& prop = table.get( p.proposalid );
     auto g = prop.get_grants();
     table.modify( prop, 0, [&]( auto& pr ) {
       g.insert( p.perm );
       pr.set_grants( g );
     });
   }

   void multisig::on( const cancel& p ) {
     require_auth( p.proposer );
     proposals table( _this_contract, p.proposer );

     const auto& prop = table.get( p.proposalid );

     if( prop.pending_trx_id ) {
        /// cancel deferred trx
     }
     table.erase( prop );
   }

   void multisig::on( const exec& e ) {
     proposals table( _this_contract, e.proposer );
     const auto& prop = table.get( e.proposalid );

     eosio_assert( !prop.pending_trx_id, "transaction already dispatched" );

     assert_trx_permission( prop.packedtrx.data(), prop.packedtrx.size(), 0, 0, 
                           prop.grants.data(), prop.grants.size() );

     send_deferred( (uint32_t)prop.id, 
                    prop.effective_after, 
                    prop.packedtrx.data(), 
                    prop.packedtrx.size() );

     table.modify( prop, 0, [&]( auto& pr ) {
        pr.pending_trx_id = (uint32_t)prop.id;
     });
   }

   bool multisig::apply( account_name code, action_name act ) {
      if( code == _this_contract ) {
         if( act == N(propose) ) {
            on( unpack_action_data<propose>() );
         } else if( act == N(reject) ) {
            on( unpack_action_data<reject>() );
         } else if( act == N(accept) ) {
            on( unpack_action_data<accept>() );
         } else if( act == N(cancel) ) {
            on( unpack_action_data<cancel>() );
         } else if( act == N(exec) ) {
            on( unpack_action_data<exec>() );
         }
      }
      return false;
   }

} /// namespace eosio

extern "C" {
   void apply( account_name code, action_name act ) {
      eosio::multisig(current_receiver()).apply( code, act );
   }
}
