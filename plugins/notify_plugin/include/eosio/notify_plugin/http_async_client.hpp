#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_future.hpp>
#include <fc/variant.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/time.hpp>
#include <fc/network/http/http_client.hpp>
#include <thread>

namespace eosio
{
using namespace fc;
namespace asio = boost::asio;

template <typename F>
struct final_action
{
  final_action(F f) : clean{f} {}
  ~final_action() { clean(); }

private:
  F clean;
};

template <typename F>
final_action<F> finally(F f)
{
  return final_action<F>(f);
}

class http_async_client
{
public:
  http_async_client() : sync_client(std::make_unique<http_client>()),
                        work_guard(asio::make_work_guard(ioc)) {}

  ~http_async_client()
  {
    work_guard.reset();
  }

  void start()
  {
    worker = std::make_unique<std::thread>([this]() {
      ioc.run();
    });
  }

  void stop()
  {
    work_guard.reset();
    worker->join();
  }

  void set_default_retry_times(int64_t t) {
    default_retry_times = t;
  }

  template <typename T>
  void post(const url &dest, const T &payload, const time_point &deadline = time_point::maximum())
  {
    asio::post(ioc.get_executor(), [this, dest, payload, deadline]() {
      post_sync(dest, payload, deadline);
    });
  }

private:
  template <typename T>
  void post_sync(const url &dest, const T &payload,
                 const time_point &deadline = time_point::maximum())
  {
    auto exit = finally([this]() { 
      retry_times = default_retry_times;
    });

    try
    {
      sync_client->post_sync(dest, payload, deadline);
    }
    catch (const fc::eof_exception &exc)
    {
    }
    catch (const fc::assert_exception &exc)
    {
      wlog("Exception while trying to send: ${exc}", ("exc", exc.to_detail_string()));
      if (retry_times > 0)
      {
        wlog("Trying ${t} times: ", ("t", retry_times));
        retry_times--;
        post_sync(dest, payload, deadline);
      }
    }
    FC_CAPTURE_AND_LOG((dest)(payload)(deadline))
  };

  std::unique_ptr<http_client> sync_client;
  std::unique_ptr<std::thread> worker;
  asio::io_context ioc;
  asio::executor_work_guard<asio::io_context::executor_type> work_guard;
  int64_t default_retry_times = 3;
  int64_t retry_times = default_retry_times;
};
} // namespace eosio