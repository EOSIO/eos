/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "help_text.hpp"
#include "localize.hpp"
#include <regex>
#include <fc/io/json.hpp>
#include <eosio/chain/exceptions.hpp>

using namespace eosio::client::localize;
using namespace eosio::chain;

const char* transaction_help_text_header = _("An error occurred while submitting the transaction for this command!");

const char* duplicate_transaction_help_text = _(R"text(The transaction is a duplicate of one already pushed to the producers.  If this
is an intentionally repeated transaction there are a few ways to resolve the
issue:
  - wait for the next block
  - combine duplicate transactions into a single transaction
  - adjust the expiration time using the `--expiration <milliseconds>` option
  - use the `--force-unique` option to add additional nonce data
    Please note, this will consume more bandwidth than the base transaction )text");

const char* missing_perms_help_text = _(R"text(The transaction requires permissions that were not granted by the transaction.
Missing permission from:
  - ${1}

Please use the `-p,--permissions` option to add the missing accounts!
Note: you will need an unlocked wallet that can authorized these permissions.)text");

const char* missing_sigs_help_text = _(R"text(The transaction requires permissions that could not be authorized by the wallet.
Missing authrizations:
  - ${1}@${2}

Please make sure the proper keys are imported into an unlocked wallet and try again!)text");

const char* missing_scope_help_text = _(R"text(The transaction requires scopes that were not listed by the transaction.
Missing scope(s):
  - ${1}

Please use the `-S,--scope` option to add the missing accounts!)text");


const char* tx_unknown_account_help_text = _("The transaction references an account which does not exist.");

const char* unknown_account_help_text = _(R"text(Unknown accounts:
  - ${1}

Please check the account names and try again!)text");

const char* missing_abi_help_text = _(R"text(The ABI for action "${2}" on code account "${1}" is unknown.
The payload cannot be automatically serialized.

You can push an arbitrary transaction using the 'push action' subcommand)text");

const char* unknown_wallet_help_text = _("Unable to find a wallet named \"${1}\", are you sure you typed the name correctly?");

const char* bad_wallet_password_help_text = _("Invalid password for wallet named \"${1}\"");

const char* locked_wallet_help_text = _("The wallet named \"${1}\" is locked.  Please unlock it and try again.");

const char* duplicate_key_import_help_text = _("This key is already imported into the wallet named \"${1}\".");

const char* unknown_abi_table_help_text = _(R"text(The ABI for the code on account "${1}" does not specify table "${2}".

Please check the account and table name, and verify that the account has the expected code using:
  cleos get code ${1})text");

const char* help_regex_error = _("Error locating help text: ${code} ${what}");

const std::vector<std::pair<const char*, std::vector<const char *>>> error_help_text {
   {"Error\n: 3030011", {transaction_help_text_header, duplicate_transaction_help_text}},
   {"Error\n: 3030001[^\\x00]*\\{\"acct\":\"([^\"]*)\"\\}", {transaction_help_text_header, missing_perms_help_text}},
   {"Error\n: 3030002[^\\x00]*Transaction declares authority.*account\":\"([^\"]*)\",\"permission\":\"([^\"]*)\"", {transaction_help_text_header, missing_sigs_help_text}},
   {"Error\n: 3030008[^\\x00]*\\{\"scope\":\"([^\"]*)\"\\}", {transaction_help_text_header, missing_scope_help_text}},
   {"Account not found: ([\\S]*)", {transaction_help_text_header, tx_unknown_account_help_text, unknown_account_help_text}},
   {"Error\n: 303", {transaction_help_text_header}},
   {"unknown key[^\\x00]*abi_json_to_bin.*code\":\"([^\"]*)\".*action\":\"([^\"]*)\"", {missing_abi_help_text}},
   {"unknown key[^\\x00]*chain/get_code.*name\":\"([^\"]*)\"", {unknown_account_help_text}},
   {"Unable to open file[^\\x00]*wallet/open.*postdata\":\"([^\"]*)\"", {unknown_wallet_help_text}},
   {"AES error[^\\x00]*wallet/unlock.*postdata\":\\[\"([^\"]*)\"", {bad_wallet_password_help_text}},
   {"Wallet is locked: ([\\S]*)", {locked_wallet_help_text}},
   {"Key already in wallet[^\\x00]*wallet/import_key.*postdata\":\\[\"([^\"]*)\"", {duplicate_key_import_help_text}},
   {"ABI does not define table[^\\x00]*get_table_rows.*code\":\"([^\"]*)\",\"table\":\"([^\"]*)\"", {unknown_abi_table_help_text}}
};

auto smatch_to_variant(const std::smatch& smatch) {
   auto result = fc::mutable_variant_object();
   for(size_t index = 0; index < smatch.size(); index++) {
      auto name = boost::lexical_cast<std::string>(index);
      if (smatch[index].matched) {
         result = result(name, smatch.str(index));
      } else {
         result = result(name, "");
      }
   }

   return result;
};

const char* error_advice_name_type_exception = R"=====(Name should be less than 13 characters and only contains the following symbol .12345abcdefghijklmnopqrstuvwxyz)=====";
const char* error_advice_public_key_type_exception = R"=====(Public key should be encoded in base58 and starts with EOS prefix)=====";
const char* error_advice_private_key_type_exception = R"=====(Private key should be encoded in base58 WIF)=====";
const char* error_advice_authority_type_exception = R"=====(Ensure that your authority JSON is valid follows the following format!
{
  "threshold":      <INTEGER [1-2^32): the threshold that must be met to satisfy this authority>,
  "keys": [         <keys must be alpha-numerically sorted by their string representations and unique>
    ...
    {
      "key":        <STRING: EOS.IO compatible Public Key>,
      "weight":     <INTEGER [1-2^16): a signature from this key contributes this to satisfying the threshold>
    }
    ...
  ],
  "accounts": [     <accounts must be alpha-numerically sorted by their permission (actor, then permission) and unique>
    ...
    {
      "permission": {
        "actor":      <STRING: account name of the delegated signer>,
        "permission": <STRING: permission level on the account that must be satisfied>,
      },
      "weight":     <INTEGER [1-2^16): satisfying the delegation contributes this to satisfying the threshold>
    }
    ...
  ],
  "waits": [        <waits must be sorted by wait_sec, largest first, and be unique>
    ...
    {
      "wait_sec":   <INTEGER [1-2^32): seconds of delay which qualifies as passing this wait>
      "weight":     <INTEGER [1-2^16): satisfying the delay contributes this to satisfying the threshold>
    }
    ...
  ]
}
)=====";
const char* error_advice_action_type_exception = R"=====(Ensure that your action JSON follows the contract's abi!)=====";
const char* error_advice_transaction_type_exception = R"=====(Ensure that your transaction JSON follows the right transaction format!
You can refer to contracts/eosiolib/transaction.hpp for reference)=====";
const char* error_advice_abi_type_exception =  R"=====(Ensure that your abi JSON follows the following format!
{
  "types" : [{ "new_type_name":"type_name", "type":"type_name" }],
  "structs" : [{ "name":"type_name", "base":"type_name", "fields": [{ "name":"field_name", "type": "type_name" }] }],
  "actions" : [{ "name":"action_name","type":"type_name"}],
  "tables" : [{
    "name":"table_name",
    "index_type":"type_name",
    "key_names":[ "field_name" ],
    "key_types":[ "type_name" ],
    "type":"type_name" "
  }],
  "ricardian_clauses": [{ "id": "string", "body": "string" }]
}
e.g.
{
  "types" : [{ "new_type_name":"account_name", "type":"name" }],
  "structs" : [
    { "name":"foo", "base":"", "fields": [{ "name":"by", "type": "account_name" }] },
    { "name":"foobar", "base":"", "fields": [{ "name":"by", "type": "account_name" }] }
  ],
  "actions" : [{ "name":"foo","type":"foo"}],
  "tables" : [{
    "name":"foobar_table",
    "index_type":"i64",
    "key_names":[ "by" ],
    "key_types":[ "account_name" ],
    "type":"foobar"
  }],
  "ricardian_clauses": [{ "id": "foo", "body": "bar" }]
})=====";
const char* error_advice_block_id_type_exception =  "Ensure that the block ID is a SHA-256 hexadecimal string!";
const char* error_advice_transaction_id_type_exception =  "Ensure that the transaction ID is a SHA-256 hexadecimal string!";
const char* error_advice_packed_transaction_type_exception =  R"=====(Ensure that your packed transaction JSON follows the following format!
{
  "signatures" : [ "signature" ],
  "compression" : enum("none", "zlib"),
  "packed_context_free_data" : "bytes",
  "packed_trx" : "bytes";
}
e.g.
{
  "signatures" : [ "SIG_K1_Jze4m1ZHQ4UjuHpBcX6uHPN4Xyggv52raQMTBZJghzDLepaPcSGCNYTxaP2NiaF4yRF5RaYwqsQYAwBwFtfuTJr34Z5GJX" ],
  "compression" : "none",
  "packed_context_free_data" : "6c36a25a00002602626c5e7f0000000000010000001e4d75af460000000000a53176010000000000ea305500000000a8ed3232180000001e4d75af4680969800000000000443555200000000",
  "packed_trx" : "6c36a25a00002602626c5e7f0000000000010000001e4d75af460000000000a53176010000000000ea305500000000a8ed3232180000001e4d75af4680969800000000000443555200000000"
})=====";

const char* error_advice_transaction_exception =  "Ensure that your transaction satisfy the contract's constraint!";
const char* error_advice_expired_tx_exception =  "Please increase the expiration time of your transaction!";
const char* error_advice_tx_exp_too_far_exception =  "Please decrease the expiration time of your transaction!";
const char* error_advice_invalid_ref_block_exception =  "Ensure that the reference block exist in the blockchain!";
const char* error_advice_tx_duplicate =  "You can try embedding eosio nonce action inside your transaction to ensure uniqueness.";

const char* error_advice_invalid_action_args_exception = R"=====(Ensure that your arguments follow the contract abi!
You can check the contract's abi by using 'cleos get code' command.)=====";

const char* error_advice_permission_query_exception =  "Most likely, the given account/ permission doesn't exist in the blockchain.";
const char* error_advice_account_query_exception =  "Most likely, the given account doesn't exist in the blockchain.";
const char* error_advice_contract_table_query_exception =  "Most likely, the given table doesnt' exist in the blockchain.";
const char* error_advice_contract_query_exception =  "Most likely, the given contract doesnt' exist in the blockchain.";

const char* error_advice_tx_irrelevant_sig =  "Please remove the unnecessary signature from your transaction!";
const char* error_advice_unsatisfied_authorization =  "Ensure that you have the related private keys inside your wallet and your wallet is unlocked.";
const char* error_advice_missing_auth_exception =  R"=====(Ensure that you have the related authority inside your transaction!;
If you are currently using 'cleos push action' command, try to add the relevant authority using -p option.)=====";
const char* error_advice_irrelevant_auth_exception =  "Please remove the unnecessary authority from your action!";

const char* error_advice_missing_chain_api_plugin_exception =  "Ensure that you have \033[2meosio::chain_api_plugin\033[0m\033[32m added to your node's configuration!";
const char* error_advice_missing_wallet_api_plugin_exception =  "Ensure that you have \033[2meosio::wallet_api_plugin\033[0m\033[32m added to your node's configuration!\n"\
                                    "Otherwise specify your wallet location with \033[2m--wallet-url\033[0m\033[32m argument!";
const char* error_advice_missing_history_api_plugin_exception =  "Ensure that you have \033[2meosio::history_api_plugin\033[0m\033[32m added to your node's configuration!";
const char* error_advice_missing_net_api_plugin_exception =  "Ensure that you have \033[2meosio::net_api_plugin\033[0m\033[32m added to your node's configuration!";

const char* error_advice_wallet_exist_exception =  "Try to use different wallet name.";
const char* error_advice_wallet_nonexistent_exception =  "Are you sure you typed the wallet name correctly?";
const char* error_advice_wallet_locked_exception =  "Ensure that your wallet is unlocked before using it!";
const char* error_advice_wallet_missing_pub_key_exception =  "Ensure that you have the relevant private key imported!";
const char* error_advice_wallet_invalid_password_exception =  "Are you sure you are using the right password?";
const char* error_advice_wallet_not_available_exception =  "Ensure that you have created a wallet and have it open";

const std::map<int64_t, std::string> error_advice = {
   { name_type_exception::code_value, error_advice_name_type_exception },
   { public_key_type_exception::code_value, error_advice_public_key_type_exception },
   { private_key_type_exception::code_value, error_advice_private_key_type_exception },
   { authority_type_exception::code_value, error_advice_authority_type_exception },
   { action_type_exception::code_value, error_advice_action_type_exception },
   { transaction_type_exception::code_value, error_advice_transaction_type_exception },
   { abi_type_exception::code_value, error_advice_abi_type_exception },
   { block_id_type_exception::code_value, error_advice_block_id_type_exception },
   { transaction_id_type_exception::code_value, error_advice_transaction_id_type_exception },
   { packed_transaction_type_exception::code_value, error_advice_packed_transaction_type_exception },

   { transaction_exception::code_value, error_advice_transaction_exception },
   { expired_tx_exception::code_value, error_advice_expired_tx_exception },
   { tx_exp_too_far_exception::code_value, error_advice_tx_exp_too_far_exception },
   { invalid_ref_block_exception::code_value, error_advice_invalid_ref_block_exception },
   { tx_duplicate::code_value, error_advice_tx_duplicate },

   { invalid_action_args_exception::code_value, error_advice_invalid_action_args_exception },

   { permission_query_exception::code_value, error_advice_permission_query_exception },
   { account_query_exception::code_value, error_advice_account_query_exception },
   { contract_table_query_exception::code_value, error_advice_contract_table_query_exception },
   { contract_query_exception::code_value, error_advice_contract_query_exception },

   { tx_irrelevant_sig::code_value, error_advice_tx_irrelevant_sig },
   { unsatisfied_authorization::code_value, error_advice_unsatisfied_authorization },
   { missing_auth_exception::code_value, error_advice_missing_auth_exception },
   { irrelevant_auth_exception::code_value, error_advice_irrelevant_auth_exception },

   { missing_chain_api_plugin_exception::code_value, error_advice_missing_chain_api_plugin_exception },
   { missing_wallet_api_plugin_exception::code_value, error_advice_missing_wallet_api_plugin_exception },
   { missing_history_api_plugin_exception::code_value, error_advice_missing_history_api_plugin_exception },
   { missing_net_api_plugin_exception::code_value, error_advice_missing_net_api_plugin_exception },

   { wallet_exist_exception::code_value, error_advice_wallet_exist_exception },
   { wallet_nonexistent_exception::code_value, error_advice_wallet_nonexistent_exception },
   { wallet_locked_exception::code_value, error_advice_wallet_locked_exception },
   { wallet_missing_pub_key_exception::code_value, error_advice_wallet_missing_pub_key_exception },
   { wallet_invalid_password_exception::code_value, error_advice_wallet_invalid_password_exception },
   { wallet_not_available_exception::code_value, error_advice_wallet_not_available_exception }
};

namespace eosio { namespace client { namespace help {

bool print_recognized_errors(const fc::exception& e, const bool verbose_errors) {
   // eos recognized error code is from 3000000
   // refer to libraries/chain/include/eosio/chain/exceptions.hpp
   if (e.code() >= chain_exception::code_value) {
      std::string advice, explanation, stack_trace;

      // Get advice, if any
      const auto advice_itr = error_advice.find(e.code());
      if (advice_itr != error_advice.end()) advice = advice_itr->second;

      // Get explanation from log, if any
      for (auto &log : e.get_log()) {
         // Check if there's a log to display
         if (!log.get_format().empty()) {
            // Localize the message as needed
            explanation += "\n" + localized_with_variant(log.get_format().data(), log.get_data());
         } else if (log.get_data().size() > 0 && verbose_errors) {
            // Show data-only log only if verbose_errors option is enabled
            explanation += "\n" + fc::json::to_string(log.get_data());
         }
         // Check if there's stack trace to be added
         if (!log.get_context().get_method().empty() && verbose_errors) {
            stack_trace += "\n" +
                           log.get_context().get_file() +  ":" +
                           fc::to_string(log.get_context().get_line_number())  + " " +
                           log.get_context().get_method();
         }
      }
      // Append header
      if (!explanation.empty()) explanation = std::string("Error Details:") + explanation;
      if (!stack_trace.empty()) stack_trace = std::string("Stack Trace:") + stack_trace;

      std::cerr << "\033[31m" << "Error " << e.code() << ": " << e.what() << "\033[0m";
      if (!advice.empty()) std::cerr << "\n" << "\033[32m" << advice << "\033[0m";
      if (!explanation.empty()) std::cerr  << "\n" << "\033[33m" << explanation << "\033[0m";
      if (!stack_trace.empty()) std::cerr  << "\n" << stack_trace;
      std::cerr << std::endl;
      return true;
   }
   return false;
}

bool print_help_text(const fc::exception& e) {
   bool result = false;
   // Large input strings to std::regex can cause SIGSEGV, this is a known bug in libstdc++.
   // See https://stackoverflow.com/questions/36304204/%D0%A1-regex-segfault-on-long-sequences
   auto detail_str = e.to_detail_string();
   // 2048 nice round number. Picked for no particular reason. Bug above was reported for 22K+ strings.
   if (detail_str.size() > 2048) return result;
   try {
      for (const auto& candidate : error_help_text) {
         auto expr = std::regex {candidate.first};
         std::smatch matches;
         if (std::regex_search(detail_str, matches, expr)) {
            auto args = smatch_to_variant(matches);
            for (const auto& msg: candidate.second) {
               std::cerr << localized_with_variant(msg, args) << std::endl;
            }
            result = true;
            break;
         }
      }
   } catch (const std::regex_error& e ) {
      std::cerr << localized(help_regex_error, ("code", (int64_t)e.code())("what", e.what())) << std::endl;
   }

   return result;
}

}}}
