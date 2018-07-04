#include "common.hpp"

#include <eosiolib/contract.hpp>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/vector.hpp>

namespace identity {
   using eosio::action_meta;
   using eosio::singleton;
   using eosio::key256;
   using std::string;
   using std::vector;

   /**
    *  This contract maintains a graph database of certified statements about an
    *  identity.  An identity is separated from the concept of an account because
    *  the mapping of identity to accounts is subject to community consensus.
    *
    *  Some use cases need a global source of trust, this trust rooted in the voter
    *  who selects block producers. A block producer's opinion is "trusted" and so
    *  is the opinion of anyone the block producer marks as "trusted".
    *
    *  When a block producer is voted out the implicit trust in every certification
    *  they made or those they trusted made is removed. All users are liable for
    *  making false certifications.
    *
    *  An account needs to claim the identity and a trusted account must certify the
    *  claim.
    *
    *  Data for an identity is stored:
    *
    *  DeployToAccount / identity / certs / [property, trusted, certifier] => value
    *
    *  Questions database is designed to answer:
    *
    *     1. has $identity.$unique been certified a "trusted" certifier
    *     2. has $identity.$property been certified by $account
    *     3. has $identity.$trusted been certified by a "trusted" certifier
    *     4. what account has authority to speak on behalf of identity?
    *         - for each trusted owner certification
    *            check to see if the account has claimed it
    *
    *     5. what identity does account have authority to speak on behalf?
    *         - check what identity the account has self certified owner
    *         - verify that a trusted certifier has confirmed owner
    *
    *  This database structure enables parallel opeartions on independent identities.
    *
    *  When an account certs a property we check to see if that
    */
   class contract : public identity_base {
      public:

        using identity_base::identity_base;

         void settrust( const account_name trustor, ///< the account authorizing the trust
                        const account_name trusting, ///< the account receiving the trust
                        const uint8_t      trust = 0 )/// 0 to remove, -1 to mark untrusted, 1 to mark trusted
         {
            require_auth( trustor );
            require_recipient( trusting );

            trust_table table( _self, trustor );
            auto itr = table.find(trusting);
            if( itr == table.end() && trust > 0 ) {
               table.emplace( trustor, [&](trustrow& row) {
                     row.account = trusting;
                  });
            } else if( itr != table.end() && trust == 0 ) {
               table.erase(itr);
            }
         }

         /**
          * This action create a new globally unique 64 bit identifier,
          * to minimize collisions each account is automatically assigned
          * a 32 bit identity prefix based upon hash(account_name) ^ hash(tapos).
          *
          * With this method no two accounts are likely to be assigned the same
          * 32 bit prefix consistantly due to the constantly changing tapos. This prevents
          * abuse of 'creator' selection to generate intentional conflicts with other users.
          *
          * The creator can determine the last 32 bits using an algorithm of their choice. We
          * presume the creator's algorithm can avoid collisions with itself.
          *
          * Even if two accounts get a collision in first 32 bits, a proper creator algorithm
          * should generate randomness in last 32 bits that will minimize collisions. In event
          * of collision transaction will fail and creator can try again.
          *
          * A 64 bit identity is used because the key is used frequently and it makes for more
          * effecient tables/scopes/etc.
          */
         void create( const account_name creator, const uint64_t identity ) {
            require_auth( creator );
            idents_table t( _self, _self );
            auto itr = t.find( identity );
            eosio_assert( itr == t.end(), "identity already exists" );
            eosio_assert( identity != 0, "identity=0 is not allowed" );
            t.emplace(creator, [&](identrow& i) {
                  i.identity = identity;
                  i.creator = creator;
               });
         }

         void certprop( const account_name       bill_storage_to, ///< account which is paying for storage
                               const account_name       certifier,
                               const identity_name      identity,
                               const vector<certvalue>& values )
         {
            require_auth( certifier );
            if( bill_storage_to != certifier )
               require_auth( bill_storage_to );

            idents_table t( _self, _self );
            eosio_assert( t.find( identity ) != t.end(), "identity does not exist" );

            /// the table exists in the scope of the identity
            certs_table certs( _self, identity );
            bool trusted = is_trusted( certifier );

            for( const auto& value : values ) {
               auto idx = certs.template get_index<N(bytuple)>();
               if (value.confidence) {
                  eosio_assert(value.type.size() <= 32, "certrow::type should be not longer than 32 bytes");
                  auto itr = idx.lower_bound( certrow::key(value.property, trusted, certifier) );

                  if (itr != idx.end() && itr->property == value.property && itr->trusted == trusted && itr->certifier == certifier) {
                     idx.modify(itr, 0, [&](certrow& row) {
                           row.confidence = value.confidence;
                           row.type       = value.type;
                           row.data       = value.data;
                        });
                  } else {
                     auto pk = certs.available_primary_key();
                     certs.emplace(_self, [&](certrow& row) {
                           row.id = pk;
                           row.property   = value.property;
                           row.trusted    = trusted;
                           row.certifier  = certifier;
                           row.confidence = value.confidence;
                           row.type       = value.type;
                           row.data       = value.data;
                        });
                  }

                  auto itr_old = idx.lower_bound( certrow::key(value.property, !trusted, certifier) );
                  if (itr_old != idx.end() && itr_old->property == value.property && itr_old->trusted == !trusted && itr_old->certifier == certifier) {
                     idx.erase(itr_old);
                  }

                  //special handling for owner
                  if (value.property == N(owner)) {
                     eosio_assert(sizeof(account_name) == value.data.size(), "data size doesn't match account_name size");
                     account_name acnt = *reinterpret_cast<const account_name*>(value.data.data());
                     if (certifier == acnt) { //only self-certitication affects accounts_table
                        accounts_table( _self, acnt ).set( identity, acnt );
                     }
                  }
               } else {
                  bool removed = false;
                  auto itr = idx.lower_bound( certrow::key(value.property, trusted, certifier) );
                  if (itr != idx.end() && itr->property == value.property && itr->trusted == trusted && itr->certifier == certifier) {
                     idx.erase(itr);
                  } else {
                     removed = true;
                  }
                  itr = idx.lower_bound( certrow::key(value.property, !trusted, certifier) );
                  if (itr != idx.end() && itr->property == value.property && itr->trusted == !trusted && itr->certifier == certifier) {
                     idx.erase(itr);
                  } else {
                     removed = true;
                  }
                  //special handling for owner
                  if (value.property == N(owner)) {
                     eosio_assert(sizeof(account_name) == value.data.size(), "data size doesn't match account_name size");
                     account_name acnt = *reinterpret_cast<const account_name*>(value.data.data());
                     if (certifier == acnt) { //only self-certitication affects accounts_table
                        accounts_table( _self, acnt ).remove();
                     }
                  }
               }
            }
         }
   };

} /// namespace identity

EOSIO_ABI( identity::contract, (create)(certprop)(settrust) );
