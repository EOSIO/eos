#include <cyberway/chain/domain_name.hpp>
#include <boost/algorithm/string.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/algorithm/string.hpp>

namespace cyberway { namespace chain {

using eosio::chain::domain_name_type_exception;
using eosio::chain::username_type_exception;

using std::string;

//
inline bool is_digit(char c) {
   return c >= '0' && c <= '9';
}
inline bool is_letter(char c) {
   return c >= 'a' && c <= 'z';
}
inline bool is_allowed_punct(char c) {
   return c == '-' || c == '.';
}
inline bool is_valid_char(char c) {
   return is_letter(c) || is_digit(c) || is_allowed_punct(c);
}

enum name_error {
   valid = 0,
   name_empty,
   name_too_long,
   part_too_short,
   part_too_long,
   bad_part_edge,
   bad_char,
   all_numeric
};

// name consists of 1 or more parts delimited by dots
name_error validate_part(const string& n, size_t min_size, size_t max_size) {
   if (n.size() < min_size) return part_too_short;
   if (n.size() > max_size) return part_too_long;
   if (n[0] == '-' || n.back() == '-') return bad_part_edge; // Domain labels may not start or end with a hyphen
   // if (!std::all_of(n.begin(), n.end(), is_valid_char)) return bad_char;   // checked outside
   return valid;
}

name_error validate_domain_name(const string& d, size_t max_size, size_t min_part, size_t max_part, bool strict_tld) {
   if (!d.size()) return name_empty;
   if (d.size() > max_size) return name_too_long;
   if (!std::all_of(d.begin(), d.end(), is_valid_char)) return bad_char;
   std::vector<std::string> parts;
   boost::split(parts, d, boost::is_any_of("."));
   for (const auto& part: parts) {
      auto r = validate_part(part, min_part, max_part);
      if (r != valid) return r;
   }
   // Top-level domain names should not be all-numeric
   if (strict_tld) {
      auto tld = parts.back();
      if (std::all_of(tld.begin(), tld.end(), is_digit)) return all_numeric;
   }
   return valid;
}

void validate_domain_name(const domain_name& n) {
   auto r = validate_domain_name(n, domain_max_size, domain_min_part_size, domain_max_part_size, true);
   static const std::vector<string> err = {
      "",
      "Domain name is empty",
      "Domain name is too long",
      "Domain label is too short",
      "Domain label is too long",
      "Domain label can't start or end with '-'",
      "Domain name contains bad symbol",
      "Top-level name is all-numeric"
   };
   EOS_ASSERT(r == valid, domain_name_type_exception, err[r]);
}

void validate_username(const username& n) {
   auto r = validate_domain_name(n, username_max_size, username_min_part_size, username_max_part_size, false);
   static const std::vector<string> err = {
      "",
      "Username is empty",
      "Username is too long",
      "Username part is too short",
      "Username part is too long",
      "Username part can't start or end with '-'",
      "Username contains bad symbol"
   };
   EOS_ASSERT(r == valid, username_type_exception, err[r]);
}


} } /// cyberway::chain
