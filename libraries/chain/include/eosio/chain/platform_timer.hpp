#pragma once
#include <fc/time.hpp>
#include <fc/fwd.hpp>

#include <signal.h>

namespace eosio { namespace chain {

struct platform_timer {
   platform_timer();
   ~platform_timer();

   void start(fc::time_point tp);
   void stop();

   volatile sig_atomic_t expired = 1;

private:
   struct impl;
   constexpr static size_t fwd_size = 8;
   fc::fwd<impl,fwd_size> my;
};

}}