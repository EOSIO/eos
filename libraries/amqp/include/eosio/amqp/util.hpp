#pragma once

#include <amqpcpp.h>
#include <fc/variant.hpp>
#include <fmt/format.h>

namespace fc {
void to_variant(const AMQP::Address& a, fc::variant& v);
}

namespace fmt {
template<>
struct formatter<AMQP::Address> {
   template<typename ParseContext>
   constexpr auto parse( ParseContext& ctx ) { return ctx.begin(); }

   template<typename FormatContext>
   auto format( const AMQP::Address& a, FormatContext& ctx ) {
      std::string str(a.secure() ? "amqps://" : "amqp://");
      str.append("********:********").append("@");
      str.append(a.hostname().empty() ? "localhost" : a.hostname());
      if(a.port() != 5672)
         str.append(":").append(std::to_string(a.port()));
      str.append("/");
      if (a.vhost() != "/")
         str.append(a.vhost());
      return format_to( ctx.out(), "{}", std::move(str) );
   }
};
}
