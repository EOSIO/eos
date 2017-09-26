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
#include <boost/locale.hpp>
#include <fc/variant.hpp>

using namespace boost::locale;

const std::vector<std::pair<const char*, std::vector<const char *>>> error_help_text {
   {"Error\n: 3030011", {"transaction_help_text_header", "duplicate_transaction_help_text"}},
   {"Error\n: 3030001[^\\x00]*\\{\"acct\":\"([^\"]*)\"\\}", {"transaction_help_text_header", "missing_perms_help_text"}},
   {"Error\n: 3030002[^\\x00]*Transaction declares authority.*account\":\"([^\"]*)\",\"permission\":\"([^\"]*)\"", {"transaction_help_text_header", "missing_sigs_help_text"}},
   {"Error\n: 3030008[^\\x00]*\\{\"scope\":\"([^\"]*)\"\\}", {"transaction_help_text_header", "missing_scope_help_text"}},
   {"Account not found: ([\\S]*)", {"transaction_help_text_header", "tx_unknown_account_help_text", "unknown_account_help_text"}},
   {"Error\n: 303", {"transaction_help_text_header"}},
   {"unknown key[^\\x00]*abi_json_to_bin.*code\":\"([^\"]*)\".*action\":\"([^\"]*)\"", {"missing_abi_help_text"}},
   {"unknown key[^\\x00]*chain/get_code.*name\":\"([^\"]*)\"", {"unknown_account_help_text"}},
   {"Unable to open file[^\\x00]*wallet/open.*postdata\":\"([^\"]*)\"", {"unknown_wallet_help_text"}},
   {"AES error[^\\x00]*wallet/unlock.*postdata\":\\[\"([^\"]*)\"", {"bad_wallet_password_help_text"}},
   {"Wallet is locked: ([\\S]*)", {"locked_wallet_help_text"}},
   {"Key already in wallet[^\\x00]*wallet/import_key.*postdata\":\\[\"([^\"]*)\"", {"duplicate_key_import_help_text"}},
   {"Abi does not define table[^\\x00]*get_table_rows.*code\":\"([^\"]*)\",\"table\":\"([^\"]*)\"", {"unknown_abi_table_help_text"}}
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
               auto localized = boost::locale::gettext(msg);
               std::cerr << fc::format_string(localized, args) << std::endl;
            }               
            result = true;
            break;
         }
      }
   } catch (const std::regex_error& e ) {
      std::cerr << translate("err_locate_help_text") << e.code() << " " << translate(e.what()) << std::endl;
   }

   return result;   
}

}}}