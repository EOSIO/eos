#include <fc/thread/spin_lock.hpp>
#include <fc/time.hpp>
#include <boost/atomic.hpp>
#include <boost/memory_order.hpp>
#include <new>

namespace fc {
  #define define_self  boost::atomic<int>* self = (boost::atomic<int>*)&_lock
  spin_lock::spin_lock() 
  { 
     define_self;
     new (self) boost::atomic<int>();
     static_assert( sizeof(boost::atomic<int>) == sizeof(_lock), "" );
     self->store(unlocked);
  }

  bool spin_lock::try_lock() { 
    define_self;
    return self->exchange(locked, boost::memory_order_acquire)!=locked;
  }

  bool spin_lock::try_lock_for( const fc::microseconds& us ) {
    return try_lock_until( fc::time_point::now() + us );
  }

  bool spin_lock::try_lock_until( const fc::time_point& abs_time ) {
     while( abs_time > time_point::now() ) {
        if( try_lock() ) 
           return true;
     }
     return false;
  }

  void spin_lock::lock() {
    define_self;
    while( self->exchange(locked, boost::memory_order_acquire)==locked) { }
  }

  void spin_lock::unlock() {
     define_self;
     self->store(unlocked, boost::memory_order_release);
  }

  #undef define_self

} // namespace fc
