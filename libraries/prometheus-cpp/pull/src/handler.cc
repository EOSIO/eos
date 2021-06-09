#include "handler.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>

#ifdef HAVE_ZLIB
#include <zconf.h>
#include <zlib.h>
#endif

#include "civetweb.h"
#include "metrics_collector.h"
#include "prometheus/counter.h"
#include "prometheus/metric_family.h"
#include "prometheus/summary.h"
#include "prometheus/text_serializer.h"

namespace prometheus {
namespace detail {

MetricsHandler::MetricsHandler(Registry& registry)
    : bytes_transferred_family_(
          BuildCounter()
              .Name("exposer_transferred_bytes_total")
              .Help("Transferred bytes to metrics services")
              .Register(registry)),
      bytes_transferred_(bytes_transferred_family_.Add({})),
      num_scrapes_family_(BuildCounter()
                              .Name("exposer_scrapes_total")
                              .Help("Number of times metrics were scraped")
                              .Register(registry)),
      num_scrapes_(num_scrapes_family_.Add({})),
      request_latencies_family_(
          BuildSummary()
              .Name("exposer_request_latencies")
              .Help("Latencies of serving scrape requests, in microseconds")
              .Register(registry)),
      request_latencies_(request_latencies_family_.Add(
          {}, Summary::Quantiles{{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}})) {}

#ifdef HAVE_ZLIB
static bool IsEncodingAccepted(struct mg_connection* conn,
                               const char* encoding) {
  auto accept_encoding = mg_get_header(conn, "Accept-Encoding");
  if (!accept_encoding) {
    return false;
  }
  return std::strstr(accept_encoding, encoding) != nullptr;
}

static std::vector<Byte> GZipCompress(const std::string& input) {
  auto zs = z_stream{};
  auto windowSize = 16 + MAX_WBITS;
  auto memoryLevel = 9;

  if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowSize,
                   memoryLevel, Z_DEFAULT_STRATEGY) != Z_OK) {
    return {};
  }

  zs.next_in = (Bytef*)input.data();
  zs.avail_in = input.size();

  int ret;
  std::vector<Byte> output;
  output.reserve(input.size() / 2u);

  do {
    static const auto outputBytesPerRound = std::size_t{32768};

    zs.avail_out = outputBytesPerRound;
    output.resize(zs.total_out + zs.avail_out);
    zs.next_out = reinterpret_cast<Bytef*>(output.data() + zs.total_out);

    ret = deflate(&zs, Z_FINISH);

    output.resize(zs.total_out);
  } while (ret == Z_OK);

  deflateEnd(&zs);

  if (ret != Z_STREAM_END) {
    return {};
  }

  return output;
}
#endif

static std::size_t WriteResponse(struct mg_connection* conn,
                                 const std::string& body) {
  mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain; charset=utf-8\r\n");

#ifdef HAVE_ZLIB
  auto acceptsGzip = IsEncodingAccepted(conn, "gzip");

  if (acceptsGzip) {
    auto compressed = GZipCompress(body);
    if (!compressed.empty()) {
      mg_printf(conn,
                "Content-Encoding: gzip\r\n"
                "Content-Length: %lu\r\n\r\n",
                static_cast<unsigned long>(compressed.size()));
      mg_write(conn, compressed.data(), compressed.size());
      return compressed.size();
    }
  }
#endif

  mg_printf(conn, "Content-Length: %lu\r\n\r\n",
            static_cast<unsigned long>(body.size()));
  mg_write(conn, body.data(), body.size());
  return body.size();
}

void MetricsHandler::RegisterCollectable(
    const std::weak_ptr<Collectable>& collectable) {
  std::lock_guard<std::mutex> lock{collectables_mutex_};
  CleanupStalePointers(collectables_);
  collectables_.push_back(collectable);
}

void MetricsHandler::RemoveCollectable(
    const std::weak_ptr<Collectable>& collectable) {
  std::lock_guard<std::mutex> lock{collectables_mutex_};

  auto locked = collectable.lock();
  auto same_pointer = [&locked](const std::weak_ptr<Collectable>& candidate) {
    return locked == candidate.lock();
  };

  collectables_.erase(std::remove_if(std::begin(collectables_),
                                     std::end(collectables_), same_pointer),
                      std::end(collectables_));
}

bool MetricsHandler::handleGet(CivetServer*, struct mg_connection* conn) {
  auto start_time_of_request = std::chrono::steady_clock::now();

  std::vector<MetricFamily> metrics;

  {
    std::lock_guard<std::mutex> lock{collectables_mutex_};
    metrics = CollectMetrics(collectables_);
  }

  const TextSerializer serializer;

  auto bodySize = WriteResponse(conn, serializer.Serialize(metrics));

  auto stop_time_of_request = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      stop_time_of_request - start_time_of_request);
  request_latencies_.Observe(duration.count());

  bytes_transferred_.Increment(bodySize);
  num_scrapes_.Increment();
  return true;
}

void MetricsHandler::CleanupStalePointers(
    std::vector<std::weak_ptr<Collectable>>& collectables) {
  collectables.erase(
      std::remove_if(std::begin(collectables), std::end(collectables),
                     [](const std::weak_ptr<Collectable>& candidate) {
                       return candidate.expired();
                     }),
      std::end(collectables));
}
}  // namespace detail
}  // namespace prometheus
