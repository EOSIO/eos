#pragma once
#include <fc/time.hpp>
#include <fc/fwd.hpp>

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
      callback_state current_state;
      while((current_state = _callback_state.load(std::memory_order_acquire)) == READING) {} //wait for call_expiration_callback() to be done with it
      if(func)
         EOS_ASSERT(current_state == UNSET, misc_exception, "Setting a platform_timer callback when one already exists");
      if(!func && current_state == UNSET)
         return;

      //prepare for loading in new values by moving from UNSET->WRITING
      while(!atomic_compare_exchange_strong(&_callback_state, &current_state, WRITING))
         current_state = UNSET;
      _expiration_callback = func;
      _expiration_callback_data = user;

      //finally go WRITING->SET
      _callback_state.store(func ? SET : UNSET, std::memory_order_release);
   }

   volatile sig_atomic_t expired = 1;

private:
   struct impl;
   constexpr static size_t fwd_size = 8;
   fc::fwd<impl,fwd_size> my;

   enum callback_state {
      UNSET,
      READING,
      WRITING,
      SET
   };

   void call_expiration_callback() {
      callback_state state_is_SET = SET;
      if(atomic_compare_exchange_strong(&_callback_state, &state_is_SET, READING)) {
         void(*cb)(void*) = _expiration_callback;
         void* cb_data = _expiration_callback_data;
         _callback_state.store(SET, std::memory_order_release);
         cb(cb_data);
      }
   }

   std::atomic<callback_state> _callback_state = UNSET;
   void(*_expiration_callback)(void*);
   void* _expiration_callback_data;
};

}}