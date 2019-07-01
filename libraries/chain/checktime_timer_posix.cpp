#include <eosio/chain/checktime_timer.hpp>
#include <eosio/chain/checktime_timer_calibrate.hpp>

#include <fc/time.hpp>
#include <fc/fwd_impl.hpp>
#include <fc/exception/exception.hpp>

#include <mutex>

#include <signal.h>
#include <time.h>

namespace eosio { namespace chain {

static checktime_timer_calibrate the_calibrate;

struct checktime_timer::impl {
   timer_t timerid;

   static void sig_handler(int, siginfo_t* si, void*) {
      *(sig_atomic_t*)(si->si_value.sival_ptr) = 1;
   }
};

checktime_timer::checktime_timer() {
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
   se.sigev_value.sival_ptr = (void*)&expired;

   FC_ASSERT(timer_create(CLOCK_REALTIME, &se, &my->timerid) == 0, "failed to create timer");

   the_calibrate.do_calibrate(*this);
}

checktime_timer::~checktime_timer() {
   timer_delete(my->timerid);
}

void checktime_timer::start(fc::time_point tp) {
   if(tp == fc::time_point::maximum()) {
      expired = 0;
      return;
   }
   if(!the_calibrate.use_timer()) {
      expired = 1;
      return;
   }
   fc::microseconds x = tp.time_since_epoch() - fc::time_point::now().time_since_epoch();
   if(x.count() <= the_calibrate.timer_overhead())
      expired = 1;
   else {
      struct itimerspec enable = {{0, 0}, {0, ((int)x.count()-the_calibrate.timer_overhead())*1000}};
      expired = 0;
      if(timer_settime(my->timerid, 0, &enable, NULL) != 0)
         expired = 1;
   }
}

void checktime_timer::stop() {
   if(expired)
      return;
   struct itimerspec disable = {{0, 0}, {0, 0}};
   timer_settime(my->timerid, 0, &disable, NULL);
   expired = 1;
}

}}