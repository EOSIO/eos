#include <eosio/wallet_plugin/wallet.hpp>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <list>

#include <fc/container/deque.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/git_revision.hpp>
#include <fc/io/json.hpp>
#include <fc/crypto/aes.hpp>
#include <fc/crypto/hex.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

#ifndef WIN32
# include <sys/types.h>
# include <sys/stat.h>
#include <eosio/chain/exceptions.hpp>

#endif

namespace eosio { namespace wallet {

namespace detail {

private_key_type derive_private_key( const std::string& prefix_string,
                                         int sequence_number )
{
   std::string sequence_string = std::to_string(sequence_number);
   fc::sha512 h = fc::sha512::hash(prefix_string + " " + sequence_string);
   return private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(h));
}

class soft_wallet_impl
{
   private:
      void enable_umask_protection() {
#ifdef __unix__
         _old_umask = umask( S_IRWXG | S_IRWXO );
#endif
      }

      void disable_umask_protection() {
#ifdef __unix__
         umask( _old_umask );
#endif
      }

public:
   soft_wallet& self;
   soft_wallet_impl( soft_wallet& s, const wallet_data& initial_data )
      : self( s )
   {
   }

   virtual ~soft_wallet_impl()
   {}

   void encrypt_keys()
   {
      if( !is_locked() )
      {
         plain_keys data;
         data.keys = _keys;
         data.checksum = _checksum;
         auto plain_txt = fc::raw::pack(data);
         _wallet.cipher_keys = fc::aes_encrypt( data.checksum, plain_txt );
      }
   }

   bool copy_wallet_file( string destination_filename )
   {
      fc::path src_path = get_wallet_filename();
      if( !fc::exists( src_path ) )
         return false;
      fc::path dest_path = destination_filename + _wallet_filename_extension;
      int suffix = 0;
      while( fc::exists(dest_path) )
      {
         ++suffix;
         dest_path = destination_filename + "-" + std::to_string( suffix ) + _wallet_filename_extension;
      }
      wlog( "backing up wallet ${src} to ${dest}",
            ("src", src_path)
            ("dest", dest_path) );

      fc::path dest_parent = fc::absolute(dest_path).parent_path();
      try
      {
         enable_umask_protection();
         if( !fc::exists( dest_parent ) )
            fc::create_directories( dest_parent );
         fc::copy( src_path, dest_path );
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
      return true;
   }

   bool is_locked()const
   {
      return _checksum == fc::sha512();
   }

   string get_wallet_filename() const { return _wallet_filename; }

   std::optional<private_key_type>  try_get_private_key(const public_key_type& id)const
   {
      auto it = _keys.find(id);
      if( it != _keys.end() )
         return  it->second;
      return std::optional<private_key_type>();
   }

   std::optional<signature_type> try_sign_digest( const digest_type digest, const public_key_type public_key ) {
      auto it = _keys.find(public_key);
      if( it == _keys.end() )
         return std::optional<signature_type>();
      return it->second.sign(digest);
   }

   private_key_type get_private_key(const public_key_type& id)const
   {
      auto has_key = try_get_private_key( id );
      EOS_ASSERT( has_key, chain::key_nonexistent_exception, "Key doesn't exist!" );
      return *has_key;
   }


   // imports the private key into the wallet, and associate it in some way (?) with the
   // given account name.
   // @returns true if the key matches a current active/owner/memo key for the named
   //          account, false otherwise (but it is stored either way)
   bool import_key(string wif_key)
   {
      private_key_type priv(wif_key);
      eosio::chain::public_key_type wif_pub_key = priv.get_public_key();

      auto itr = _keys.find(wif_pub_key);
      if( itr == _keys.end() ) {
         _keys[wif_pub_key] = priv;
         return true;
      }
      EOS_THROW( chain::key_exist_exception, "Key already in wallet" );
   }

   // Removes a key from the wallet
   // @returns true if the key matches a current active/owner/memo key for the named
   //          account, false otherwise (but it is removed either way)
   bool remove_key(string key)
   {
      public_key_type pub(key);
      auto itr = _keys.find(pub);
      if( itr != _keys.end() ) {
         _keys.erase(pub);
         return true;
      }
      EOS_THROW( chain::key_nonexistent_exception, "Key not in wallet" );
   }

   string create_key(string key_type)
   {
      if(key_type.empty())
         key_type = _default_key_type;

      private_key_type priv_key;
      if(key_type == "K1")
         priv_key = fc::crypto::private_key::generate<fc::ecc::private_key_shim>();
      else if(key_type == "R1")
         priv_key = fc::crypto::private_key::generate<fc::crypto::r1::private_key_shim>();
      else
         EOS_THROW(chain::unsupported_key_type_exception, "Key type \"${kt}\" not supported by software wallet", ("kt", key_type));

      import_key(priv_key.to_string());
      return priv_key.get_public_key().to_string();
   }

   bool load_wallet_file(string wallet_filename = "")
   {
      // TODO:  Merge imported wallet with existing wallet,
      //        instead of replacing it
      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      if( ! fc::exists( wallet_filename ) )
         return false;

      _wallet = fc::json::from_file( wallet_filename ).as< wallet_data >();

      return true;
   }

   void save_wallet_file(string wallet_filename = "")
   {
      //
      // Serialize in memory, then save to disk
      //
      // This approach lessens the risk of a partially written wallet
      // if exceptions are thrown in serialization
      //

      encrypt_keys();

      if( wallet_filename == "" )
         wallet_filename = _wallet_filename;

      wlog( "saving wallet to file ${fn}", ("fn", wallet_filename) );

      string data = fc::json::to_pretty_string( _wallet );
      try
      {
         enable_umask_protection();
         //
         // Parentheses on the following declaration fails to compile,
         // due to the Most Vexing Parse.  Thanks, C++
         //
         // http://en.wikipedia.org/wiki/Most_vexing_parse
         //
         ofstream outfile{ wallet_filename };
         if (!outfile) {
            elog("Unable to open file: ${fn}", ("fn", wallet_filename));
            EOS_THROW(wallet_exception, "Unable to open file: ${fn}", ("fn", wallet_filename));
         }
         outfile.write( data.c_str(), data.length() );
         outfile.flush();
         outfile.close();
         disable_umask_protection();
      }
      catch(...)
      {
         disable_umask_protection();
         throw;
      }
   }

   string                                  _wallet_filename;
   wallet_data                             _wallet;

   map<public_key_type,private_key_type>   _keys;
   fc::sha512                              _checksum;

#ifdef __unix__
   mode_t                  _old_umask;
#endif
   const string _wallet_filename_extension = ".wallet";
   const string _default_key_type = "K1";
};

} } } // eosio::wallet::detail



namespace eosio { namespace wallet {

soft_wallet::soft_wallet(const wallet_data& initial_data)
   : my(new detail::soft_wallet_impl(*this, initial_data))
{}

soft_wallet::~soft_wallet() {}

bool soft_wallet::copy_wallet_file(string destination_filename)
{
   return my->copy_wallet_file(destination_filename);
}

string soft_wallet::get_wallet_filename() const
{
   return my->get_wallet_filename();
}

bool soft_wallet::import_key(string wif_key)
{
   EOS_ASSERT(!is_locked(), wallet_locked_exception, "Unable to import key on a locked wallet");

   if( my->import_key(wif_key) )
   {
      save_wallet_file();
      return true;
   }
   return false;
}

bool soft_wallet::remove_key(string key)
{
   EOS_ASSERT(!is_locked(), wallet_locked_exception, "Unable to remove key from a locked wallet");

   if( my->remove_key(key) )
   {
      save_wallet_file();
      return true;
   }
   return false;
}

string soft_wallet::create_key(string key_type)
{
   EOS_ASSERT(!is_locked(), wallet_locked_exception, "Unable to create key on a locked wallet");

   string ret = my->create_key(key_type);
   save_wallet_file();
   return ret;
}

bool soft_wallet::load_wallet_file( string wallet_filename )
{
   return my->load_wallet_file( wallet_filename );
}

void soft_wallet::save_wallet_file( string wallet_filename )
{
   my->save_wallet_file( wallet_filename );
}

bool soft_wallet::is_locked() const
{
   return my->is_locked();
}

bool soft_wallet::is_new() const
{
   return my->_wallet.cipher_keys.size() == 0;
}

void soft_wallet::encrypt_keys()
{
   my->encrypt_keys();
}

void soft_wallet::lock()
{ try {
   EOS_ASSERT( !is_locked(), wallet_locked_exception, "Unable to lock a locked wallet" );
   encrypt_keys();
   for( auto key : my->_keys )
      key.second = private_key_type();

   my->_keys.clear();
   my->_checksum = fc::sha512();
} FC_CAPTURE_AND_RETHROW() }

void soft_wallet::unlock(string password)
{ try {
   FC_ASSERT(password.size() > 0);
   auto pw = fc::sha512::hash(password.c_str(), password.size());
   vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
   auto pk = fc::raw::unpack<plain_keys>(decrypted);
   FC_ASSERT(pk.checksum == pw);
   my->_keys = std::move(pk.keys);
   my->_checksum = pk.checksum;
} EOS_RETHROW_EXCEPTIONS(chain::wallet_invalid_password_exception,
                          "Invalid password for wallet: \"${wallet_name}\"", ("wallet_name", get_wallet_filename())) }

void soft_wallet::check_password(string password)
{ try {
   FC_ASSERT(password.size() > 0);
   auto pw = fc::sha512::hash(password.c_str(), password.size());
   vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
   auto pk = fc::raw::unpack<plain_keys>(decrypted);
   FC_ASSERT(pk.checksum == pw);
} EOS_RETHROW_EXCEPTIONS(chain::wallet_invalid_password_exception,
                          "Invalid password for wallet: \"${wallet_name}\"", ("wallet_name", get_wallet_filename())) }

void soft_wallet::set_password( string password )
{
   if( !is_new() )
      EOS_ASSERT( !is_locked(), wallet_locked_exception, "The wallet must be unlocked before the password can be set" );
   my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
   lock();
}

map<public_key_type, private_key_type> soft_wallet::list_keys()
{
   EOS_ASSERT(!is_locked(), wallet_locked_exception, "Unable to list public keys of a locked wallet");
   return my->_keys;
}

flat_set<public_key_type> soft_wallet::list_public_keys() {
   EOS_ASSERT(!is_locked(), wallet_locked_exception, "Unable to list private keys of a locked wallet");
   flat_set<public_key_type> keys;
   boost::copy(my->_keys | boost::adaptors::map_keys, std::inserter(keys, keys.end()));
   return keys;
}

private_key_type soft_wallet::get_private_key( public_key_type pubkey )const
{
   return my->get_private_key( pubkey );
}

std::optional<signature_type> soft_wallet::try_sign_digest( const digest_type digest, const public_key_type public_key ) {
   return my->try_sign_digest(digest, public_key);
}

pair<public_key_type,private_key_type> soft_wallet::get_private_key_from_password( string account, string role, string password )const {
   auto seed = account + role + password;
   EOS_ASSERT( seed.size(), wallet_exception, "seed should not be empty" );
   auto secret = fc::sha256::hash( seed.c_str(), seed.size() );
   auto priv = private_key_type::regenerate<fc::ecc::private_key_shim>( secret );
   return std::make_pair(  priv.get_public_key(), priv );
}

void soft_wallet::set_wallet_filename(string wallet_filename)
{
   my->_wallet_filename = wallet_filename;
}

} } // eosio::wallet

