#pragma once

#include <fc/log/appender.hpp>
#include <fc/log/logger.hpp>
#include <string>

namespace fc
{
  // Log appender that sends zipkin messages in JSON format over gelf
  // The zipkin format is part of the full_message of gelf.
  // The gelf full_message contains a zipkin span
  // https://docs.graylog.org/en/3.2/pages/gelf.html#gelf-payload-specification
  // https://zipkin.io/zipkin-api/
  //
  // log_message should contain following variant data
  //     uint64_t zipkin.traceId  - unique id for trace, all children spans shared same id
  //     std::string zipkin.name  - logical operation, should have low cardinality
  //     uint64_t zipkin.parentId - The parent span id, or absent if root span
  //     uint64_t zipkin.id       - unique if for this span
  class gelf_zipkin_appender final : public appender {
  public:
     struct config {
        std::string endpoint = "127.0.0.1:12201";
        std::string host = "fc"; // the name of the host, source or application that sent this message (just passed through to GELF server)
     };

     explicit gelf_zipkin_appender( const variant& args );
     ~gelf_zipkin_appender() = default;

     /** \brief Required for name resolution and socket initialization.
      *
      * \warning If this method is not called, this appender will log nothing.
      */
     void initialize( boost::asio::io_service& io_service ) override;
     void log( const log_message& m ) override;

  private:
     class impl;
     std::shared_ptr<class impl> my;
  };
} // namespace fc

#include <fc/reflect/reflect.hpp>
FC_REFLECT(fc::gelf_zipkin_appender::config,
           (endpoint)(host))
