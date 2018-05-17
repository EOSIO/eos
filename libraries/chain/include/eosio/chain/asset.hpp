/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/symbol.hpp>

/// eos with 4 digits of precision
#define EOS_SYMBOL_VALUE  (int64_t(4) | (uint64_t('E') << 8) | (uint64_t('O') << 16) | (uint64_t('S') << 24))
static const eosio::chain::symbol EOS_SYMBOL(EOS_SYMBOL_VALUE);

/// Defined to be largest power of 10 that fits in 53 bits of precision
#define EOS_MAX_SHARE_SUPPLY   int64_t(1'000'000'000'000'000ll)

namespace eosio { namespace chain {

/**

asset includes amount and currency symbol

asset::from_string takes a string of the form "10.0000 CUR" and constructs an asset 
with amount = 10 and symbol(4,"CUR")

*/

struct asset
{
   static constexpr int64_t max_amount = (1LL << 62) - 1;

   explicit asset(share_type a = 0, symbol id = EOS_SYMBOL) :amount(a), sym(id) {
      EOS_ASSERT( is_amount_within_range(), asset_type_exception, "magnitude of asset amount must be less than 2^62" );
      EOS_ASSERT( sym.valid(), asset_type_exception, "invalid symbol" );
   }

   share_type amount;
   symbol     sym;

   bool is_amount_within_range()const { return -max_amount <= amount && amount <= max_amount; }
   bool is_valid()const               { return is_amount_within_range() && sym.valid(); }

   double to_real()const { return static_cast<double>(amount) / precision(); }

   uint8_t     decimals()const;
   string      symbol_name()const;
   int64_t     precision()const;
   const symbol& get_symbol() const { return sym; }

   static asset from_string(const string& from);
   string       to_string()const;

   asset& operator += (const asset& o)
   {
      FC_ASSERT(get_symbol() == o.get_symbol());
      amount += o.amount;
      return *this;
   }

   asset& operator -= (const asset& o)
   {
      FC_ASSERT(get_symbol() == o.get_symbol());
      amount -= o.amount;
      return *this;
   }
   asset operator -()const { return asset(-amount, get_symbol()); }

   friend bool operator == (const asset& a, const asset& b)
   {
      return std::tie(a.get_symbol(), a.amount) == std::tie(b.get_symbol(), b.amount);
   }
   friend bool operator < (const asset& a, const asset& b)
   {
      FC_ASSERT(a.get_symbol() == b.get_symbol());
      return std::tie(a.amount,a.get_symbol()) < std::tie(b.amount,b.get_symbol());
   }
   friend bool operator <= (const asset& a, const asset& b) { return (a == b) || (a < b); }
   friend bool operator != (const asset& a, const asset& b) { return !(a == b); }
   friend bool operator > (const asset& a, const asset& b)  { return !(a <= b); }
   friend bool operator >= (const asset& a, const asset& b) { return !(a < b);  }

   friend asset operator - (const asset& a, const asset& b) {
      FC_ASSERT(a.get_symbol() == b.get_symbol());
      return asset(a.amount - b.amount, a.get_symbol());
   }

   friend asset operator + (const asset& a, const asset& b) {
      FC_ASSERT(a.get_symbol() == b.get_symbol());
      return asset(a.amount + b.amount, a.get_symbol());
   }

   friend std::ostream& operator << (std::ostream& out, const asset& a) { return out << a.to_string(); }

};

struct extended_asset  {
  extended_asset(){}
  extended_asset( asset a, name n ):quantity(a),contract(n){}
  asset quantity;
  name contract;
};

bool  operator <  (const asset& a, const asset& b);
bool  operator <= (const asset& a, const asset& b);

}} // namespace eosio::chain

namespace fc {
inline void to_variant(const eosio::chain::asset& var, fc::variant& vo) { vo = var.to_string(); }
inline void from_variant(const fc::variant& var, eosio::chain::asset& vo) {
   vo = eosio::chain::asset::from_string(var.get_string());
}
}

FC_REFLECT(eosio::chain::asset, (amount)(sym))
FC_REFLECT(eosio::chain::extended_asset, (quantity)(contract) )
