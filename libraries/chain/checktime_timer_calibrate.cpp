#include <eosio/chain/checktime_timer_calibrate.hpp>
#include <eosio/chain/checktime_timer.hpp>

#include <fc/time.hpp>
#include <fc/log/logger.hpp>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/weighted_mean.hpp>
#include <boost/accumulators/statistics/weighted_variance.hpp>

#include <chrono>

namespace eosio { namespace chain {

namespace bacc = boost::accumulators;

checktime_timer_calibrate::~checktime_timer_calibrate() = default;

struct checktime_timer_calibrate::impl {
   bacc::accumulator_set<int, bacc::stats<bacc::tag::mean, bacc::tag::min, bacc::tag::max, bacc::tag::variance>, float> samples;
   bool _stats_once_is_enough = false;
};

checktime_timer_calibrate::checktime_timer_calibrate() : my(std::make_unique<impl>()) {
   checktime_timer timer;

   //keep longest first in list. You're effectively going to take test_intervals[0]*sizeof(test_intervals[0])
   //time to do the the "calibration"
   int test_intervals[] = {50000, 10000, 5000, 1000, 500, 100, 50, 10};

   for(int& interval : test_intervals) {
      unsigned int loops = test_intervals[0]/interval;

      for(unsigned int i = 0; i < loops; ++i) {
         auto start = std::chrono::high_resolution_clock::now();
         timer.start(fc::time_point(fc::time_point::now().time_since_epoch() + fc::microseconds(interval)));
         while(!timer.expired) {}
         auto end = std::chrono::high_resolution_clock::now();
         int timer_slop = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() - interval;

         //since more samples are run for the shorter expirations, weigh the longer expirations accordingly. This
         //helps to make a few results more fair. Two such examples: AWS c4&i5 xen instances being rather stable
         //down to 100us but then struggling with 50us and 10us. MacOS having performance that seems to correlate
         //with expiry length; that is, long expirations have high error, short expirations have low error.
         //That said, for these platforms, a tighter tolerance may possibly be achieved by taking performance
         //metrics in mulitple bins and appliying the slop based on which bin a deadline resides in. Not clear
         //if that's worth the extra complexity at this point.
         my->samples(timer_slop, bacc::weight = interval/(float)test_intervals[0]);
      }
   }
   _timer_overhead = bacc::mean(my->samples) + sqrt(bacc::variance(my->samples))*2; //target 95% of expirations before deadline
   _use_timer = _timer_overhead < 1000;
}

void checktime_timer_calibrate::print_stats() {
   #define TIMER_STATS_FORMAT "min:${min}us max:${max}us mean:${mean}us stddev:${stddev}us"
   #define TIMER_STATS \
      ("min", bacc::min(my->samples))("max", bacc::max(my->samples)) \
      ("mean", (int)bacc::mean(my->samples))("stddev", (int)sqrt(bacc::variance(my->samples))) \
      ("t", _timer_overhead)

   if(my->_stats_once_is_enough)
      return;
   my->_stats_once_is_enough = true;

   if(_use_timer)
      ilog("Using ${t}us deadline timer for checktime: " TIMER_STATS_FORMAT, TIMER_STATS);
   else
      wlog("Using polled checktime; deadline timer too inaccurate: " TIMER_STATS_FORMAT, TIMER_STATS);
}

}}