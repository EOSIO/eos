/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
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
#include <fc/smart_ref_impl.hpp>

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

class wallet_api_impl
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
   wallet_api& self;
   wallet_api_impl( wallet_api& s, const wallet_data& initial_data )
      : self( s )
   {
   }

   virtual ~wallet_api_impl()
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

   optional<private_key_type>  try_get_private_key(const public_key_type& id)const
   {
      auto it = _keys.find(id);
      if( it != _keys.end() )
         return  it->second;
      return optional<private_key_type>();
   }

   private_key_type get_private_key(const public_key_type& id)const
   {
      auto has_key = try_get_private_key( id );
      FC_ASSERT( has_key );
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
      FC_ASSERT( !"Key already in wallet" );
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
         FC_THROW_EXCEPTION(chain::wallet_exception, "Key type \"${kt}\" not supported by software wallet", ("kt", key_type));

      import_key((string)priv_key);
      return (string)priv_key.get_public_key();
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
            FC_THROW("Unable to open file: ${fn}", ("fn", wallet_filename));
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

wallet_api::wallet_api(const wallet_data& initial_data)
   : my(new detail::wallet_api_impl(*this, initial_data))
{}

bool wallet_api::copy_wallet_file(string destination_filename)
{
   return my->copy_wallet_file(destination_filename);
}

string wallet_api::get_wallet_filename() const
{
   return my->get_wallet_filename();
}

bool wallet_api::import_key(string wif_key)
{
   FC_ASSERT(!is_locked());

   if( my->import_key(wif_key) )
   {
      save_wallet_file();
      return true;
   }
   return false;
}

string wallet_api::create_key(string key_type)
{
   FC_ASSERT(!is_locked());

   string ret = my->create_key(key_type);
   save_wallet_file();
   return ret;
}

bool wallet_api::load_wallet_file( string wallet_filename )
{
   return my->load_wallet_file( wallet_filename );
}

void wallet_api::save_wallet_file( string wallet_filename )
{
   my->save_wallet_file( wallet_filename );
}

bool wallet_api::is_locked() const
{
   return my->is_locked();
}

bool wallet_api::is_new() const
{
   return my->_wallet.cipher_keys.size() == 0;
}

void wallet_api::encrypt_keys()
{
   my->encrypt_keys();
}

void wallet_api::lock()
{ try {
   FC_ASSERT( !is_locked() );
   encrypt_keys();
   for( auto key : my->_keys )
      key.second = private_key_type();

   my->_keys.clear();
   my->_checksum = fc::sha512();
} FC_CAPTURE_AND_RETHROW() }

void wallet_api::unlock(string password)
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

void wallet_api::check_password(string password)
{ try {
   FC_ASSERT(password.size() > 0);
   auto pw = fc::sha512::hash(password.c_str(), password.size());
   vector<char> decrypted = fc::aes_decrypt(pw, my->_wallet.cipher_keys);
   auto pk = fc::raw::unpack<plain_keys>(decrypted);
   FC_ASSERT(pk.checksum == pw);
} EOS_RETHROW_EXCEPTIONS(chain::wallet_invalid_password_exception,
                          "Invalid password for wallet: \"${wallet_name}\"", ("wallet_name", get_wallet_filename())) }

void wallet_api::set_password( string password )
{
   if( !is_new() )
      FC_ASSERT( !is_locked(), "The wallet must be unlocked before the password can be set" );
   my->_checksum = fc::sha512::hash( password.c_str(), password.size() );
   lock();
}

map<public_key_type, private_key_type> wallet_api::list_keys()
{
   FC_ASSERT(!is_locked());
   return my->_keys;
}

private_key_type wallet_api::get_private_key( public_key_type pubkey )const
{
   return my->get_private_key( pubkey );
}

optional<private_key_type> wallet_api::try_get_private_key(const public_key_type& id)const
{
   return my->try_get_private_key(id);
}


pair<public_key_type,private_key_type> wallet_api::get_private_key_from_password( string account, string role, string password )const {
   auto seed = account + role + password;
   FC_ASSERT( seed.size() );
   auto secret = fc::sha256::hash( seed.c_str(), seed.size() );
   auto priv = private_key_type::regenerate<fc::ecc::private_key_shim>( secret );
   return std::make_pair(  priv.get_public_key(), priv );
}

void wallet_api::set_wallet_filename(string wallet_filename)
{
   my->_wallet_filename = wallet_filename;
}

} } // eosio::wallet

