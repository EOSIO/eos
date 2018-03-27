#pragma once

#include <eosiolib/chain.h>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/multi_index.hpp>
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
   template<uint64_t DeployToAccount>
   class contract {
      public:

         static const uint64_t code = DeployToAccount;
         typedef uint64_t identity_name;
         typedef uint64_t property_name;
         typedef uint64_t property_type_name;

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
         struct create : public action_meta< code, N(create) >
         {
            account_name creator;
            uint64_t     identity = 0; ///< first 32 bits determinsitically derived from creator and tapos

            EOSLIB_SERIALIZE( create, (creator)(identity) )
         };


         struct certvalue {
            property_name property; ///< name of property, base32 encoded i64
            string     type; ///< defines type serialized in data
            vector<char>  data; ///<
            string        memo; ///< meta data documenting basis of certification
            uint8_t       confidence = 1; ///< used to define liability for lies,
                                          /// 0 to delete

            EOSLIB_SERIALIZE( certvalue, (property)(type)(data)(memo)(confidence) )
         };

         struct certprop : public action_meta< code, N(certprop) >
         {
            account_name        bill_storage_to; ///< account which is paying for storage
            account_name        certifier;
            identity_name       identity;
            vector<certvalue>   values;

            EOSLIB_SERIALIZE( certprop, (bill_storage_to)(certifier)(identity)(values) )
         };

         struct settrust : public action_meta< code, N(settrust) >
         {
            account_name trustor; ///< the account authorizing the trust
            account_name trusting; ///< the account receiving the trust
            uint8_t      trust = 0; /// 0 to remove, -1 to mark untrusted, 1 to mark trusted

            EOSLIB_SERIALIZE( settrust, (trustor)(trusting)(trust) )
         };

         struct certrow {
            uint64_t            id;
            property_name       property;
            uint64_t            trusted;
            account_name        certifier;
            uint8_t             confidence = 0;
            string              type;
            vector<char>        data;
            uint64_t primary_key() const { return id; }
            /* constexpr */ static key256 key(uint64_t property, uint64_t trusted, uint64_t certifier) {
               /*
               key256 key;
               key.uint64s[0] = property;
               key.uint64s[1] = trusted;
               key.uint64s[2] = certifier;
               key.uint64s[3] = 0;
               */
               return key256::make_from_word_sequence<uint64_t>(property, trusted, certifier);
            }
            key256 get_key() const { return key(property, trusted, certifier); }

            EOSLIB_SERIALIZE( certrow , (property)(trusted)(certifier)(confidence)(type)(data)(id) )
         };

         struct identrow {
            uint64_t     identity;
            account_name creator;

            uint64_t primary_key() const { return identity; }

            EOSLIB_SERIALIZE( identrow , (identity)(creator) )
         };

         struct trustrow {
            account_name account;

            uint64_t primary_key() const { return account; }

            EOSLIB_SERIALIZE( trustrow, (account) )
         };

         typedef eosio::multi_index<N(certs), certrow,
                                    eosio::indexed_by< N(bytuple), eosio::const_mem_fun<certrow, key256, &certrow::get_key> >
                                    > certs_table;
         typedef eosio::multi_index<N(ident), identrow> idents_table;
         typedef singleton<code, N(account), code, identity_name>  accounts_table;
         typedef eosio::multi_index<N(trust), trustrow> trust_table;

         static identity_name get_claimed_identity( account_name acnt ) {
            return accounts_table::get_or_default(acnt, 0);
         }

         static account_name get_owner_for_identity( identity_name ident ) {
            // for each trusted owner certification
            //   check to see if the certification is still trusted
            //   check to see if the account has claimed it
            certs_table certs( code, ident );
            auto idx = certs.template get_index<N(bytuple)>();
            auto itr = idx.lower_bound(certrow::key(N(owner), 1, 0));
            account_name owner = 0;
            while (itr != idx.end() && itr->property == N(owner) && itr->trusted) {
               if (sizeof(account_name) == itr->data.size()) {
                  account_name account = *reinterpret_cast<const account_name*>(itr->data.data());
                  if (ident == get_claimed_identity(account)) {
                     if (is_trusted(itr->certifier) ) {
                        // the certifier is still trusted
                        if (!owner || owner == account) {
                           owner = account;
                        } else {
                           //contradiction found: different owners certified for the same identity
                           return 0;
                        }
                     } else if (DeployToAccount == current_receiver()){
                        //the certifier is no longer trusted, need to unset the flag
                        idx.modify(itr, 0, [&](certrow& r) {
                              r.trusted = 0;
                           });
                     } else {
                        // the certifier is no longer trusted, but the code runs in read-only mode
                     }
                  }
               } else {
                  // bad row - skip it
               }
               ++itr;
            }
            if (owner) {
               //owner found, no contradictions among certifications flaged as trusted
               return owner;
            }
            // trusted certification not found
            // let's see if any untrusted certifications became trusted
            itr = idx.lower_bound(certrow::key(N(owner), 0, 0));
            while (itr != idx.end() && itr->property == N(owner) && !itr->trusted) {
               if (sizeof(account_name) == itr->data.size()) {
                  account_name account = *reinterpret_cast<const account_name*>(itr->data.data());
                  if (ident == get_claimed_identity(account) && is_trusted(itr->certifier)) {
                     if (DeployToAccount == current_receiver()) {
                        // the certifier became trusted and we have permissions to update the flag
                        idx.modify(itr, 0, [&](certrow& r) {
                              r.trusted = 1;
                           });
                     }
                     if (!owner || owner == account) {
                        owner = account;
                     } else {
                        //contradiction found: different owners certified for the same identity
                        return 0;
                     }
                  }
               } else {
                  // bad row - skip it
               }
               ++itr;
            }
            return owner;
         }

         static identity_name get_identity_for_account( account_name acnt ) {
            //  check what identity the account has self certified owner
            //  verify that a trusted certifier has confirmed owner
            auto identity = get_claimed_identity(acnt);
            return (identity != 0 && acnt == get_owner_for_identity(identity)) ? identity : 0;
         }

         static bool is_trusted_by( account_name trusted, account_name by ) {
            trust_table t( code, by );
            return t.find( trusted ) != t.end();
         }

         static bool is_trusted( account_name acnt ) {
            account_name active_producers[21];
            auto count = get_active_producers( active_producers, sizeof(active_producers) );
            for( size_t i = 0; i < count; ++i ) {
               if( active_producers[i] == acnt )
                  return true;
            }
            for( size_t i = 0; i < count; ++i ) {
               if( is_trusted_by( acnt, active_producers[i] ) )
                  return true;
            }
            return false;
         }

         static void on( const settrust& t ) {
            require_auth( t.trustor );
            require_recipient( t.trusting );

            trust_table table( code, t.trustor );
            auto itr = table.find(t.trusting);
            if( itr == table.end() && t.trust > 0 ) {
               table.emplace( t.trustor, [&](trustrow& row) {
                     row.account = t.trusting;
                  });
            } else if( itr != table.end() && t.trust == 0 ) {
               table.erase(itr);
            }
         }

         static void on( const create& c ) {
            require_auth( c.creator );
            idents_table t( code, code);
            auto itr = t.find( c.identity );
            eosio_assert( itr == t.end(), "identity already exists" );
            eosio_assert( c.identity != 0, "identity=0 is not allowed" );
            t.emplace(c.creator, [&](identrow& i) {
                  i.identity = c.identity;
                  i.creator = c.creator;
               });
         }

         static void on( const certprop& cert ) {
            require_auth( cert.certifier );
            if( cert.bill_storage_to != cert.certifier )
               require_auth( cert.bill_storage_to );

            idents_table t( code, code );
            eosio_assert( t.find( cert.identity ) != t.end(), "identity does not exist" );

            /// the table exists in the scope of the identity
            certs_table certs( code, cert.identity );
            bool trusted = is_trusted( cert.certifier );

            for( const auto& value : cert.values ) {
               auto idx = certs.template get_index<N(bytuple)>();
               if (value.confidence) {
                  eosio_assert(value.type.size() <= 32, "certrow::type should be not longer than 32 bytes");
                  auto itr = idx.lower_bound( certrow::key(value.property, trusted, cert.certifier) );

                  if (itr != idx.end() && itr->property == value.property && itr->trusted == trusted && itr->certifier == cert.certifier) {
                     idx.modify(itr, 0, [&](certrow& row) {
                           row.confidence = value.confidence;
                           row.type       = value.type;
                           row.data       = value.data;
                        });
                  } else {
                     auto pk = certs.available_primary_key();
                     certs.emplace(code, [&](certrow& row) {
                           row.id = pk;
                           row.property   = value.property;
                           row.trusted    = trusted;
                           row.certifier  = cert.certifier;
                           row.confidence = value.confidence;
                           row.type       = value.type;
                           row.data       = value.data;
                        });
                  }

                  auto itr_old = idx.lower_bound( certrow::key(value.property, !trusted, cert.certifier) );
                  if (itr_old != idx.end() && itr_old->property == value.property && itr_old->trusted == !trusted && itr_old->certifier == cert.certifier) {
                     idx.erase(itr_old);
                  }

                  //special handling for owner
                  if (value.property == N(owner)) {
                     eosio_assert(sizeof(account_name) == value.data.size(), "data size doesn't match account_name size");
                     account_name acnt = *reinterpret_cast<const account_name*>(value.data.data());
                     if (cert.certifier == acnt) { //only self-certitication affects accounts_table
                        accounts_table::set( cert.identity, acnt );
                     }
                  }
               } else {
                  bool removed = false;
                  auto itr = idx.lower_bound( certrow::key(value.property, trusted, cert.certifier) );
                  if (itr != idx.end() && itr->property == value.property && itr->trusted == trusted && itr->certifier == cert.certifier) {
                     idx.erase(itr);
                  } else {
                     removed = true;
                  }
                  itr = idx.lower_bound( certrow::key(value.property, !trusted, cert.certifier) );
                  if (itr != idx.end() && itr->property == value.property && itr->trusted == !trusted && itr->certifier == cert.certifier) {
                     idx.erase(itr);
                  } else {
                     removed = true;
                  }
                  //special handling for owner
                  if (value.property == N(owner)) {
                     eosio_assert(sizeof(account_name) == value.data.size(), "data size doesn't match account_name size");
                     account_name acnt = *reinterpret_cast<const account_name*>(value.data.data());
                     if (cert.certifier == acnt) { //only self-certitication affects accounts_table
                        accounts_table::remove( acnt );
                     }
                  }
               }
            }
         }

         static void apply( account_name c, action_name act) {
            eosio::dispatch<contract, create, certprop, settrust>(c,act);
         }

   };

} /// namespace identity
