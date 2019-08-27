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
      result in an exception. Setting to nullptr disables any current set callback */
   void set_expiry_callback(void(*func)(void*), void* user) {
      EOS_ASSERT(!(func && __atomic_load_n(&_callback_enabled, __ATOMIC_CONSUME)), misc_exception, "Setting a platform_timer callback when one already exists");
      _expiration_callback = func;
      _expiration_callback_data = user;
      __atomic_store_n(&_callback_enabled, !!_expiration_callback, __ATOMIC_RELEASE);
   }

   volatile sig_atomic_t expired = 1;

private:
   struct impl;
   constexpr static size_t fwd_size = 8;
   fc::fwd<impl,fwd_size> my;

   void call_expiration_callback() {
      if(__atomic_load_n(&_callback_enabled, __ATOMIC_CONSUME)) {
         void(*cb)(void*) = _expiration_callback;
         void* cb_data = _expiration_callback_data;
         if(__atomic_load_n(&_callback_enabled, __ATOMIC_CONSUME))
            cb(cb_data);
      }
   }

   bool _callback_enabled = false;
   static_assert(__atomic_always_lock_free(sizeof(_callback_enabled), 0), "eosio requires an always lock free bool type");
   void(*_expiration_callback)(void*);
   void* _expiration_callback_data;
};

}}