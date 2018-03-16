#pragma once
#include <eosiolib/multi_index.hpp>
#include <eosiolib/action.hpp>
#include <boost/container/flat_set.hpp>

namespace eosio {

   using boost::container::flat_set;


   class multisig {
      public:
         multisig( account_name self ):_this_contract(self){}

         struct proposal {
            uint64_t                    id;
            account_name                proposer;
            bytes                       packedtrx;
            uint32_t                    effective_after = 0;
            bytes                       grants;
            uint32_t                    pending_trx_id = 0;

            flat_set<permission_level> get_grants()const;
            void                       set_grants( const flat_set<permission_level>& grant );

            uint64_t primary_key()const { return id; }

            EOSLIB_SERIALIZE( proposal, (id)(proposer)(packedtrx)
                              (effective_after)(grants)(pending_trx_id) )
         };
         typedef eosio::multi_index<N(proposals), proposal> proposals;


         struct propose {
            account_name proposer;
            bytes        packedtrx;
            uint32_t     effective_after = 0;

            EOSLIB_SERIALIZE( propose, (proposer)(packedtrx)(effective_after) )
         };

         struct reject {
            permission_level perm;
            account_name     proposer;
            uint64_t         proposalid;

            EOSLIB_SERIALIZE( reject, (perm)(proposer)(proposalid) )
         };

         struct accept {
            permission_level perm;
            account_name     proposer;
            uint64_t         proposalid;

            EOSLIB_SERIALIZE( accept, (perm)(proposer)(proposalid) )
         };

         struct cancel {
            account_name     proposer;
            uint64_t         proposalid;

            EOSLIB_SERIALIZE( cancel, (proposer)(proposalid) )
         };

         struct exec {
            account_name     proposer;
            uint64_t         proposalid;

            EOSLIB_SERIALIZE( exec, (proposer)(proposalid) )
         };

         void on( propose&& p );
         void on( const reject& p );
         void on( const accept& p );
         void on( const cancel& p );
         void on( const exec& p );

         bool apply( account_name code, action_name act );
      private:
         account_name _this_contract;
   };

} /// namespace eosio
