#pragma once

#include <memory>

namespace eosio { namespace chain {

struct checktime_timer;

struct checktime_timer_calibrate {
   checktime_timer_calibrate();
   //will run calibration if not already run on this instance
   void do_calibrate(checktime_timer& t);
   ~checktime_timer_calibrate();

   bool use_timer() const {return _use_timer;}
   int timer_overhead() const {return _timer_overhead;}

private:
   //These two values will be inspected by checktime_timer impls to determine if they
   // should use the timer (accurate enough) and what slop to give the expiration. Set
   // the defaults to a "perfect" timer because checktime_timer_calibrate will create
   // a checktime_timer to calibrate with.
   bool _use_timer = true;
   int _timer_overhead = 0;
};

}}