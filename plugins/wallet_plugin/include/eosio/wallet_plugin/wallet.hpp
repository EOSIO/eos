/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/wallet_plugin/wallet_api.hpp>

#include <fc/real128.hpp>
#include <fc/crypto/base58.hpp>

using namespace std;
using namespace eosio::chain;

namespace eosio { namespace wallet {

typedef uint16_t transaction_handle_type;

struct wallet_data
{
   vector<char>              cipher_keys; /** encrypted keys */
};

namespace detail {
class soft_wallet_impl;
}

/**
 * This wallet assumes it is connected to the database server with a high-bandwidth, low-latency connection and
 * performs minimal caching.
 */
class soft_wallet final : public wallet_api
{
   public:
      soft_wallet( const wallet_data& initial_data );

      ~soft_wallet();

      bool copy_wallet_file( string destination_filename );

      /** Returns the current wallet filename.
       *
       * This is the filename that will be used when automatically saving the wallet.
       *
       * @see set_wallet_filename()
       * @return the wallet filename
       */
      string                            get_wallet_filename() const;

      /**
       * Get the WIF private key corresponding to a public key.  The
       * private key must already be in the wallet.
       */
      private_key_type get_private_key( public_key_type pubkey )const override;

      /**
       *  @param role - active | owner | posting | memo
       */
      pair<public_key_type,private_key_type>  get_private_key_from_password( string account, string role, string password )const;

      /** Checks whether the wallet has just been created and has not yet had a password set.
       *
       * Calling \c set_password will transition the wallet to the locked state.
       * @return true if the wallet is new
       * @ingroup Wallet Management
       */
      bool    is_new()const;

      /** Checks whether the wallet is locked (is unable to use its private keys).
       *
       * This state can be changed by calling \c lock() or \c unlock().
       * @return true if the wallet is locked
       * @ingroup Wallet Management
       */
      bool    is_locked()const override;

      /** Locks the wallet immediately.
       * @ingroup Wallet Management
       */
      void    lock() override;

      /** Unlocks the wallet.
       *
       * The wallet remain unlocked until the \c lock is called
       * or the program exits.
       * @param password the password previously set with \c set_password()
       * @ingroup Wallet Management
       */
      void    unlock(string password) override;

      /** Checks the password of the wallet
       *
       * Validates the password on a wallet even if the wallet is already unlocked,
       * throws if bad password given.
       * @param password the password previously set with \c set_password()
       * @ingroup Wallet Management
       */
      void    check_password(string password) override;

      /** Sets a new password on the wallet.
       *
       * The wallet must be either 'new' or 'unlocked' to
       * execute this command.
       * @ingroup Wallet Management
       */
      void    set_password(string password) override;

      /** Dumps all private keys owned by the wallet.
       *
       * The keys are printed in WIF format.  You can import these keys into another wallet
       * using \c import_key()
       * @returns a map containing the private keys, indexed by their public key
       */
      map<public_key_type, private_key_type> list_keys() override;
      
      /** Dumps all public keys owned by the wallet.
       * @returns a vector containing the public keys
       */
      flat_set<public_key_type> list_public_keys() override;

      /** Loads a specified Graphene wallet.
       *
       * The current wallet is closed before the new wallet is loaded.
       *
       * @warning This does not change the filename that will be used for future
       * wallet writes, so this may cause you to overwrite your original
       * wallet unless you also call \c set_wallet_filename()
       *
       * @param wallet_filename the filename of the wallet JSON file to load.
       *                        If \c wallet_filename is empty, it reloads the
       *                        existing wallet file
       * @returns true if the specified wallet is loaded
       */
      bool    load_wallet_file(string wallet_filename = "");

      /** Saves the current wallet to the given filename.
       *
       * @warning This does not change the wallet filename that will be used for future
       * writes, so think of this function as 'Save a Copy As...' instead of
       * 'Save As...'.  Use \c set_wallet_filename() to make the filename
       * persist.
       * @param wallet_filename the filename of the new wallet JSON file to create
       *                        or overwrite.  If \c wallet_filename is empty,
       *                        save to the current filename.
       */
      void    save_wallet_file(string wallet_filename = "");

      /** Sets the wallet filename used for future writes.
       *
       * This does not trigger a save, it only changes the default filename
       * that will be used the next time a save is triggered.
       *
       * @param wallet_filename the new filename to use for future saves
       */
      void    set_wallet_filename(string wallet_filename);

      /** Imports a WIF Private Key into the wallet to be used to sign transactions by an account.
       *
       * example: import_key 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
       *
       * @param wif_key the WIF Private Key to import
       */
      bool import_key( string wif_key ) override;

      /** Removes a key from the wallet.
       *
       * example: remove_key EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
       *
       * @param key the Public Key to remove
       */
      bool remove_key( string key ) override;

       /** Creates a key within the wallet to be used to sign transactions by an account.
       *
       * example: create_key K1
       *
       * @param key_type the key type to create. May be empty to allow wallet to pick appropriate/"best" key type
       */
      string create_key( string key_type ) override;

      /* Attempts to sign a digest via the given public_key
      */
      optional<signature_type> try_sign_digest( const digest_type digest, const public_key_type public_key ) override;

      std::shared_ptr<detail::soft_wallet_impl> my;
      void encrypt_keys();
};

struct plain_keys {
   fc::sha512                            checksum;
   map<public_key_type,private_key_type> keys;
};

} }

FC_REFLECT( eosio::wallet::wallet_data, (cipher_keys) )

FC_REFLECT( eosio::wallet::plain_keys, (checksum)(keys) )
