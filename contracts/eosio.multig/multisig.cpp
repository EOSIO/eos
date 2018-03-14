#include <eosio.multisig/multisig.hpp>

namespace eosio {

   flat_set<permission_level> proposal::get_grants()const {
      return unpack<flat_set<permission_level>>( grants );
   }

   void proposal::set_grants( const flat_set& grant ) {
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
        newprop.id              = hash[0];
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
        g.erase( prop );
        pr.set_grants( g );
      });
   }

   void multisig::on( const accept& p ) {
     require_auth( p.perm.actor ); /// TODO: update api to allow specifying permisison level
     proposals table( _this_contract, p.proposer );

     const auto& prop = table.get( p.proposalid );
     auto g = prop.get_grants();
     table.modify( prop, 0, [&]( auto& pr ) {
       g.insert( prop );
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
     require_auth( p.proposer );
     proposals table( _this_contract, p.proposer );

     const auto& prop = table.get( p.proposalid );
     table.modify( prop, 0, [&]( auto& pr ) {
        
     });

     send_deferred_with_permission( prop.proposalid, 
                                    prop.effective_after, 
                                    prop.packedtrx.data(), 
                                    prop.packedtrx.size(),
                                    prop.grants.data(), prop.grants.size() );
   }

   bool apply( account_name code, action_name act ) {
      if( code == _this_contract ) {
         if( act == N(propose) ) {
            on( unpack_action<propose>() );
         } else if( act == N(reject) ) {
            on( unpack_action<reject>() );
         } else if( act == N(accept) ) {
            on( unpack_action<accept>() );
         } else if( act == N(cancel) ) {
            on( unpack_action<cancel>() );
         }
      }
      return false;
   }


}
