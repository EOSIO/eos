#include <fc/compress/zlib.hpp>
#include <fc/crypto/city.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <fc/log/gelf_appender.hpp>
#include <fc/network/ip.hpp>
#include <fc/network/resolve.hpp>
#include <fc/network/udp_socket.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/variant.hpp>

#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>

namespace fc {

class gelf_appender::impl : public retainable {
public:
  config cfg;
  optional<ip::endpoint> gelf_endpoint;
  udp_socket gelf_socket;

  impl(const config &c) : cfg(c) {}

  ~impl() {}
};

gelf_appender::gelf_appender(const variant &args)
    : my(new impl(args.as<config>())) {
  try {
    try {
      // if it's a numeric address:port, this will parse it
      my->gelf_endpoint = ip::endpoint::from_string(my->cfg.endpoint);
    } catch (...) {
    }
    if (!my->gelf_endpoint) {
      // couldn't parse as a numeric ip address, try resolving as a DNS name.
      // This can yield, so don't do it in the catch block above
      string::size_type colon_pos = my->cfg.endpoint.find(':');
      try {
        uint16_t port = boost::lexical_cast<uint16_t>(
            my->cfg.endpoint.substr(colon_pos + 1, my->cfg.endpoint.size()));

        string hostname = my->cfg.endpoint.substr(0, colon_pos);
        std::vector<ip::endpoint> endpoints = resolve(hostname, port);
        if (endpoints.empty())
          FC_THROW_EXCEPTION(unknown_host_exception,
                             "The host name can not be resolved: ${hostname}",
                             ("hostname", hostname));
        my->gelf_endpoint = endpoints.back();
      } catch (const boost::bad_lexical_cast &) {
        FC_THROW(
            "Bad port: ${port}",
            ("port",
             my->cfg.endpoint.substr(colon_pos + 1, my->cfg.endpoint.size())));
      }
    }

    if (my->gelf_endpoint)
      my->gelf_socket.open();
  } catch (...) {
    std::cerr << "error opening GELF socket to endpoint ${endpoint}"
              << my->cfg.endpoint << "\n";
  }
}

gelf_appender::~gelf_appender() {}

void gelf_appender::log(const log_message &message) {
  if (!my->gelf_endpoint)
    return;

  log_context context = message.get_context();

  mutable_variant_object gelf_message;
  gelf_message["version"] = "1.1";
  gelf_message["host"] = my->cfg.host;
  gelf_message["short_message"] =
      format_string(message.get_format(), message.get_data());

  gelf_message["timestamp"] =
      context.get_timestamp().time_since_epoch().count() / 1000000.;

  switch (context.get_log_level()) {
  case log_level::debug:
    gelf_message["level"] = 7; // debug
    break;
  case log_level::info:
    gelf_message["level"] = 6; // info
    break;
  case log_level::warn:
    gelf_message["level"] = 4; // warning
    break;
  case log_level::error:
    gelf_message["level"] = 3; // error
    break;
  case log_level::all:
  case log_level::off:
    // these shouldn't be used in log messages, but do something deterministic
    // just in case
    gelf_message["level"] = 6; // info
    break;
  }

  if (!context.get_context().empty())
    gelf_message["context"] = context.get_context();
  gelf_message["_line"] = context.get_line_number();
  gelf_message["_file"] = context.get_file();
  gelf_message["_method_name"] = context.get_method();
  gelf_message["_thread_name"] = context.get_thread_name();
  if (!context.get_task_name().empty())
    gelf_message["_task_name"] = context.get_task_name();

  string gelf_message_as_string = json::to_string(gelf_message);
  // unsigned uncompressed_size = gelf_message_as_string.size();
  gelf_message_as_string = zlib_compress(gelf_message_as_string);

  // graylog2 expects the zlib header to be 0x78 0x9c
  // but miniz.c generates 0x78 0x01 (indicating
  // low compression instead of default compression)
  // so change that here
  assert(gelf_message_as_string[0] == (char)0x78);
  if (gelf_message_as_string[1] == (char)0x01 ||
      gelf_message_as_string[1] == (char)0xda)
    gelf_message_as_string[1] = (char)0x9c;
  assert(gelf_message_as_string[1] == (char)0x9c);

  // packets are sent by UDP, and they tend to disappear if they
  // get too large.  It's hard to find any solid numbers on how
  // large they can be before they get dropped -- datagrams can
  // be up to 64k, but anything over 512 is not guaranteed.
  // You can play with this number, intermediate values like
  // 1400 and 8100 are likely to work on most intranets.
  const unsigned max_payload_size = 512;

  if (gelf_message_as_string.size() <= max_payload_size) {
    // no need to split
    std::shared_ptr<char> send_buffer(new char[gelf_message_as_string.size()],
                                      [](char *p) { delete[] p; });
    memcpy(send_buffer.get(), gelf_message_as_string.c_str(),
           gelf_message_as_string.size());

    my->gelf_socket.send_to(send_buffer, gelf_message_as_string.size(),
                            *my->gelf_endpoint);
  } else {
    // split the message
    // we need to generate an 8-byte ID for this message.
    // city hash should do
    uint64_t message_id = city_hash64(gelf_message_as_string.c_str(),
                                      gelf_message_as_string.size());
    const unsigned header_length =
        2 /* magic */ + 8 /* msg id */ + 1 /* seq */ + 1 /* count */;
    const unsigned body_length = max_payload_size - header_length;
    unsigned total_number_of_packets =
        (gelf_message_as_string.size() + body_length - 1) / body_length;
    unsigned bytes_sent = 0;
    unsigned number_of_packets_sent = 0;
    while (bytes_sent < gelf_message_as_string.size()) {
      unsigned bytes_to_send = std::min(
          (unsigned)gelf_message_as_string.size() - bytes_sent, body_length);

      std::shared_ptr<char> send_buffer(new char[max_payload_size],
                                        [](char *p) { delete[] p; });
      char *ptr = send_buffer.get();
      // magic number for chunked message
      *(unsigned char *)ptr++ = 0x1e;
      *(unsigned char *)ptr++ = 0x0f;

      // message id
      memcpy(ptr, (char *)&message_id, sizeof(message_id));
      ptr += sizeof(message_id);

      *(unsigned char *)(ptr++) = number_of_packets_sent;
      *(unsigned char *)(ptr++) = total_number_of_packets;
      memcpy(ptr, gelf_message_as_string.c_str() + bytes_sent, bytes_to_send);
      my->gelf_socket.send_to(send_buffer, header_length + bytes_to_send,
                              *my->gelf_endpoint);
      ++number_of_packets_sent;
      bytes_sent += bytes_to_send;
    }
    assert(number_of_packets_sent == total_number_of_packets);
  }
}
} // fc
