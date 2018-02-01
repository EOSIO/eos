#pragma once

#include <eoslib/chain.h>
#include <eoslib/dispatcher.hpp>
#include <eoslib/singleton.hpp>
#include <eoslib/table.hpp>
#include <eoslib/vector.hpp>

namespace identity {
   using eosio::action_meta;
   using eosio::table_i64i64i64;
   using eosio::table64;
   using eosio::singleton;
   using eosio::string;
   using eosio::vector;

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

            template<typename DataStream>
            friend DataStream& operator << ( DataStream& ds, const create& c ){
               return ds << c.creator << c.identity;
            }
            template<typename DataStream>
            friend DataStream& operator >> ( DataStream& ds, create& c ){
               return ds >> c.creator >> c.identity;
            }
         };


         struct certvalue {
            property_name property; ///< name of property, base32 encoded i64
            string     type; ///< defines type serialized in data
            vector<char>  data; ///< 
            string        memo; ///< meta data documenting basis of certification
            uint8_t       confidence = 1; ///< used to define liability for lies, 
                                          /// 0 to delete
            template<typename DataStream>
            friend DataStream& operator << ( DataStream& ds, const certvalue& c ){
               return ds << c.property << c.type << c.data << c.memo << c.confidence;
            }
            template<typename DataStream>
            friend DataStream& operator >> ( DataStream& ds, certvalue& c ){
               return ds >> c.property >> c.type >> c.data >> c.memo >> c.confidence;
            }
         };

         struct certprop : public action_meta< code, N(certprop) > 
         {
            account_name        bill_storage_to; ///< account which is paying for storage
            account_name        certifier; 
            identity_name       identity;
            vector<certvalue>   values;

            template<typename DataStream>
            friend DataStream& operator << ( DataStream& ds, const certprop& c ){
               return ds << c.bill_storage_to << c.certifier << c.identity << c.values;
            }
            template<typename DataStream>
            friend DataStream& operator >> ( DataStream& ds, certprop& c ){
               return ds >> c.bill_storage_to >> c.certifier >> c.identity >> c.values;
            }
         };

         struct settrust : public action_meta< code, N(settrust) > 
         {
            account_name trustor; ///< the account authorizing the trust
            account_name trusting; ///< the account receiving the trust
            uint8_t      trust = 0; /// 0 to remove, -1 to mark untrusted, 1 to mark trusted

            template<typename DataStream>
            friend DataStream& operator << ( DataStream& ds, const settrust& c ){
               return ds << c.trustor << c.trusting << c.trust;
            }
            template<typename DataStream>
            friend DataStream& operator >> ( DataStream& ds, settrust& c ){
               return ds >> c.trustor >> c.trusting >> c.trust;
            }
         };

         /**
          * Defines an object in an i64i64i64 table 
          */
         struct certrow {
            property_name       property;
            uint64_t            trusted;
            account_name        certifier;
            uint8_t             confidence = 0;
            string              type;
            vector<char>        data;

            template<typename DataStream>
            friend DataStream& operator << ( DataStream& ds, const certrow& r ){
               return ds << r.property << r.trusted << r.certifier << r.confidence << r.type << r.data;
            }
            template<typename DataStream>
            friend DataStream& operator >> ( DataStream& ds, certrow& r ){
               return ds >> r.property >> r.trusted >> r.certifier >> r.confidence >> r.type >> r.data;
            }
         };

         struct identrow {
            uint64_t     identity; 
            account_name creator;

            template<typename DataStream>
            friend DataStream& operator << ( DataStream& ds, const identrow& r ){
               return ds << r.identity << r.creator;
            }
            template<typename DataStream>
            friend DataStream& operator >> ( DataStream& ds, identrow& r ){
               return ds >> r.identity >> r.creator;
            }
         };

         typedef table_i64i64i64<code, N(certs), certrow>   certs_table;
         typedef table64<code, N(ident), identrow>          idents_table;
         typedef singleton<code, N(account), identity_name> accounts_table;
         typedef table64<code, N(trust), account_name>      trust_table;

         static identity_name get_claimed_identity( account_name acnt ) {
            return accounts_table::get_or_default(acnt, 0);
         }

         static account_name get_owner_for_identity( identity_name ident ) {
            // for each trusted owner certification 
            //   check to see if the certification is still trusted
            //   check to see if the account has claimed it
            certs_table certs( ident );
            certrow row;
            bool ok = certs.primary_lower_bound(row, N(owner), 1, 0);
            account_name owner = 0;
            while (ok && row.property == N(owner) && row.trusted) {
               if (sizeof(account_name) == row.data.size()) {
                  account_name account = *reinterpret_cast<account_name*>(row.data.data());
                  if (ident == get_claimed_identity(account)) {
                     if (is_trusted(row.certifier) ) {
                        // the certifier is still trusted
                        if (!owner || owner == account) {
                           owner = account;
                        } else {
                           //contradiction found: different owners certified for the same identity
                           return 0;
                        }
                     } else if (DeployToAccount == current_receiver()){
                        //the certifier is no longer trusted, need to unset the flag
                        row.trusted = 0;
                        certs.store( row, 0 ); //assuming 0 means bill to the same account
                     } else {
                        // the certifier is no longer trusted, but the code runs in read-only mode
                     }
                  }
               } else {
                  // bad row - skip it
               }
               //ok = certs.primary_upper_bound(row, row.property, row.trusted, row.certifier);
               ok = certs.next_primary(row, row);
            }
            if (owner) {
               //owner found, no contradictions among certifications flaged as trusted
               return owner;
            }
            // trusted certification not found
            // let's see if some of untrusted certifications became trusted
            ok = certs.primary_lower_bound(row, N(owner), 0, 0);
            while (ok && row.property == N(owner) && !row.trusted) {
               if (sizeof(account_name) == row.data.size()) {
                  account_name account = *reinterpret_cast<account_name*>(row.data.data());
                  if (ident == get_claimed_identity(account) && is_trusted(row.certifier)) {
                     if (DeployToAccount == current_receiver()) {
                        // the certifier became trusted and we have permissions to update the flag
                        row.trusted = 1;
                        certs.store( row, 0 ); //assuming 0 means bill to the same account
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
               //ok = certs.primary_upper_bound(row, row.property, row.trusted, row.certifier);
               ok = certs.next_primary(row, row);
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
            return trust_table::exists(trusted, by);
         }

         static bool is_trusted( account_name acnt ) {
            account_name active_producers[21];
            get_active_producers( active_producers, sizeof(active_producers) );
            for( const auto& n : active_producers ) {
               if( n == acnt ) 
                  return true;
            }
            for( const auto& n : active_producers ) {
               if( is_trusted_by( acnt, n ) )
                  return true;
            }
            return false;
         }

         static void on( const settrust& t ) {
            require_auth( t.trustor );
            require_recipient( t.trusting );

            if( t.trust != 0 ) {
               trust_table::set( t.trusting, t.trustor );
            } else {
               trust_table::remove( t.trusting, t.trustor );
            }
         }

         static void on( const create& c ) {
            require_auth( c.creator );
            assert( !idents_table::exists( c.identity ), "identity already exists" );
            assert( c.identity != 0, "identity=0 is not allowed" );
            idents_table::set( identrow{ .identity = c.identity,
                                         .creator  = c.creator } );
         }

         static void on( const certprop& cert ) {
            require_auth( cert.certifier );
            if( cert.bill_storage_to != cert.certifier )
               require_auth( cert.bill_storage_to );

            assert( idents_table::exists( cert.identity ), "identity does not exist" );

            /// the table exists in the scope of the identity 
            certs_table certs( cert.identity );

            for( const auto& value : cert.values ) {
               if (value.confidence) {
                  certrow row;
                  row.property   = value.property;
                  row.trusted    = is_trusted( cert.certifier );
                  row.certifier  = cert.certifier;
                  row.confidence = value.confidence;
                  assert(value.type.get_size() <= 32, "certrow::type should be not longer than 32 bytes");
                  row.type       = value.type;
                  row.data       = value.data;

                  certs.store( row, cert.bill_storage_to );
                  //remove row with different "trusted" value
                  certs.remove(value.property, !row.trusted, cert.certifier);

                  //special handling for owner
                  if (value.property == N(owner)) {
                     assert(sizeof(account_name) == value.data.size(), "data size doesn't match account_name size");
                     account_name acnt = *reinterpret_cast<const account_name*>(value.data.data());
                     if (cert.certifier == acnt) { //only self-certitication affects accounts_table
                        accounts_table::set( cert.identity, acnt );
                     }
                  }
               } else {
                  //remove both tursted and untrusted because we cannot know if it was trusted back at creation time
                  certs.remove(value.property, 0, cert.certifier);
                  certs.remove(value.property, 1, cert.certifier);
                  //special handling for owner
                  if (value.property == N(owner)) {
                     assert(sizeof(account_name) == value.data.size(), "data size doesn't match account_name size");
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
