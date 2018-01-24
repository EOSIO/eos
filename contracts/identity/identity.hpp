#include <eoslib/vector.hpp>

namespace identity {

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
         typedef uint64_t property_name;
         typedef uint64_t property_type_name;

         enum reserved_names {
            owner      = N(owner),
            trusted    = N(trusted),
            unique     = N(unique),
            firstname  = N(firstname),
            lastname   = N(lastname),
            midname    = N(midname),
            unique     = N(unique)
         };


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
         };


         struct certvalue {
            property_name property; ///< name of property, base32 encoded i64
            type_name     type; ///< defines type serialized in data
            vector<char>  data; ///< 
            string        memo; ///< meta data documenting basis of certification
            uint8_t       confidence = 1; ///< used to define liability for lies, 
                                          /// 0 to delete
         };

         struct certprop : public action_meta< code, N(certprop) > 
         {
            account_name        bill_storage_to; ///< account which is paying for storage
            account_name        certifier; 
            identity_name       identity;
            vector<certvalue>   values;
         };

         struct settrust : public action_meta< code, N(settrust) > 
         {
            account_name trustor; ///< the account authorizing the trust
            account_name trusting; ///< the account receiving the trust
            uint8_t      trust = 0; /// 0 to remove, -1 to mark untrusted, 1 to mark trusted
         };

         /**
          * Defines an object in an i64i64i64 table 
          */
         struct certrow {
            property_name       property;
            uint64_t            trusted;
            account_name        certifier;
            uint64_t            confidence = 0;
            type_name           type;
            vector<char>        data;
         };

         struct identrow {
            uint64_t     identity; 
            account_name creator;
         };

         struct trustrow {
            account_name account;
            uint8_t      trusted;
         };

         typedef table_i64i64i64<code, N(certs), certrow>  certs_table;
         typedef table_i64<code, N(ident), identrow>       idents_table;
         typedef table_i64<code, N(trust), trustrow>       trust_table;

         static account_name get_owner_for_identity( identity_name ident ) {
             // for each trusted owner certification 
             //   check to see if the certification is still trusted
             //   check to see if the account has claimed it
             return 0;
         }

         static identity_name get_identity_for_account( account_name acnt ) {
             //  check what identity the account has self certified owner
             //  verify that a trusted certifier has confirmed owner
             return 0;
         }

         static bool is_trusted_by( account_name trusted, account_name by ) {
            return false;
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
            require_notice( t.trusting );

            if( t.trust != 0 ) {
               trustrow  row{ .account = t.trusting;
                              .trusted = t.trust; }
               trust_table::set( row, t.trustor );
            } else {
               trustrow  row{ .account = t.trusting;
                              .trusted = t.trust; }
               trust_table::remove( row, t.trustor );
            }
         }

         static void on( const create& c ) {
            require_auth( c );
            assert( !idents_table::exists( c.identity ), "identity already exists" );
            idents_table::set( identrow{ .identity = c.identity,
                                         .creator  = c.creator } );
         }

         static void on( const certprop& cert ) {
            require_auth( cert.certifier );
            if( cert.bill_storage_to != cert.certifier )
               require_auth( bill_storage_to );

            assert( !idents_table::exists( cert.identity ), "identity does not exist" );

            /// the table exists in the scope of the identity 
            certs_table certs( cert.identity );

            for( const auto& value : cert.values ) {
               certrow row;
               row.property   = value.property;
               row.trusted    = is_trusted( cert.certifier );
               row.certifier  = cert.certifier;
               row.confidence = value.confidence;
               row.type       = value.type;
               row.data       = value.data;

               certs.store( row, cert.bill_storage_to );
            }
         }

         static void apply( account_name c, action_name act) {
            eosio::dispatch<contract, create, certprop, settrust>(c,act);
         }

   };

} /// namespace identity
