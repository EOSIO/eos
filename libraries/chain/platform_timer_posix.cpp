#include <eosio/chain/platform_timer.hpp>
#include <eosio/chain/platform_timer_accuracy.hpp>

#include <fc/time.hpp>
#include <fc/fwd_impl.hpp>
#include <fc/exception/exception.hpp>

#include <mutex>

#include <signal.h>
#include <time.h>

namespace eosio { namespace chain {

static_assert(std::atomic_bool::is_always_lock_free, "Only lock-free atomics AS-safe.");

struct platform_timer::impl {
   timer_t timerid;

   static void sig_handler(int, siginfo_t* si, void*) {
      platform_timer* self = (platform_timer*)si->si_value.sival_ptr;
      self->expired = 1;
      self->call_expiration_callback();
   }
};

platform_timer::platform_timer() {
   static_assert(sizeof(impl) <= fwd_size);

   static bool initialized;
   static std::mutex initalized_mutex;

   if(std::lock_guard guard(initalized_mutex); !initialized) {
      struct sigaction act;
      sigemptyset(&act.sa_mask);
      act.sa_sigaction = impl::sig_handler;
      act.sa_flags = SA_SIGINFO;
      FC_ASSERT(sigaction(SIGRTMIN, &act, NULL) == 0, "failed to aquire SIGRTMIN signal");
      initialized = true;
   }

   struct sigevent se;
   se.sigev_notify = SIGEV_SIGNAL;
   se.sigev_signo = SIGRTMIN;
   se.sigev_value.sival_ptr = (void*)this;

   FC_ASSERT(timer_create(CLOCK_REALTIME, &se, &my->timerid) == 0, "failed to create timer");

   compute_and_print_timer_accuracy(*this);
}

platform_timer::~platform_timer() {
   timer_delete(my->timerid);
}

void platform_timer::start(fc::time_point tp) {
   if(tp == fc::time_point::max()) {
      expired = 0;
      return;
   }
   fc::microseconds x = tp.time_since_epoch() - fc::now().time_since_epoch();
   if(x.count() <= 0)
      expired = 1;
   else {
      struct itimerspec enable = {{0, 0}, {0, (int)x.count()*1000}};
      expired = 0;
      if(timer_settime(my->timerid, 0, &enable, NULL) != 0)
         expired = 1;
   }
}

void platform_timer::stop() {
   if(expired)
      return;
   struct itimerspec disable = {{0, 0}, {0, 0}};
   timer_settime(my->timerid, 0, &disable, NULL);
   expired = 1;
}

}}
