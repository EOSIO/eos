#pragma once

#include <memory>

namespace eosio { namespace chain {

struct checktime_timer_calibrate {
   //on construction it will do the calibration
   checktime_timer_calibrate();
   ~checktime_timer_calibrate();

   bool use_timer() const {return _use_timer;}
   int timer_overhead() const {return _timer_overhead;}

   void print_stats();

private:
   //These two values will be inspected by checktime_timer impls to determine if they
   // should use the timer (accurate enough) and what slop to give the expiration. Set
   // the defaults to a "perfect" timer because checktime_timer_calibrate will create
   // a checktime_timer to calibrate with.
   bool _use_timer = true;
   int _timer_overhead = 0;

   struct impl;
   std::unique_ptr<impl> my;
};

}}