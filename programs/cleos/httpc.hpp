/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

namespace eosio { namespace client { namespace http {

   namespace detail {
      class http_context_impl;

      struct http_context_deleter {
         void operator()(http_context_impl*) const;
      };
   }

   using http_context = std::unique_ptr<detail::http_context_impl, detail::http_context_deleter>;

   http_context create_http_context();

   struct parsed_url {
      string scheme;
      string server;
      string port;
      string path;

      static string normalize_path(const string& path);

      parsed_url operator+ (string sub_path) {
         return {scheme, server, port, path + sub_path};
      }
   };

   parsed_url parse_url( const string& server_url );

   struct resolved_url : parsed_url {
      resolved_url( const parsed_url& url, vector<string>&& resolved_addresses, uint16_t resolved_port, bool is_loopback)
      :parsed_url(url)
      ,resolved_addresses(std::move(resolved_addresses))
      ,resolved_port(resolved_port)
      ,is_loopback(is_loopback)
      {
      }

      //used for unix domain, where resolving and ports are nonapplicable
      resolved_url(const parsed_url& url) : parsed_url(url) {}

      vector<string> resolved_addresses;
      uint16_t resolved_port = 0;
      bool is_loopback = false;
   };

   resolved_url resolve_url( const http_context& context,
                             const parsed_url& url );

   struct connection_param {
      const http_context& context;
      resolved_url url;
      bool verify_cert;
      std::vector<string>& headers;

      connection_param( const http_context& context,
                        const resolved_url& url,
                        bool verify,
                        std::vector<string>& h) : context(context),url(url), headers(h) {
         verify_cert = verify;
      }

      connection_param( const http_context& context,
                        const parsed_url& url,
                        bool verify,
                        std::vector<string>& h) : context(context),url(resolve_url(context, url)), headers(h) {
         verify_cert = verify;
      }
   };

   fc::variant do_http_call(
                             const connection_param& cp,
                             const fc::variant& postdata = fc::variant(),
                             bool print_request = false,
                             bool print_response = false);

   const string chain_func_base = "/v1/chain";
   const string get_info_func = chain_func_base + "/get_info";
   const string push_txn_func = chain_func_base + "/push_transaction";
   const string push_txns_func = chain_func_base + "/push_transactions";
   const string json_to_bin_func = chain_func_base + "/abi_json_to_bin";
   const string get_block_func = chain_func_base + "/get_block";
   const string get_block_header_state_func = chain_func_base + "/get_block_header_state";
   const string get_account_func = chain_func_base + "/get_account";
   const string get_table_func = chain_func_base + "/get_table_rows";
   const string get_table_by_scope_func = chain_func_base + "/get_table_by_scope";
   const string get_code_func = chain_func_base + "/get_code";
   const string get_abi_func = chain_func_base + "/get_abi";
   const string get_raw_code_and_abi_func = chain_func_base + "/get_raw_code_and_abi";
   const string get_currency_balance_func = chain_func_base + "/get_currency_balance";
   const string get_currency_stats_func = chain_func_base + "/get_currency_stats";
   const string get_producers_func = chain_func_base + "/get_producers";
   const string get_schedule_func = chain_func_base + "/get_producer_schedule";
   const string get_required_keys = chain_func_base + "/get_required_keys";


   const string history_func_base = "/v1/history";
   const string get_actions_func = history_func_base + "/get_actions";
   const string get_transaction_func = history_func_base + "/get_transaction";
   const string get_key_accounts_func = history_func_base + "/get_key_accounts";
   const string get_controlled_accounts_func = history_func_base + "/get_controlled_accounts";

   const string net_func_base = "/v1/net";
   const string net_connect = net_func_base + "/connect";
   const string net_disconnect = net_func_base + "/disconnect";
   const string net_status = net_func_base + "/status";
   const string net_connections = net_func_base + "/connections";


   const string wallet_func_base = "/v1/wallet";
   const string wallet_create = wallet_func_base + "/create";
   const string wallet_open = wallet_func_base + "/open";
   const string wallet_list = wallet_func_base + "/list_wallets";
   const string wallet_list_keys = wallet_func_base + "/list_keys";
   const string wallet_public_keys = wallet_func_base + "/get_public_keys";
   const string wallet_lock = wallet_func_base + "/lock";
   const string wallet_lock_all = wallet_func_base + "/lock_all";
   const string wallet_unlock = wallet_func_base + "/unlock";
   const string wallet_import_key = wallet_func_base + "/import_key";
   const string wallet_remove_key = wallet_func_base + "/remove_key";
   const string wallet_create_key = wallet_func_base + "/create_key";
   const string wallet_sign_trx = wallet_func_base + "/sign_transaction";
   const string keosd_stop = "/v1/keosd/stop";

   FC_DECLARE_EXCEPTION( connection_exception, 1100000, "Connection Exception" );
 }}}
