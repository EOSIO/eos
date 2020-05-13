#include <eosio/chain/platform_timer.hpp>
#include <eosio/chain/platform_timer_accuracy.hpp>

#include <fc/fwd_impl.hpp>
#include <fc/log/logger_config.hpp> //set_os_thread_name()

#include <boost/asio.hpp>

#include <mutex>
#include <thread>

namespace eosio { namespace chain {

//a thread is shared for all instances
static std::mutex timer_ref_mutex;
static unsigned refcount;
static std::thread checktime_thread;
static std::unique_ptr<boost::asio::io_service> checktime_ios;

struct platform_timer::impl {
   std::unique_ptr<boost::asio::high_resolution_timer> timer;
};

platform_timer::platform_timer() {
   static_assert(sizeof(impl) <= fwd_size);

   std::lock_guard guard(timer_ref_mutex);

   if(refcount++ == 0) {
      std::promise<void> p;
      checktime_thread = std::thread([&p]() {
         fc::set_os_thread_name("checktime");
         checktime_ios = std::make_unique<boost::asio::io_service>();
         boost::asio::io_service::work work(*checktime_ios);
         p.set_value();

         checktime_ios->run();
      });
      p.get_future().get();
   }

   my->timer = std::make_unique<boost::asio::high_resolution_timer>(*checktime_ios);

   //compute_and_print_timer_accuracy(*this);
}

platform_timer::~platform_timer() {
   stop();
   if(std::lock_guard guard(timer_ref_mutex); --refcount == 0) {
      checktime_ios->stop();
      checktime_thread.join();
      checktime_ios.reset();
   }
}

void platform_timer::start(fc::time_point tp) {
   if(tp == fc::time_point::max()) {
      expired = 0;
      return;
   }
   fc::microseconds x = tp - fc::now();
   if(x.count() <= 0)
      expired = 1;
   else {
#if 0
      std::promise<void> p;
      checktime_ios->post([&p,this]() {
         expired = 0;
         p.set_value();
      });
      p.get_future().get();
#endif
      expired = 0;
      my->timer->expires_after(std::chrono::microseconds((int)x.count()));
      my->timer->async_wait([this](const boost::system::error_code& ec) {
         if(ec)
            return;
         expired = 1;
         call_expiration_callback();
      });
   }
}

void platform_timer::stop() {
   if(expired)
      return;

   my->timer->cancel();
   expired = 1;
}

}}
