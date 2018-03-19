#pragma once
#include <eosiolib/serialize.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/system.h>
#include <eosiolib/safe_number.hpp>
#include <tuple>

namespace eosio {

   static constexpr uint64_t string_to_symbol( uint8_t precision, const char* str ) {
      uint32_t len = 0;
      while( str[len] ) ++len;

      uint64_t result = 0;
      for( uint32_t i = 0; i < len; ++i ) {
         if( str[i] < 'A' || str[i] > 'Z' ) {
            /// ERRORS?
         } else {
            result |= (uint64_t(str[i]) << (8*(1+i)));
         }
      }

      result |= uint64_t(precision);
      return result;
   }

   #define S(P,X) ::eosio::string_to_symbol(P,#X)

   typedef uint64_t symbol_name;

   static constexpr bool is_valid_symbol( symbol_name sym ) {
      sym >>= 8;
      for( int i = 0; i < 7; ++i ) {
         char c = (char)(sym & 0xff);
         if( !('A' <= c && c <= 'Z')  ) return false;
         sym >>= 8;
         if( !(sym & 0xff) ) {
            do {
              sym >>= 8;
              if( (sym & 0xff) ) return false;
              ++i;
            } while( i < 7 );
         }
      }
      return true;
   }

   static constexpr uint32_t symbol_name_length( symbol_name tmp ) {
      tmp >>= 8; /// skip precision
      uint32_t length = 0;
      while( tmp & 0xff && length <= 7) {
         ++length;
         tmp >>= 8;
      }

      return length;
   }

   struct symbol_type {
      symbol_name value;

      static constexpr symbol_name default_symbol_name = S(4, EOS);

      constexpr symbol_type( symbol_name v = default_symbol_name ) : value(v) {}

      constexpr bool     is_valid()const  { return is_valid_symbol( value ); }
      constexpr uint64_t precision()const { return value & 0xff; }
      constexpr uint64_t name()const      { return value >> 8;   }
      constexpr uint32_t name_length()const { return symbol_name_length( value ); }

      constexpr operator symbol_name()const { return value; }
      /// Sorting by the converted to symbol_name (aka uint64_t) is consistent with sorting by the comparison operators defined below.

      constexpr inline friend bool operator <=( const symbol_type& lhs, const symbol_type& rhs )
      { return lhs.value <= rhs.value; }
      constexpr inline friend bool operator < ( const symbol_type& lhs, const symbol_type& rhs )
      { return lhs.value <  rhs.value; }
      constexpr inline friend bool operator >=( const symbol_type& lhs, const symbol_type& rhs )
      { return lhs.value >= rhs.value; }
      constexpr inline friend bool operator > ( const symbol_type& lhs, const symbol_type& rhs )
      { return lhs.value >  rhs.value; }
      constexpr inline friend bool operator ==( const symbol_type& lhs, const symbol_type& rhs )
      { return lhs.value == rhs.value; }
      constexpr inline friend bool operator !=( const symbol_type& lhs, const symbol_type& rhs )
      { return lhs.value != rhs.value; }

      void print(bool show_precision=true)const {
         if( show_precision ){
            ::eosio::print(precision());
            prints(",");
         }

         auto sym = value;
         sym >>= 8;
         for( int i = 0; i < 7; ++i ) {
            char c = (char)(sym & 0xff);
            if( !c ) return;
            prints_l(&c, 1 );
            sym >>= 8;
         }
      }

      template<typename DataStream>
      friend DataStream& operator << ( DataStream& ds, const symbol_type& s ){
         return ds << s.value;
      }

      template<typename DataStream>
      friend DataStream& operator >> ( DataStream& ds, symbol_type& s ){
         ds >> s.value;
         eosio_assert( s.is_valid(), "deserialized symbol is invalid" );
         return ds;
      }
   };

   struct extended_symbol : public symbol_type
   {
      account_name contract;

      static constexpr account_name default_contract_name = N(eosio.token);

      constexpr explicit extended_symbol( symbol_name s = default_symbol_name, account_name c = default_contract_name )
      : symbol_type(s), contract(c) {}

      constexpr explicit extended_symbol( uint128_t v )
      : symbol_type(static_cast<symbol_name>(v)), contract(static_cast<account_name>(v >> 64)) {}

      constexpr operator uint128_t()const { return (static_cast<uint128_t>(contract) << 64) + value; }
      /// Sorting by the converted to uint128_t is consistent with sorting by the comparison operators defined below.

      constexpr inline friend bool operator <=( const extended_symbol& lhs, const extended_symbol& rhs )
      { return std::tie(lhs.contract, lhs.value) <= std::tie(rhs.contract, rhs.value); }
      constexpr inline friend bool operator < ( const extended_symbol& lhs, const extended_symbol& rhs )
      { return std::tie(lhs.contract, lhs.value) <  std::tie(rhs.contract, rhs.value); }
      constexpr inline friend bool operator >=( const extended_symbol& lhs, const extended_symbol& rhs )
      { return std::tie(lhs.contract, lhs.value) >= std::tie(rhs.contract, rhs.value); }
      constexpr inline friend bool operator > ( const extended_symbol& lhs, const extended_symbol& rhs )
      { return std::tie(lhs.contract, lhs.value) >  std::tie(rhs.contract, rhs.value); }
      constexpr inline friend bool operator ==( const extended_symbol& lhs, const extended_symbol& rhs )
      { return std::tie(lhs.contract, lhs.value) == std::tie(rhs.contract, rhs.value); }
      constexpr inline friend bool operator !=( const extended_symbol& lhs, const extended_symbol& rhs )
      { return std::tie(lhs.contract, lhs.value) != std::tie(rhs.contract, rhs.value); }

      void print()const {
         symbol_type::print();
         prints("@");
         printn( contract );
      }

      EOSLIB_SERIALIZE_DERIVED( extended_symbol, symbol_type, (contract) )
   };

   namespace _token_detail {

      void print( uint64_t whole_value, bool is_negative, const char* fraction, uint32_t fraction_len ) {
         if( is_negative ) prints("-");
         printui( whole_value );

         if( fraction_len > 0 ) {
            prints(".");
            prints_l( fraction, fraction_len );
         }
      }

      void print( int64_t v, symbol_type symbol ) {
         uint64_t p = symbol.precision(); // Invariants guarantee that 10^(symbol.precision()) can be represented as a uint64_t.
         uint64_t p10 = 1;
         while( p > 0  ) {
            p10 *= 10;
            --p;
         }
         p = symbol.precision();

         char fraction[p+1];
         fraction[p] = '\0';
         auto abs_v  = static_cast<uint64_t>(std::abs(v));
         auto change = abs_v % p10;

         for( uint64_t i = p; i > 0; --i ) {
            fraction[i-1] = (change % 10) + '0';
            change /= 10;
         }
         print( (abs_v / p10), (v < 0), &fraction[0], uint32_t(p) );
         prints(" ");
         symbol.print(false);
      }

   }; /// namespace eosio::_token_detail

   template<typename NumberType = int64_t>
   struct token {
   public:
      safe_number<NumberType> amount;
   protected:
      symbol_type             symbol;

      void validate_token()const {
         eosio_assert( symbol.is_valid(), "invalid symbol name" );
         eosio_assert( std::numeric_limits<uint64_t>::digits10 >= symbol.precision(), "precision of the token is too high" );
      }

   public:
      typedef token<NumberType> token_type;

      /*
      static_assert( symbol_type(symbol_type::default_symbol_name).is_valid(), "default symbol name is invalid" );

      static_assert( std::numeric_limits<uint64_t>::digits10 >= symbol_type(symbol_type::default_symbol_name).precision(),
                     "precision of the default token is too high" );
      */

      token() : amount(NumberType()), symbol(0) {}

      explicit token( const NumberType& v, symbol_name s = symbol_type::default_symbol_name )
      : amount(v), symbol(s)
      {
         validate_token();
      }

      explicit token( const safe_number<NumberType, 0>& q, symbol_name s = symbol_type::default_symbol_name )
      : amount(q), symbol(s)
      {
         validate_token();
      }

      template<uint128_t Unit>
      explicit token( const safe_number<NumberType, Unit>& q,
                      typename std::enable_if<Unit != 0, symbol_name>::type s )
      : amount(q.get_number()), symbol(s)
      {
         eosio_assert( static_cast<symbol_name>(Unit) == s, "mismatch between the symbol and the Unit of safe_number" );
         validate_token();
      }

      token( const token& ) = default;
      token( token&& )      = default;

      token& operator=( const token& other ) {
         bool was_not_initialized = !is_initialized();
         eosio_assert( was_not_initialized || symbol == other.symbol, "token symbol mismatch" );

         amount = other.amount;
         if( was_not_initialized ) {
            symbol = other.symbol;
            validate_token();
         }

         return *this;
      }

      token& operator=( token&& other ) {
         bool was_not_initialized = !is_initialized();
         eosio_assert( was_not_initialized || symbol == other.symbol, "token symbol mismatch" );

         amount = std::move(other.amount);
         if( was_not_initialized ) {
            symbol = std::move(other.symbol);
            validate_token();
         }

         return *this;
      }

      inline symbol_type get_symbol()const { return symbol; }

      void set_symbol(symbol_type s) {
         symbol = s;
         validate_token();
      }

      void reset_to( const token_type& other ) {
         symbol = 0;
         *this = other;
      }

      void reset_to( token_type&& other ) {
         symbol = 0;
         *this = std::move(other);
      }

      inline bool is_initialized()const { return symbol.value != 0; }

      token operator-()const {
         eosio_assert( is_initialized(), "token has not been initialized" );
         token r = *this;
         r.amount = -r.amount;
         return r;
      }

      token& operator +=( const token& t ) {
         eosio_assert( is_initialized(), "token has not been initialized" );
         eosio_assert( symbol == t.symbol, "token symbol mismatch" );
         amount += t.amount;
         return *this;
      }

      token& operator -=( const token& t ) {
         eosio_assert( is_initialized(), "token has not been initialized" );
         eosio_assert( symbol == t.symbol, "token symbol mismatch" );
         amount -= t.amount;
         return *this;
      }

      inline friend token_type operator +( const token_type& lhs, const token_type& rhs ) {
         token sum = lhs;
         sum += rhs;
         return sum;
      }

      inline friend token_type operator -( const token_type& lhs, const token_type& rhs ) {
         token difference = lhs;
         difference -= rhs;
         return difference;
      }

      inline void print()const {
         eosio_assert( is_initialized(), "token has not been initialized" );
         _print<NumberType>();
      }

      template<typename DataStream>
      friend DataStream& operator << ( DataStream& ds, const token_type& t ){
         eosio_assert( t.is_initialized(), "token has not been initialized" );
         return ds << t.amount << t.symbol;
      }

      template<typename DataStream>
      friend DataStream& operator >> ( DataStream& ds, token_type& t ){
         ds >> t.amount >> t.symbol;
         t.validate_token();
         return ds;
      }

   protected:
      template<class T>
      inline void
      _print(typename std::enable_if<std::is_same<T, NumberType>::value &&
                                     ( std::is_floating_point<T>::value ||
                                       (std::is_signed<T>::value && std::is_convertible<T, int64_t>::value) ||
                                       (std::is_unsigned<T>::value && std::is_convertible<T, uint64_t>::value) )>::type* = nullptr)const {
         _token_detail::print( static_cast<int64_t>(amount.get_number()), symbol);
      }

   };

   template<typename NumberType = int64_t>
   struct extended_token : public token<NumberType> {
   public:
      account_name contract;

   public:
      typedef extended_token<NumberType> extended_token_type;
      typedef token<NumberType>          base_token_type;

      extended_token()
      : base_token_type(), contract(extended_symbol::default_contract_name) {}

      explicit extended_token( const base_token_type& t, account_name c = extended_symbol::default_contract_name )
      : base_token_type(t), contract(c) {
         eosio_assert( t.is_initialized(), "cannot pass uninitialized token to extended_token constructor" );
      }

      explicit extended_token( const NumberType& v, extended_symbol s = extended_symbol() )
      : base_token_type(v,s), contract(s.contract) {}

      explicit extended_token( const safe_number<int64_t, 0>& q, extended_symbol s = extended_symbol() )
      : base_token_type(q,s), contract(s.contract) {}

      template<uint128_t Unit>
      explicit extended_token( const safe_number<int64_t, Unit>& q,
                               typename std::enable_if<Unit != 0, extended_symbol>::type s = extended_symbol(Unit) )
      : base_token_type(q,s), contract(s.contract)
      {
         eosio_assert( Unit == static_cast<uint128_t>(s), "mismatch between the extended_symbol and the Unit of safe_number" );
      }

      extended_token( const extended_token& ) = default;
      extended_token( extended_token&& )      = default;

      extended_token& operator=( const extended_token& other ) {
         if( base_token_type::is_initialized() )
            eosio_assert( contract == other.contract, "token contract mismatch" );
         else
            contract = other.contract;
         this->base_token_type::operator=(other);
         contract = other.contract;
         return *this;
      }

      extended_token& operator=( extended_token&& other ) {
         if( base_token_type::is_initialized() )
            eosio_assert( contract == other.contract, "token contract mismatch" );
         else
            contract = std::move(other.contract);
         this->base_token_type::operator=(std::move(other));
         return *this;
      }

      extended_symbol get_extended_symbol()const { return extended_symbol( base_token_type::symbol, contract ); }

      void set_extended_symbol(extended_symbol s) {
         base_token_type::set_symbol(s);
         contract = s.contract;
      }

      void reset_to( const extended_token_type& other ) {
         base_token_type::symbol = 0;
         *this = other;
      }

      void reset_to( extended_token_type&& other ) {
         base_token_type::symbol = 0;
         *this = std::move(other);
      }

      extended_token operator-()const {
         base_token_type r = this->base_token_type::operator-();
         return extended_token{r, contract};
      }

      extended_token& operator +=( const extended_token& t ) {
         static_cast<base_token_type&>(*this) += static_cast<const base_token_type&>(t);
         eosio_assert( contract == t.contract, "token contract mismatch" );
         return *this;
      }

      extended_token& operator -=( const extended_token& t ) {
         static_cast<base_token_type&>(*this) -= static_cast<const base_token_type&>(t);
         eosio_assert( contract == t.contract, "token contract mismatch" );
         return *this;
      }

      inline friend extended_token_type operator +( const extended_token_type& lhs, const extended_token_type& rhs ) {
         extended_token sum = lhs;
         sum += rhs;
         return sum;
      }

      inline friend extended_token_type operator -( const extended_token_type& lhs, const extended_token_type& rhs ) {
         extended_token difference = lhs;
         difference -= rhs;
         return difference;
      }

      void print()const {
         this->base_token_type::print();
         prints("@");
         printn(contract);
      }

      EOSLIB_SERIALIZE_DERIVED( extended_token_type, base_token_type, (contract) )
   };

   typedef token<int64_t>          asset;
   typedef extended_token<int64_t> extended_asset;


} /// namespace eosio
