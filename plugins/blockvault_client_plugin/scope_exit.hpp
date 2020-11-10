#pragma once
#include <utility>

namespace eosio {
namespace blockvault {
template <typename F>
struct scope_exit {
 public:
   scope_exit(F&& f)
       : _f(f) {}
   scope_exit(const scope_exit&) = delete;
   scope_exit& operator=(const scope_exit&) = delete;
   ~scope_exit() { _f(); }

 private:
   F _f;
};

template <typename F>
scope_exit<F> make_scope_exit(F&& f) {
   return scope_exit<F>(std::forward<F>(f));
}
} // namespace blockvault
} // namespace eosio