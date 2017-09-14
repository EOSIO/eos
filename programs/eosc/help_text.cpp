/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "help_text.hpp"
#include <regex>
#include <fc/variant.hpp>

const char* transaction_help_text_header = 1 + R"text(
An error occurred while submitting the transaction for this command!
)text";

const char* duplicate_transaction_help_text = 1 + R"text(
The transaction is a duplicate of one already pushed to the producers.  If this
is an intentionally repeated transaction there are a few ways to resolve the
issue:
  - wait for the next block
  - combine duplicate transactions into a single transaction
  - adjust the expiration time using the `--expiration <milliseconds>` option
  - use the `--force-unique` option to add additional nonce data
    Please note, this will consume more bandwidth than the base transaction 
)text";

const char* missing_perms_help_text = 1 + R"text(
The transaction requires permissions that were not granted by the transaction.
Missing permission from:
  - ${1}

Please use the `-p,--permissions` option to add the missing accounts!
Note: you will need an unlocked wallet that can authorized these permissions.  
)text";

const char* missing_sigs_help_text = 1 + R"text(
The transaction requires permissions that could not be authorized by the wallet.
Missing authrizations:
  - ${1}@${2}

Please make sure the proper keys are imported into an unlocked wallet and try again!    
)text";

const char* missing_scope_help_text = 1 + R"text(
The transaction requires scopes that were not listed by the transaction.
Missing scope(s):
  - ${1}

Please use the `-S,--scope` option to add the missing accounts!
)text";


const char* tx_unknown_account_help_text = 1 + R"text(
The transaction references an account which does not exist.
)text";

const char* unknown_account_help_text = 1 + R"text(
Unknown accounts:
  - ${1}

Please check the account names and try again!    
)text";

const char* missing_abi_help_text = 1 + R"text(
The ABI for action "${2}" on code account "${1}" is unknown.
The payload cannot be automatically serialized.

You can push an arbitrary transaction using the 'push transaction' subcommand
)text";

const char* unknown_wallet_help_text = 1 + R"text(
Unable to find a wallet named "${1}", are you sure you typed the name correctly?
)text";

const char* bad_wallet_password_help_text = 1 + R"text(

Invalid password for wallet named "${1}"
)text";

const char* locked_wallet_help_text = 1 + R"text(
The wallet named "${1}" is locked.  Please unlock it and try again.
)text";

const char* duplicate_key_import_help_text = 1 + R"text(
This key is already imported into the wallet named "${1}".
)text";

const char* unknown_abi_table_help_text = 1 + R"text(
The ABI for the code on account "${1}" does not specify table "${2}".

Please check the account and table name, and verify that the account has the expected code using:
  eosc get code ${1}
)text";

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
   {"Abi does not define table[^\\x00]*get_table_rows.*code\":\"([^\"]*)\",\"table\":\"([^\"]*)\"", {unknown_abi_table_help_text}}
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

namespace eos { namespace client { namespace help {

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
               std::cerr << fc::format_string(msg, args) << std::endl;
            }               
            result = true;
            break;
         }
      }
   } catch (const std::regex_error& e ) {
      std::cerr << "Error locating help text: "<< e.code() << " " << e.what() << std::endl;
   }

   return result;   
}

}}}