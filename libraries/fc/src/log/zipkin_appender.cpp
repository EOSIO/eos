#include <fc/network/udp_socket.hpp>
#include <fc/network/ip.hpp>
#include <fc/network/resolve.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/gelf_zipkin_appender.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/crypto/city.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/compress/zlib.hpp>

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <queue>
#include <sstream>

namespace fc
{
   namespace detail
   {
     boost::asio::ip::udp::endpoint to_asio_ep( const fc::ip::endpoint& e ) {
        return boost::asio::ip::udp::endpoint( boost::asio::ip::address_v4( e.get_address() ), e.port() );
     }
   }

   class gelf_zipkin_appender::impl {
   public:
      config cfg;
      optional<boost::asio::ip::udp::endpoint> gelf_endpoint;
      udp_socket gelf_socket;

      explicit impl( config c ) :
            cfg( std::move( c ) ) {
      }

      ~impl() = default;
   };

  gelf_zipkin_appender::gelf_zipkin_appender(const variant& args) :
    my(new impl(args.as<config>()))
  {
  }

  void gelf_zipkin_appender::initialize(boost::asio::io_service &io_service)
  {
     try {
        try {
           // if it's a numeric address:port, this will parse it
           my->gelf_endpoint = detail::to_asio_ep( ip::endpoint::from_string( my->cfg.endpoint ) );
        } catch( ... ) {}
        if( !my->gelf_endpoint ) {
           // couldn't parse as a numeric ip address, try resolving as a DNS name.
           // This can yield, so don't do it in the catch block above
           string::size_type colon_pos = my->cfg.endpoint.find( ':' );
           try {
              uint16_t port = boost::lexical_cast<uint16_t>(
                    my->cfg.endpoint.substr( colon_pos + 1, my->cfg.endpoint.size() ) );

              string hostname = my->cfg.endpoint.substr( 0, colon_pos );
              auto endpoints = resolve( io_service, hostname, port );
              if( endpoints.empty() )
                 FC_THROW_EXCEPTION( unknown_host_exception,
                                     "The logging destination host name can not be resolved: ${hostname}",
                                     ("hostname", hostname) );
              my->gelf_endpoint = endpoints.back();
           } catch( const boost::bad_lexical_cast& ) {
              FC_THROW( "Bad port: ${port}",
                        ("port", my->cfg.endpoint.substr( colon_pos + 1, my->cfg.endpoint.size() )) );
           }
        }

        if( my->gelf_endpoint ) {
           my->gelf_socket.initialize( io_service );
           my->gelf_socket.open();
           std::cerr << "opened GELF socket to endpoint " << my->cfg.endpoint << "\n";
        }
     } catch( ... ) {
        std::cerr << "error opening GELF socket to endpoint " << my->cfg.endpoint << "\n";
     }
  }

  fc::mutable_variant_object create_zipkin_variant(const log_message& message, const log_context& context) {
     // https://zipkin.io/zipkin-api/
     //   std::string traceId;  // [a-f0-9]{16,32} unique id for trace, all children spans shared same id
     //   std::string name;     // logical operation, should have low cardinality
     //   std::string parentId; // The parent span id, or absent if root span
     //   std::string id        // a-f0-9]{16}

     fc::mutable_variant_object d = message.get_data();

     fc::mutable_variant_object mvo;
     auto i = d.find( "zipkin.traceId" );
     if( i != d.end() && i->value().is_uint64() ) {
        mvo( "traceId", fc::to_hex( reinterpret_cast<const char*>(i->value().as_uint64()), sizeof( uint64_t ) ) );
        d.erase( "zipkin.traceId" );
     }
     i = d.find( "zipkin.id" );
     if( i != d.end() && i->value().is_uint64() ) {
        mvo( "id", fc::to_hex( reinterpret_cast<const char*>(i->value().as_uint64()), sizeof( uint64_t ) ) );
        d.erase( "zipkin.id" );
     }
     i = d.find( "zipkin.parentId" );
     if( i != d.end() && i->value().is_uint64() ) {
        uint64_t v = i->value().as_uint64();
        if( v != 0 )
           mvo( "parentId", fc::to_hex( reinterpret_cast<const char*>(v), sizeof( v ) ) );
        d.erase( "zipkin.parentId" );
     }
     i = d.find( "zipkin.name" );
     if( i != d.end() ) {
        mvo( "name", i->value() );
        d.erase( "zipkin.name" );
     }

     mvo( "timestamp", context.get_timestamp().time_since_epoch().count() );

     mvo( "tags", d );

     return mvo;
  }

  void gelf_zipkin_appender::log(const log_message& message)
  {
    if (!my->gelf_endpoint)
      return;

    log_context context = message.get_context();

    mutable_variant_object gelf_message;
    gelf_message["version"] = "1.1";
    gelf_message["host"] = my->cfg.host;
    gelf_message["full_message"] = json::to_string( create_zipkin_variant(message, context),
                                                    fc::time_point::now() + fc::exception::format_time_limit );

    // use now() instead of context.get_timestamp() because log_message construction can include user provided long running calls
    const auto time_ms = time_point::now().time_since_epoch().count();
    gelf_message["timestamp"] = time_ms / 1000000.;
    gelf_message["_timestamp_ms"] = time_ms;

    static uint64_t gelf_log_counter;
    gelf_message["_log_id"] = fc::to_string(++gelf_log_counter);

    switch (context.get_log_level())
    {
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
      // these shouldn't be used in log messages, but do something deterministic just in case
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

    string gelf_message_as_string = json::to_string(gelf_message,
          fc::time_point::now() + fc::exception::format_time_limit,
          json::output_formatting::legacy_generator); // GELF 1.1 specifies unstringified numbers
    //unsigned uncompressed_size = gelf_message_as_string.size();
    gelf_message_as_string = zlib_compress(gelf_message_as_string);

    // graylog2 expects the zlib header to be 0x78 0x9c
    // but miniz.c generates 0x78 0x01 (indicating
    // low compression instead of default compression)
    // so change that here
    FC_ASSERT(gelf_message_as_string[0] == (char)0x78);
    if (gelf_message_as_string[1] == (char)0x01 ||
        gelf_message_as_string[1] == (char)0xda)
      gelf_message_as_string[1] = (char)0x9c;
    FC_ASSERT(gelf_message_as_string[1] == (char)0x9c);

    // packets are sent by UDP, and they tend to disappear if they
    // get too large.  It's hard to find any solid numbers on how
    // large they can be before they get dropped -- datagrams can
    // be up to 64k, but anything over 512 is not guaranteed.
    // You can play with this number, intermediate values like
    // 1400 and 8100 are likely to work on most intranets.
    const unsigned max_payload_size = 512;

    if (gelf_message_as_string.size() <= max_payload_size)
    {
      // no need to split
      std::shared_ptr<char> send_buffer(new char[gelf_message_as_string.size()],
                                        [](char* p){ delete[] p; });
      memcpy(send_buffer.get(), gelf_message_as_string.c_str(),
             gelf_message_as_string.size());

      my->gelf_socket.send_to(send_buffer, gelf_message_as_string.size(),
                              *my->gelf_endpoint);
    }
    else
    {
      // split the message
      // we need to generate an 8-byte ID for this message.
      // city hash should do
      uint64_t message_id = city_hash64(gelf_message_as_string.c_str(), gelf_message_as_string.size());
      const unsigned header_length = 2 /* magic */ + 8 /* msg id */ + 1 /* seq */ + 1 /* count */;
      const unsigned body_length = max_payload_size - header_length;
      unsigned total_number_of_packets = (gelf_message_as_string.size() + body_length - 1) / body_length;
      unsigned bytes_sent = 0;
      unsigned number_of_packets_sent = 0;
      while (bytes_sent < gelf_message_as_string.size())
      {
        unsigned bytes_to_send = std::min((unsigned)gelf_message_as_string.size() - bytes_sent,
                                          body_length);

        std::shared_ptr<char> send_buffer(new char[max_payload_size],
                                          [](char* p){ delete[] p; });
        char* ptr = send_buffer.get();
        // magic number for chunked message
        *(unsigned char*)ptr++ = 0x1e;
        *(unsigned char*)ptr++ = 0x0f;

        // message id
        memcpy(ptr, (char*)&message_id, sizeof(message_id));
        ptr += sizeof(message_id);

        *(unsigned char*)(ptr++) = number_of_packets_sent;
        *(unsigned char*)(ptr++) = total_number_of_packets;
        memcpy(ptr, gelf_message_as_string.c_str() + bytes_sent,
               bytes_to_send);
        my->gelf_socket.send_to(send_buffer, header_length + bytes_to_send,
                                *my->gelf_endpoint);
        ++number_of_packets_sent;
        bytes_sent += bytes_to_send;
      }
      FC_ASSERT(number_of_packets_sent == total_number_of_packets);
    }
  }
} // fc
