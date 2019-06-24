/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/symbol.hpp>

namespace eosio { namespace chain {

/**

asset includes amount and currency symbol

asset::from_string takes a string of the form "10.0000 CUR" and constructs an asset 
with amount = 10 and symbol(4,"CUR")

*/

struct asset : fc::reflect_init
{
   static constexpr int64_t max_amount = (1LL << 62) - 1;

   explicit asset(share_type a = 0, symbol id = symbol(CORE_SYMBOL)) :amount(a), sym(id) {
      EOS_ASSERT( is_amount_within_range(), asset_type_exception, "magnitude of asset amount must be less than 2^62" );
      EOS_ASSERT( sym.valid(), asset_type_exception, "invalid symbol" );
   }

   bool is_amount_within_range()const { return -max_amount <= amount && amount <= max_amount; }
   bool is_valid()const               { return is_amount_within_range() && sym.valid(); }

   double to_real()const { return static_cast<double>(amount) / precision(); }

   uint8_t     decimals()const;
   string      symbol_name()const;
   int64_t     precision()const;
   const symbol& get_symbol() const { return sym; }
   share_type get_amount()const { return amount; }

   static asset from_string(const string& from);
   string       to_string()const;

   asset& operator += (const asset& o)
   {
      EOS_ASSERT(get_symbol() == o.get_symbol(), asset_type_exception, "addition between two different asset is not allowed");
      amount += o.amount;
      return *this;
   }

   asset& operator -= (const asset& o)
   {
      EOS_ASSERT(get_symbol() == o.get_symbol(), asset_type_exception, "subtraction between two different asset is not allowed");
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
      EOS_ASSERT(a.get_symbol() == b.get_symbol(), asset_type_exception, "logical operation between two different asset is not allowed");
      return std::tie(a.amount,a.get_symbol()) < std::tie(b.amount,b.get_symbol());
   }
   friend bool operator <= (const asset& a, const asset& b) { return (a == b) || (a < b); }
   friend bool operator != (const asset& a, const asset& b) { return !(a == b); }
   friend bool operator > (const asset& a, const asset& b)  { return !(a <= b); }
   friend bool operator >= (const asset& a, const asset& b) { return !(a < b);  }

   friend asset operator - (const asset& a, const asset& b) {
      EOS_ASSERT(a.get_symbol() == b.get_symbol(), asset_type_exception, "subtraction between two different asset is not allowed");
      return asset(a.amount - b.amount, a.get_symbol());
   }

   friend asset operator + (const asset& a, const asset& b) {
      EOS_ASSERT(a.get_symbol() == b.get_symbol(), asset_type_exception, "addition between two different asset is not allowed");
      return asset(a.amount + b.amount, a.get_symbol());
   }

   friend std::ostream& operator << (std::ostream& out, const asset& a) { return out << a.to_string(); }

   friend struct fc::reflector<asset>;

   void reflector_init()const {
      EOS_ASSERT( is_amount_within_range(), asset_type_exception, "magnitude of asset amount must be less than 2^62" );
      EOS_ASSERT( sym.valid(), asset_type_exception, "invalid symbol" );
   }

private:
   share_type amount;
   symbol     sym;

};

struct asset_info final {
    share_type  _amount;
    uint8_t     _decs;
    symbol_code _sym;

    asset_info() = default;

    asset_info(const asset& src)
    : _amount(src.get_amount()),
      _decs(src.get_symbol().decimals()),
      _sym(src.get_symbol().to_symbol_code())
    { }

    operator asset() const {
       return asset(_amount, symbol(_sym.value << 8 | _decs));
    }

    static asset_info from_string(const string& from) {
        return asset_info(asset::from_string(from));
    }
};

bool  operator <  (const asset& a, const asset& b);
bool  operator <= (const asset& a, const asset& b);

}} // namespace eosio::chain

FC_REFLECT(eosio::chain::asset_info, (_amount)(_decs)(_sym))
FC_REFLECT(eosio::chain::asset, (amount)(sym))

namespace fc {
   namespace raw {
      template<typename Stream> void pack(Stream& s, const eosio::chain::asset_info& vo) {
         pack(s, static_cast<eosio::chain::asset>(vo));
      }

      template<typename Stream> void unpack(Stream& s, eosio::chain::asset_info& vo) {
         eosio::chain::asset temp;
         unpack(s, temp);
         vo = eosio::chain::asset_info(temp);
      }
   } // namespace raw

   inline void from_variant(const variant& var, eosio::chain::asset_info& vo) {
      using namespace eosio::chain;
      if (var.get_type() == variant::type_id::string_type) {
         vo = asset::from_string(var.get_string());
      } else {
         reflector<asset_info>::visit(from_variant_visitor<asset_info>(var.get_object(), vo));
      }
   }
   inline void to_variant(const eosio::chain::asset& var, variant& vo) {
      vo = variant(var.to_string());
   }
   inline void from_variant(const fc::variant& var, eosio::chain::asset& vo) {
      eosio::chain::asset_info info;
      from_variant(var, info);
      vo = info;
   }
} //namespace fc
