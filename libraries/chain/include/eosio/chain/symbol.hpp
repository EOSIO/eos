#pragma once
#include <fc/exception/exception.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/core_symbol.hpp>
#include <string>
#include <functional>

namespace eosio {
   namespace chain {

      /**
         class symbol represents a token and contains precision and name.
         When encoded as a uint64_t, first byte represents the number of decimals, remaining bytes
         represent token name.
         Name must only include upper case alphabets.
         from_string constructs a symbol from an input a string of the form "4,EOS"
         where the integer represents number of decimals. Number of decimals must be larger than zero.
       */

      static constexpr uint64_t string_to_symbol_c(uint8_t precision, const char* str) {
         uint32_t len = 0;
         while (str[len]) ++len;

         uint64_t result = 0;
         // No validation is done at compile time
         for (uint32_t i = 0; i < len; ++i) {
            result |= (uint64_t(str[i]) << (8*(1+i)));
         }

         result |= uint64_t(precision);
         return result;
      }

#define SY(P,X) ::eosio::chain::string_to_symbol_c(P,#X)

      static uint64_t string_to_symbol(uint8_t precision, const char* str) {
         try {
            uint32_t len = 0;
            while(str[len]) ++len;
            uint64_t result = 0;
            for (uint32_t i = 0; i < len; ++i) {
               // All characters must be upper case alphabets
               EOS_ASSERT (str[i] >= 'A' && str[i] <= 'Z', symbol_type_exception, "invalid character in symbol name");
               result |= (uint64_t(str[i]) << (8*(i+1)));
            }
            result |= uint64_t(precision);
            return result;
         } FC_CAPTURE_LOG_AND_RETHROW((str))
      }

      struct symbol_code {
         uint64_t value;

         operator uint64_t()const { return value; }
      };

      class symbol : fc::reflect_init {
         public:

            static constexpr uint8_t max_precision = 18;

            explicit symbol(uint8_t p, const char* s): m_value(string_to_symbol(p, s)) {
               EOS_ASSERT(valid(), symbol_type_exception, "invalid symbol: ${s}", ("s",s));
            }
            explicit symbol(uint64_t v = CORE_SYMBOL): m_value(v) {
               EOS_ASSERT(valid(), symbol_type_exception, "invalid symbol: ${name}", ("name",name()));
            }
            static symbol from_string(const string& from)
            {
               try {
                  string s = fc::trim(from);
                  EOS_ASSERT(!s.empty(), symbol_type_exception, "creating symbol from empty string");
                  auto comma_pos = s.find(',');
                  EOS_ASSERT(comma_pos != string::npos, symbol_type_exception, "missing comma in symbol");
                  auto prec_part = s.substr(0, comma_pos);
                  uint8_t p = fc::to_int64(prec_part);
                  string name_part = s.substr(comma_pos + 1);
                  EOS_ASSERT( p <= max_precision, symbol_type_exception, "precision ${p} should be <= 18", ("p", p));
                  return symbol(string_to_symbol(p, name_part.c_str()));
               } FC_CAPTURE_LOG_AND_RETHROW((from))
            }
            uint64_t value() const { return m_value; }
            bool valid() const
            {
               const auto& s = name();
               return decimals() <= max_precision && valid_name(s);
            }
            static bool valid_name(const string& name)
            {
               return all_of(name.begin(), name.end(), [](char c)->bool { return (c >= 'A' && c <= 'Z'); });
            }

            uint8_t decimals() const { return m_value & 0xFF; }
            uint64_t precision() const
            {
               EOS_ASSERT( decimals() <= max_precision, symbol_type_exception, "precision ${p} should be <= 18", ("p", decimals()) );
               uint64_t p10 = 1;
               uint64_t p = decimals();
               while( p > 0  ) {
                  p10 *= 10; --p;
               }
               return p10;
            }
            string name() const
            {
               uint64_t v = m_value;
               v >>= 8;
               string result;
               while (v > 0) {
                  char c = v & 0xFF;
                  result += c;
                  v >>= 8;
               }
               return result;
            }

            symbol_code to_symbol_code()const { return {m_value >> 8}; }

            explicit operator string() const
            {
               uint64_t v = m_value;
               uint8_t p = v & 0xFF;
               string ret = eosio::chain::to_string(p);
               ret += ',';
               ret += name();
               return ret;
            }

            string to_string() const { return string(*this); }
            template <typename DataStream>
            friend DataStream& operator<< (DataStream& ds, const symbol& s)
            {
               return ds << s.to_string();
            }

            void reflector_init()const {
               EOS_ASSERT( decimals() <= max_precision, symbol_type_exception, "precision ${p} should be <= 18", ("p", decimals()) );
               EOS_ASSERT( valid_name(name()), symbol_type_exception, "invalid symbol: ${name}", ("name",name()));
            }

         private:
            uint64_t m_value;
            friend struct fc::reflector<symbol>;
      }; // class symbol

      struct extended_symbol {
         symbol       sym;
         account_name contract;
      };

      inline bool operator== (const symbol& lhs, const symbol& rhs)
      {
         return lhs.value() == rhs.value();
      }
      inline bool operator!= (const symbol& lhs, const symbol& rhs)
      {
         return lhs.value() != rhs.value();
      }
      inline bool operator< (const symbol& lhs, const symbol& rhs)
      {
         return lhs.value() < rhs.value();
      }
      inline bool operator> (const symbol& lhs, const symbol& rhs)
      {
         return lhs.value() > rhs.value();
      }

      inline bool operator== (const extended_symbol& lhs, const extended_symbol& rhs)
      {
         return std::tie(lhs.sym, lhs.contract) == std::tie(rhs.sym, rhs.contract);
      }

      inline bool operator!= (const extended_symbol& lhs, const extended_symbol& rhs)
      {
         return std::tie(lhs.sym, lhs.contract) != std::tie(rhs.sym, rhs.contract);
      }

      inline bool operator< (const extended_symbol& lhs, const extended_symbol& rhs)
      {
         return std::tie(lhs.sym, lhs.contract) < std::tie(rhs.sym, rhs.contract);
      }

      inline bool operator> (const extended_symbol& lhs, const extended_symbol& rhs)
      {
         return std::tie(lhs.sym, lhs.contract) > std::tie(rhs.sym, rhs.contract);
      }
   } // namespace chain
} // namespace eosio

namespace fc {
   inline void to_variant(const eosio::chain::symbol& var, fc::variant& vo) { vo = var.to_string(); }
   inline void from_variant(const fc::variant& var, eosio::chain::symbol& vo) {
      vo = eosio::chain::symbol::from_string(var.get_string());
   }
}

namespace fc {
   inline void to_variant(const eosio::chain::symbol_code& var, fc::variant& vo) {
      vo = eosio::chain::symbol(var.value << 8).name();
   }
   inline void from_variant(const fc::variant& var, eosio::chain::symbol_code& vo) {
      vo = eosio::chain::symbol(0, var.get_string().c_str()).to_symbol_code();
   }
}

FC_REFLECT(eosio::chain::symbol_code, (value))
FC_REFLECT(eosio::chain::symbol, (m_value))
FC_REFLECT(eosio::chain::extended_symbol, (sym)(contract))
