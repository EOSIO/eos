#include <eosio/amqp/util.hpp>
namespace fc {
void to_variant(const AMQP::Address& a, fc::variant& v) {
   std::string str(a.secure() ? "amqps://" : "amqp://");
   str.append(a.login().user()).append(":********").append("@");
   str.append(a.hostname().empty() ? "localhost" : a.hostname());
   if(a.port() != 5672)
      str.append(":").append(std::to_string(a.port()));
   str.append("/");
   if (a.vhost() != "/")
      str.append(a.vhost());

   v = std::move(str);
}
}