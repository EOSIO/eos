#pragma once
#include <memory>

namespace eosio::witness_plugin_wrappers {

struct wrapped_shared_ptr_base {
   virtual ~wrapped_shared_ptr_base() {}
   virtual bool valid() const = 0;
};

struct wrapped_weak_ptr_base {
   virtual ~wrapped_weak_ptr_base() {}
   virtual std::shared_ptr<wrapped_shared_ptr_base> lock() = 0;
};

template<typename T>
struct wrapped_shared_ptr : wrapped_shared_ptr_base {
   wrapped_shared_ptr(std::shared_ptr<T> shared) : _shared(shared){}
   bool valid() const override {
      return !!_shared;
   }
private:
   std::shared_ptr<T> _shared;
};

template<typename T>
struct wrapped_weak_ptr : wrapped_weak_ptr_base {
   wrapped_weak_ptr(std::weak_ptr<T> weak) : _weak(weak) {}
   std::shared_ptr<wrapped_shared_ptr_base> lock() override {
      return std::make_shared<wrapped_shared_ptr<T>>(_weak.lock());
   }

private:
   std::weak_ptr<T> _weak;
};

}