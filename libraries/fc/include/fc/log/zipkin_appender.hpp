#pragma once

#include <fc/log/appender.hpp>
#include <fc/log/logger.hpp>
#include <string>

namespace fc
{
  // Active Log appender that sends zipkin messages in JSON format
  // https://zipkin.io/zipkin-api/
  //
  // log_message should contain following variant data
  //     uint64_t    zipkin.traceId   - unique id for trace, all children spans shared same id
  //     std::string zipkin.name      - logical operation, should have low cardinality
  //     uint64_t    zipkin.parentId  - The parent span id, or absent if root span
  //     uint64_t    zipkin.id        - unique if for this span
  //     int64_t     zipkin.timestamp - epoch microseconds of start of span
  //     int64_t     zipkin.duration  - microseconds of span

  class zipkin_appender final : public appender {
  public:
     struct config {
        std::string endpoint = "http://127.0.0.1:9411";
        std::string path = "/api/v2/spans"; // path to post span
        std::string service_name = "unknown"; // localEndpoint.serviceName reported on each Span
        uint32_t    timeout_us = 200*1000; // 200ms
     };

     explicit zipkin_appender( const variant& args );
     ~zipkin_appender() = default;

     // no-op. connection made on first log()
     void initialize( boost::asio::io_service& io_service ) override;

     // finish logging all queued up spans
     void shutdown() override;

     // Logs zipkin json via http on separate thread
     void log( const log_message& m ) override;

  private:
     class impl;
     std::shared_ptr<impl> my;
  };
} // namespace fc

#include <fc/reflect/reflect.hpp>
FC_REFLECT( fc::zipkin_appender::config,
            (endpoint)(path)(service_name)(timeout_us) )
