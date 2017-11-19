/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "help_text.hpp"
#include "localize.hpp"
#include <regex>

using namespace eosio::client::localize;

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

You can push an arbitrary transaction using the 'push transaction' subcommand)text");

const char* unknown_wallet_help_text = _("Unable to find a wallet named \"${1}\", are you sure you typed the name correctly?");

const char* bad_wallet_password_help_text = _("Invalid password for wallet named \"${1}\"");

const char* locked_wallet_help_text = _("The wallet named \"${1}\" is locked.  Please unlock it and try again.");

const char* duplicate_key_import_help_text = _("This key is already imported into the wallet named \"${1}\".");

const char* unknown_abi_table_help_text = _(R"text(The ABI for the code on account "${1}" does not specify table "${2}".

Please check the account and table name, and verify that the account has the expected code using:
  eosc get code ${1})text");

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

namespace eosio { namespace client { namespace help {

bool print_help_text(const fc::exception& e) {
   bool result = false;
   auto detail_str = e.to_detail_string();
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
      std::cerr << localized(help_regex_error, ("code", e.code())("what", e.what())) << std::endl;
   }

   return result;   
}

}}}