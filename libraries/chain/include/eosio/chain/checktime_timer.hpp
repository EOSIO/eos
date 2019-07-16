#pragma once
#include <fc/time.hpp>
#include <fc/fwd.hpp>

#include <signal.h>

namespace eosio { namespace chain {

struct checktime_timer {
   checktime_timer();
   ~checktime_timer();

   void start(fc::time_point tp);
   void stop();

   volatile sig_atomic_t expired = 1;

private:
   struct impl;
   constexpr static size_t fwd_size = 8;
   fc::fwd<impl,fwd_size> my;
};

struct checktime_timer_scoped_stop {
   checktime_timer_scoped_stop(checktime_timer& t) : _timer(t) {}
   ~checktime_timer_scoped_stop() {
      _timer.stop();
   }
private:
   checktime_timer& _timer;
};

}}