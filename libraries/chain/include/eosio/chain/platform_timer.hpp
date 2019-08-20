#pragma once
#include <fc/time.hpp>
#include <fc/fwd.hpp>

#include <eosio/chain/exceptions.hpp>

#include <signal.h>

namespace eosio { namespace chain {

struct platform_timer {
   platform_timer();
   ~platform_timer();

   void start(fc::time_point tp);
   void stop();

   /* Sets a callback for when timer expires. Be aware this could might fire from a signal handling context and/or
      on any particular thread. Only a single callback can be registered at once; trying to register more will
      result in an exception. Setting to nullptr is allowed as a "no callback" hint */
   void set_expiry_callback(void(*func)(void*), void* user) {
      EOS_ASSERT(!(func && _expiration_callback), misc_exception, "Setting a platform_timer callback when one already exists");
      _expiration_callback = func;
      _expiration_callback_data = user;
   }

   volatile sig_atomic_t expired = 1;

private:
   struct impl;
   constexpr static size_t fwd_size = 8;
   fc::fwd<impl,fwd_size> my;

   void call_expiration_callback() {
      if(_expiration_callback)
         _expiration_callback(_expiration_callback_data);
   }

   void(*_expiration_callback)(void*) = nullptr;
   void* _expiration_callback_data;
};

}}