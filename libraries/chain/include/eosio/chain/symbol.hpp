/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <fc/exception/exception.hpp>
#include <eosio/chain/types.hpp>
#include <string>

namespace eosio {
   namespace chain {
      
      static constexpr uint64_t string_to_symbol_c( uint8_t precision, const char* str ) {
         uint32_t len = 0;
         while( str[len] ) ++len;
         
         uint64_t result = 0;
         for( uint32_t i = 0; i < len; ++i ) {
            result |= (uint64_t(str[i]) << (8*(1+i)));
         }

         result |= uint64_t(precision);
         return result;
      }
      
#define SY(P,X) ::eosio::chain::string_to_symbol_c(P,#X)

      static uint64_t string_to_symbol( uint8_t precision, const char* str ) {
         uint32_t len = 0;
         while( str[len] ) ++len;
         
         uint64_t result = 0;
         for( uint32_t i = 0; i < len; ++i ) {
            //            assert(str[i] >= 'A' && str[i] <= 'Z');
            result |= (uint64_t(str[i]) << (8*(i+1)));
         }
         
         result |= uint64_t(precision);
         return result;
      }

      class symbol {
         public:
            symbol(uint8_t p, const char* s): m_value(string_to_symbol(p, s)) { }
            symbol(uint64_t v = SY(4, EOS)): m_value(v) { }
            static symbol from_string(const string& from)
            {
               try {
                  string s = fc::trim(from);
                  if (s.empty()) {
                     return SY(4, EOS);
                  }
                  auto comma_pos = s.find(',');
                  if (comma_pos == string::npos) {
                     return string_to_symbol(4, s.c_str());
                  }
                  if (comma_pos == 0) {
                     return string_to_symbol(4, s.substr(1).c_str());
                  }
                  auto prec_part = s.substr(0, comma_pos);
                  uint8_t p = fc::to_int64(prec_part);
                  string name_part = s.substr(comma_pos + 1);
                  return string_to_symbol(p, name_part.c_str());
               } FC_CAPTURE_LOG_AND_RETHROW((from)) 
            }
            uint64_t value() const { return m_value; }
            operator uint64_t() const { return m_value; }
            bool valid() const
            { 
               const auto& s = name();
               return all_of(s.begin(), s.end(), [](char c)->bool { return (c >= 'A' && c <= 'Z'); });
            }
            uint8_t decimals() const { return m_value & 0xFF; }
            uint64_t precision() const
            {
               static int64_t table[] = {
                  1, 10, 100, 1000, 10000,
                  100000, 1000000, 10000000, 100000000ll,
                  1000000000ll, 10000000000ll,
                  100000000000ll, 1000000000000ll,
                  10000000000000ll, 100000000000000ll
               };
               return table[ decimals() ];
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
            
         private:
            uint64_t m_value;
            friend struct fc::reflector<symbol>;
      };

      inline bool operator== (const symbol& lhs, const symbol& rhs)
      {
         return lhs.value() == rhs.value();
      }
      inline bool operator< (const symbol& lhs, const symbol& rhs)
      {
         return lhs.value() < rhs.value();
      }
      inline bool operator> (const symbol& lhs, const symbol& rhs)
      {
         return lhs.value() > rhs.value();
      }

   } // namespace chain   
} // namespace eosio

namespace fc {
   inline void to_variant(const eosio::chain::symbol& var, fc::variant& vo) { vo = var.to_string(); }
   inline void from_variant(const fc::variant& var, eosio::chain::symbol& vo) {
      vo = eosio::chain::symbol::from_string(var.get_string());
   }
}

FC_REFLECT(eosio::chain::symbol, (m_value))
