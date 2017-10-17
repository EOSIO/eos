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


namespace eosio { namespace blockchain {

   using std::vector;
   using std::map;
   using std::pair;
   using std::shared_ptr;
   using std::unique_ptr;
   using std::string;

   using boost::signals2::signal;
   using boost::asio::io_service;
   using boost::asio::strand;

   using fc::flat_set;
   using fc::flat_map;
   using fc::time_point;
   using fc::time_point_sec;
   using fc::static_variant;
   using fc::path;


   typedef fc::sha256       block_id_type;
   typedef fc::sha256       chain_id_type;
   typedef fc::sha256       message_id_type;
   typedef fc::sha256       transaction_id_type;
   typedef fc::sha256       merkle_id_type;

   typedef name             name_type;
   typedef name_type        account_name;
   typedef name_type        function_name;
   typedef name_type        permission_name;
   typedef vector<char>     bytes;


   typedef fc::ecc::compact_signature     signature_type;
   typedef eosio::blockchain::public_key  public_key_type;
   typedef fc::ecc::private_key           private_key_type;

   typedef map<account_name, pair< account_name,public_key_type> > producer_changes_type;

} }  /// eosio::blockchain
