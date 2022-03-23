#pragma once

#include <eosio/chain/action.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/contract_types.hpp>

#include <fc/reflect/reflect.hpp>
#include <fc/io/json.hpp>
#include <fc/crypto/hex.hpp>
#include <string>

namespace fc {

static constexpr size_t hex_log_max_size = 32;

template<typename T>
struct member_pointer_value {
   typedef T type;
};

template<typename Class, typename Value>
struct member_pointer_value<Value Class::*> {
   typedef Value type;
};

template<class T, template<class...> class Primary>
struct is_specialization_of : std::false_type {};

template<template<class...> class Primary, class... Args>
struct is_specialization_of<Primary<Args...>, Primary> : std::true_type {};

template<class T, template<class...> class Primary>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, Primary>::value;

template<typename T, typename _ = void>
struct is_container : std::false_type {};

template<typename... Ts>
struct is_container_helper {};

template<typename T>
struct is_container<
      T,
      std::conditional_t<
            false,
            is_container_helper<
// fc::array does not have these commented out traits
//                  typename T::value_type,
//                  typename T::size_type,
//                  typename T::iterator,
//                  typename T::const_iterator,
                  decltype(std::declval<T>().size()),
                  decltype(std::declval<T>().begin()),
                  decltype(std::declval<T>().end())//,
//                  decltype(std::declval<T>().cbegin()),
//                  decltype(std::declval<T>().cend())
            >,
            void
      >
> : public std::true_type {};

template<typename S, typename T>
class is_streamable {
   template<typename SS, typename TT>
   static auto test(int) -> decltype( std::declval<SS&>() << std::declval<TT>(), std::true_type() );

   template<typename, typename>
   static auto test(...) -> std::false_type;
public:
   static const bool value = decltype(test<S,T>(0))::value;
};

template<typename T>
class to_string_visitor;

namespace to_str {

template<typename X>
void append_value( std::string& out, const X& t ) {
   out += "\"";
   if constexpr( std::is_same_v<X, eosio::chain::name> ) {
      out += t.to_string();
   } else if constexpr( std::is_same_v<X, fc::microseconds> ) {
      out += std::to_string(t.count()) + "us";
   } else if constexpr ( std::is_same_v<X, bool> ) {
      out += t ? "true" : "false";
   } else if constexpr ( std::is_integral_v<X> ) {
      out += std::to_string( t );
   } else if constexpr( std::is_same_v<X, std::string> ) {
      const bool escape_control_chars = true;
      out += fc::escape_string( t, nullptr, escape_control_chars );
   } else if constexpr( std::is_convertible_v<X, std::string> ) {
      out += (std::string) t;
   } else {
      static_assert(std::is_integral_v<X>, "Not FC_REFLECT or fc::to_str::append_value for type");
      out += "~unknown~";
   }
   out += "\"";
}

template<typename X>
void append_str(std::string& out, const char* name, const X& t) {
   out += "\"";
   out += name;
   out += "\":";
   append_value( out, t );
}

template<typename X>
void process_type_str( std::string& out, const char* name, const X& t);

template<typename X, int Depth = 0>
void process_type( std::string& out, const X& t) {
   using mem_type = std::decay_t<X>;
   if constexpr ( std::is_integral_v<mem_type> ) {
      append_value( out, t );
   } else if constexpr( std::is_same_v<mem_type, std::string> ) {
      append_value( out, t );
   } else if constexpr( std::is_same_v<mem_type, eosio::chain::name> ) {
      append_value( out, t );
   } else if constexpr( std::is_same_v<mem_type, fc::unsigned_int> || std::is_same_v<mem_type, fc::signed_int> ) {
      append_value( out, t.value );
   } else if constexpr( std::is_convertible_v<mem_type, std::exception> ) {
      append_value( out, t.to_string() );
   } else if constexpr( std::is_convertible_v<mem_type, std::string> ) {
      append_value( out, t );
   } else if constexpr( std::is_same_v<mem_type, eosio::chain::action> && Depth == 0 ) {
      const auto& act = t;
      if( act.account == eosio::chain::config::system_account_name && act.name == eosio::chain::name( "setcode" ) ) {
         fc::to_str::append_str( out, "account", act.account ); out += ",";
         fc::to_str::append_str( out, "name", act.name ); out += ",";
         fc::to_str::process_type_str( out, "authorization", act.authorization ); out += ",";
         auto setcode_act = act.template data_as<eosio::chain::setcode>();
         if( setcode_act.code.size() > 0 ) {
            fc::sha256 code_hash = fc::sha256::hash( setcode_act.code.data(), (uint32_t) setcode_act.code.size() );
            fc::to_str::append_str( out, "code_hash", code_hash );
            out += ",";
         }
         fc::to_str::process_type_str( out, "data", act.data );
      } else {
         return process_type<mem_type, 1>( out, t );
      }
   } else if constexpr ( is_specialization_of_v<mem_type, std::optional> ) {
      if( t.has_value() ) {
         process_type( out, *t );
      } else {
         out += "null";
      }
   } else if constexpr ( is_specialization_of_v<mem_type, std::shared_ptr> ) {
      if( !!t ) {
         process_type( out, *t );
      } else {
         out += "null";
      }
   } else if constexpr ( is_specialization_of_v<mem_type, std::pair> ) {
      out += "[";
      process_type(out, t.first); out += ",";
      process_type(out, t.second);
      out += "]";
   } else if constexpr ( is_specialization_of_v<mem_type, std::variant> ) {
      out += "[";
      process_type(out, t.index()); out += ",";
      size_t n = 0;
      std::visit([&](auto&& arg) {
         if( ++n > 1 ) out += ",";
         process_type(out, arg);
      }, t);
      out += "]";
   } else if constexpr( std::is_same_v<mem_type, std::vector<char>> ) {
      out += "{";
      if( t.size() > hex_log_max_size ) {
         fc::to_str::append_str( out, "size", t.size() ); out += ",";
         fc::to_str::append_str( out, "trimmed_hex", fc::to_hex( &t[0], hex_log_max_size ) );
      } else if( t.size() > 0 ) {
         fc::to_str::append_str( out, "hex", fc::to_hex( &t[0], t.size() ) );
      } else {
         fc::to_str::append_str( out, "hex", "" );
      }
      out += "}";
   } else if constexpr ( is_container<mem_type>::value ) {
      out += "[";
      size_t n = 0;
      for( const auto& i: t ) {
         if( ++n > 1 ) out += ",";
         process_type( out, i );
      }
      out += "]";
   } else if constexpr( std::is_same_v<typename fc::reflector<mem_type>::is_defined, fc::true_type> ) {
      fc::to_string_visitor<mem_type> v( t, out );
      fc::reflector<mem_type>::visit( v );
   } else if constexpr( is_streamable<std::stringstream, mem_type>::value ) {
      std::stringstream ss;
      ss << t;
      append_value( out, ss.str() );
   } else {
      append_value( out, t );
   }
}

template<typename X>
void process_type_str( std::string& out, const char* name, const X& t) {
   out += "\"";
   out += name;
   out += "\":";
   process_type( out, t );
}

} // namespace to_str

template<typename T>
class to_string_visitor {
public:
   to_string_visitor( const T& v, std::string& out )
         : obj( v ), out( out ) {
      out += "{";
   }

   ~to_string_visitor() {
      out += "}";
   }

   /**
    * Visit a single member and extract it from the variant object
    * @tparam Member - the member to visit
    * @tparam Class - the class we are traversing
    * @tparam member - pointer to the member
    * @param name - the name of the member
    */
   template<typename Member, class Class, Member (Class::*member)>
   void operator()( const char* name ) const {
      using mem_type = std::decay_t<typename member_pointer_value<decltype( this->obj.*member )>::type>;

      if( ++depth > 1 ) {
         out += ",";
      }

      out += "\""; out += name; out += "\":";
      to_str::process_type<mem_type>( out, this->obj.*member );
   }

private:
   const T& obj;
   std::string& out;
   mutable uint32_t depth = 0;
};

template <typename T, typename = std::enable_if_t<fc::reflector<std::decay_t<T>>::is_defined::value>>
std::string to_json_string(const T& t) {
   std::string out;
   to_string_visitor<std::decay_t<T>> v( t, out );
   fc::reflector<std::decay_t<T>>::visit( v );
   return out;
}

} // namespace fc

namespace fmt {

template<typename T, typename Char>
struct formatter<T, Char, typename std::enable_if_t<fc::reflector<std::decay_t<T>>::is_defined::value>> {
   template<typename ParseContext>
   constexpr auto parse( ParseContext& ctx ) { return ctx.begin(); }

   template<typename FormatContext, typename = std::enable_if_t<fc::reflector<std::decay_t<T>>::is_defined::value>>
   auto format( const T& p, FormatContext& ctx ) {
      return format_to( ctx.out(), "{}", fc::to_json_string<std::decay_t<T>>(p) );
   }
};

} // namespace fmt
