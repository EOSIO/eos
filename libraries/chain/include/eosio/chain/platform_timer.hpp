#pragma once
#include <fc/time.hpp>
#include <fc/fwd.hpp>
#include <fc/scoped_exit.hpp>

#include <eosio/chain/exceptions.hpp>

#include <atomic>

#include <signal.h>

namespace eosio { namespace chain {

struct platform_timer {
   platform_timer();
   ~platform_timer();

   void start(fc::time_point tp);
   void stop();

   /* Sets a callback for when timer expires. Be aware this could might fire from a signal handling context and/or
      on any particular thread. Only a single callback can be registered at once; trying to register more will
      result in an exception. Setting to nullptr disables any current set callback */
   void set_expiration_callback(void(*func)(void*), void* user) {
      bool expect_false = false;
      while(!atomic_compare_exchange_strong(&_callback_variables_busy, &expect_false, true))
         expect_false = false;
      auto reset_busy = fc::make_scoped_exit([this]() {
         _callback_variables_busy.store(false, std::memory_order_release);
      });
      EOS_ASSERT(!(func && _expiration_callback), misc_exception, "Setting a platform_timer callback when one already exists");

      _expiration_callback = func;
      _expiration_callback_data = user;
   }

   std::atomic_bool expired = true;

private:
   struct impl;
   constexpr static size_t fwd_size = 8;
   fc::fwd<impl,fwd_size> my;

   void call_expiration_callback() {
      bool expect_false = false;
      if(atomic_compare_exchange_strong(&_callback_variables_busy, &expect_false, true)) {
         void(*cb)(void*) = _expiration_callback;
         void* cb_data = _expiration_callback_data;
         if(cb) {
            cb(cb_data);
         }
         _callback_variables_busy.store(false, std::memory_order_release);
      }
   }

   std::atomic_bool _callback_variables_busy = false;
   void(*_expiration_callback)(void*) = nullptr;
   void* _expiration_callback_data;
};

}}
