#pragma once
#include <eosio/blockchain/name.hpp>
#include <eosio/blockchain/public_key.hpp>

#include <fc/crypto/sha256.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/time.hpp>
#include <fc/static_variant.hpp>
#include <fc/filesystem.hpp>

#include <boost/signals2/signal.hpp>
#include <boost/asio.hpp>

#include <vector>
#include <map>
#include <string>
#include <exception>

namespace eosio { namespace blockchain {

   using std::vector;
   using std::map;
   using std::set;
   using std::list;
   using std::pair;
   using std::make_pair;
   using std::weak_ptr;
   using std::shared_ptr;
   using std::make_shared;
   using std::unique_ptr;
   using std::make_unique;
   using std::string;
   using std::function;
   using std::forward;
   using std::move;

   using boost::signals2::signal;
   using boost::asio::io_service;
   using boost::asio::strand;

   using fc::flat_set;
   using fc::flat_map;
   using fc::time_point;
   using fc::time_point_sec;
   using fc::static_variant;
   using fc::path;
   using fc::optional;


   /**
    * The first 32 bits are the block number
    */
   typedef fc::sha256       block_id_type;
   typedef fc::sha256       chain_id_type;
   typedef fc::sha256       message_id_type;

   /**
    * The first byte indicates the type of the transaction id: 0 for user, 1 for generated, other bytes
    * bits in the first byte are reserved for future use.
    */
   typedef fc::sha256       transaction_id_type;
   typedef fc::sha256       merkle_id_type;
   typedef fc::sha256       digest_type;

   /*
   <h2>Reserved Names</h2>
   account names starting with "eosio." are considered reserved.  They cannot be created using the 'newaccount' transaction type

   <h3>System Account Names<h3>
   - eosio.accounts : this scope permits read and/or write access accounts
                      this scope is implicitly added to the write scopes for all of the following functions on the chain's system contract
                         - newaccount
                         - updateauth
                         - deleteauth
                         - unlinkauth
                         - linkauth
                       for ALL OTHER TRANSACTIONS this is implicitly added to the read scopes

    <h3>Virtual Account Names<h3>
    A virtual account is INVALID in an actual message or transaction on chain but can be used by client side tools to indicate special
    processing.
   
   - eosio.auto : this virtual account is used to indicate to client tools that the required set of scopes should be calculated

    */ 
   typedef name             name_type;
   typedef name_type        account_name;
   typedef name_type        function_name;
   typedef name_type        permission_name;
   typedef vector<char>     bytes;


   typedef fc::ecc::compact_signature     signature_type;
   typedef eosio::blockchain::public_key  public_key_type;
   typedef fc::ecc::private_key           private_key_type;
   typedef fc::sha256                     digest_type;


   typedef map<account_name, pair< account_name,public_key_type> > producer_changes_type;
   typedef uint32_t                       block_num_type;


   /*
    * switch to composition
    */
   template<typename Result>
   class async_result  {
      public:
         typedef fc::static_variant<Result, std::exception_ptr> storage_type;

         template<typename Arg>
         async_result( Arg&& success )
         :_value(forward<Arg>(success))
         {}

         Result&& get() {
            if (_value.template contains<std::exception_ptr>()) {
               std::rethrow_exception(_value.template get<std::exception_ptr>());
            } else {
               return move(_value.template get<Result>());
            }
         }

      private:
         storage_type _value;
   };

#define ESIO_CATCH_AND_THROW_TO_CALLBACK(cb)\
   catch (...) {\
      cb(std::current_exception());\
   }

} }  /// eosio::blockchain
