/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/faucet_testnet_plugin/faucet_testnet_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eos/utilities/key_conversion.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>
#include <fc/reflect/variant.hpp>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/clamp.hpp>

#include <utility>

namespace eosio { namespace detail {
  struct faucet_testnet_empty {};

  struct faucet_testnet_keys {
     std::string owner;
     std::string active;
  };

  struct faucet_testnet_create_account_params {
     std::string account;
     faucet_testnet_keys keys;
  };

  struct faucet_testnet_create_account_alternates_response {
     std::vector<chain::account_name> alternates;
     std::string message;
  };

  struct faucet_testnet_create_account_rate_limited_response {
     std::string message;
  };
}}

FC_REFLECT(eosio::detail::faucet_testnet_empty, );
FC_REFLECT(eosio::detail::faucet_testnet_keys, (owner)(active));
FC_REFLECT(eosio::detail::faucet_testnet_create_account_params, (account)(keys));
FC_REFLECT(eosio::detail::faucet_testnet_create_account_alternates_response, (alternates)(message));
FC_REFLECT(eosio::detail::faucet_testnet_create_account_rate_limited_response, (message));

namespace eosio {

static appbase::abstract_plugin& _faucet_testnet_plugin = app().register_plugin<faucet_testnet_plugin>();

using namespace eosio::chain;
using public_key_type = chain::public_key_type;
using key_pair = std::pair<std::string, std::string>;
using results_pair = std::pair<uint32_t,fc::variant>;

#define CALL(api_name, api_handle, call_name, invoke_cb) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this](string, string body, url_response_callback response_cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             const auto result = api_handle->invoke_cb(body); \
             response_cb(result.first, fc::json::to_string(result.second)); \
          } catch (fc::eof_exception& e) { \
             error_results results{400, "Bad Request", e.to_string()}; \
             response_cb(400, fc::json::to_string(results)); \
             elog("Unable to parse arguments: ${args}", ("args", body)); \
          } catch (fc::exception& e) { \
             error_results results{500, "Internal Service Error", e.to_detail_string()}; \
             response_cb(500, fc::json::to_string(results)); \
             elog("Exception encountered while processing ${call}: ${e}", ("call", #api_name "." #call_name)("e", e)); \
          } \
       }}

struct faucet_testnet_plugin_impl {
   struct create_faucet_account_alternate_results {
      std::vector<std::string> alternates;
   };

   faucet_testnet_plugin_impl(appbase::application& app)
   : _app(app)
   , _timer{app.get_io_service()}
   {
   }

   void timer_fired() {
      _blocking_accounts = false;
   }

   enum http_return_codes {
      account_created = 201,
      conflict_with_alternates = 409,
      too_many_requests = 429
   };

   class extension {

   public:
      explicit extension(uint32_t capacity)
      : max_capacity(capacity)
      {
      }

      bool increment() {
         bool incremented = false;
         // start incrementing from the end back to the begining
         auto rev_itr = ext.rbegin();
         for (; rev_itr != ext.rend(); ++rev_itr) {
            // done once a character is incremented
            if (increment(*rev_itr)) {
               std::string temp(1, *rev_itr);
               incremented = true;
               break;
            }
         }

         const bool add_char = (rev_itr == ext.rend());

         if (incremented)
            return true;
         else if (add_char && ext.length() >= max_capacity) {
            return false;
         }

         if (!ext.empty()) {
            do {
               --rev_itr;
               *rev_itr = init_char;
               std::string temp(1, *rev_itr);
            } while(rev_itr != ext.rbegin());
         }

         if (add_char) {
            ext.push_back(init_char);
         }
         return true;
      }

      std::string to_string() {
         return ext;
      }

      // assuming 13 characters, which will mean inefficiencies if forced to search with 13th char, since only 1 through j is valid there
      static const uint32_t max_allowed_name_length = 13;

   private:
      // NOTE: code written expecting struct name to be using charmap of ".12345abcdefghijklmnopqrstuvwxyz"
      bool increment(char& c) {
         std::string temp(1, c);
         if (c >= '1' && c < '5')
            ++c;
         else if (c == '5')
            c = 'a';
         else if (c >= 'a' && c < 'z')
            ++c;
         else
            return false;

         temp.assign(1, c);
         return true;
      }

      const uint32_t max_capacity;
      std::string ext;
      const char init_char = '1';
   };

   results_pair find_alternates(const std::string& new_account_name) {
      std::vector<account_name> names;
      std::string suggestion = new_account_name;
      while (names.size() < _default_create_alternates_to_return &&
             !suggestion.empty()) {
         const int32_t extension_capacity = extension::max_allowed_name_length - suggestion.length();
         if (extension_capacity > 0) {
            extension ext(extension_capacity);
            while(names.size() < _default_create_alternates_to_return && ext.increment()) {
               chain::account_name alt;
               // not externalizing all the name struct encoding, so need to handle cases where we
               // construct an invalid encoded name
               try {
                  alt = suggestion + ext.to_string();
               } catch (const fc::assert_exception& ) {
                  continue;
               }
               const account_object* const obj = database().find<account_object, by_name>(alt);
               if (obj == nullptr) {
                  names.push_back(alt);
               }
            }
         }
         // drop the trailing character and try again
         suggestion.pop_back();
      }

      const eosio::detail::faucet_testnet_create_account_alternates_response response{
         names, "Account name is already in use."};
      return { conflict_with_alternates, fc::variant(response) };
   }

   results_pair create_account(const std::string& new_account_name, const fc::crypto::public_key& owner_pub_key, const fc::crypto::public_key& active_pub_key) {

      auto creating_account = database().find<account_object, by_name>(_create_account_name);
      EOS_ASSERT(creating_account != nullptr, transaction_exception,
                 "To create account using the faucet, must already have created account \"${a}\"",("a",_create_account_name));

      auto existing_account = database().find<account_object, by_name>(new_account_name);
      if (existing_account != nullptr)
      {
         return find_alternates(new_account_name);
      }

      if (_blocking_accounts)
      {
         eosio::detail::faucet_testnet_create_account_rate_limited_response response{
            "Rate limit exceeded, the max is 1 request per " + fc::to_string(_create_interval_msec) +
            " milliseconds. Come back later."};
         return std::make_pair(too_many_requests, fc::variant(response));
      }

      chain::chain_id_type chainid;
      auto& plugin = _app.get_plugin<chain_plugin>();
      plugin.get_chain_id(chainid);
      chain_controller& cc = plugin.chain();
      const uint64_t deposit = 1;

      signed_transaction trx;
      auto memo = fc::variant(fc::time_point::now()).as_string() + " " + fc::variant(fc::time_point::now().time_since_epoch()).as_string();

      //create "A" account
      auto owner_auth   = chain::authority{1, {{owner_pub_key, 1}}, {}};
      auto active_auth  = chain::authority{1, {{active_pub_key, 1}}, {}};
      auto recovery_auth = chain::authority{1, {}, {{{_create_account_name, "active"}, 1}}};

      trx.actions.emplace_back(vector<chain::permission_level>{{_create_account_name,"active"}},
                               contracts::newaccount{_create_account_name, new_account_name, owner_auth, active_auth, recovery_auth, deposit});

      trx.expiration = cc.head_block_time() + fc::seconds(30);
      trx.set_reference_block(cc.head_block_id());
      trx.sign(_create_account_private_key, chainid);

      try {
         cc.push_transaction(trx);
      } catch (const account_name_exists_exception& ) {
         // another transaction ended up adding the account, so look for alternates
         return find_alternates(new_account_name);
      }

      _blocking_accounts = true;
      _timer.expires_from_now(boost::posix_time::microseconds(_create_interval_msec * 1000));
      _timer.async_wait(boost::bind(&faucet_testnet_plugin_impl::timer_fired, this));

      return std::make_pair(account_created, fc::variant(eosio::detail::faucet_testnet_empty()));
   }

   results_pair create_faucet_account(const std::string& body) {
      const eosio::detail::faucet_testnet_create_account_params params = fc::json::from_string(body).as<eosio::detail::faucet_testnet_create_account_params>();
      return create_account(params.account, fc::crypto::public_key(params.keys.owner), fc::crypto::public_key(params.keys.active));
   }

   const chainbase::database& database() {
      static const chainbase::database* db = nullptr;
      if (db == nullptr)
         db = &_app.get_plugin<chain_plugin>().chain().get_database();

      return *db;
   }

   appbase::application& _app;
   boost::asio::deadline_timer _timer;
   bool _blocking_accounts = false;

   static const uint32_t _default_create_interval_msec;
   uint32_t _create_interval_msec;
   static const uint32_t _default_create_alternates_to_return;
   static const std::string _default_create_account_name;
   chain::account_name _create_account_name;
   static const key_pair _default_key_pair;
   fc::crypto::private_key _create_account_private_key;
   public_key_type _create_account_public_key;
};

const uint32_t faucet_testnet_plugin_impl::_default_create_interval_msec = 1000;
const uint32_t faucet_testnet_plugin_impl::_default_create_alternates_to_return = 3;
const std::string faucet_testnet_plugin_impl::_default_create_account_name = "faucet";
// defaults to the public/private key of init accounts in private testnet genesis.json
const key_pair faucet_testnet_plugin_impl::_default_key_pair = {"EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV", "5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3"};


faucet_testnet_plugin::faucet_testnet_plugin()
: my(new faucet_testnet_plugin_impl(app()))
{
}

faucet_testnet_plugin::~faucet_testnet_plugin() {}

void faucet_testnet_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("faucet-create-interval-ms", bpo::value<uint32_t>()->default_value(faucet_testnet_plugin_impl::_default_create_interval_msec),
          "Time to wait, in milliseconds, between creating next faucet created account.")
         ("faucet-name", bpo::value<std::string>()->default_value(faucet_testnet_plugin_impl::_default_create_account_name),
          "Name to use as creator for faucet created accounts.")
         ("faucet-private-key", boost::program_options::value<std::string>()->default_value(fc::json::to_string(faucet_testnet_plugin_impl::_default_key_pair)),
          "[public key, WIF private key] for signing for faucet creator account")
         ;
}

void faucet_testnet_plugin::plugin_initialize(const variables_map& options) {
   my->_create_interval_msec = options.at("faucet-create-interval-ms").as<uint32_t>();
   my->_create_account_name = options.at("faucet-name").as<std::string>();

   auto faucet_key_pair = fc::json::from_string(options.at("faucet-private-key").as<std::string>()).as<key_pair>();
   my->_create_account_public_key = public_key_type(faucet_key_pair.first);
   ilog("Public Key: ${public}", ("public", my->_create_account_public_key));
   fc::crypto::private_key private_key(faucet_key_pair.second);
   my->_create_account_private_key = std::move(private_key);
}

void faucet_testnet_plugin::plugin_startup() {
   app().get_plugin<http_plugin>().add_api({
      CALL(faucet, my, create_account, faucet_testnet_plugin_impl::create_faucet_account )
   });
}

void faucet_testnet_plugin::plugin_shutdown() {
   try {
      my->_timer.cancel();
   } catch(fc::exception& e) {
      edump((e.to_detail_string()));
   }
}

}
