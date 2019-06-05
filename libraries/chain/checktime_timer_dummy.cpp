#include <eosio/chain/checktime_timer.hpp>
#include <fc/fwd_impl.hpp>

namespace eosio { namespace chain {

struct checktime_timer::impl {};

checktime_timer::checktime_timer() {
   expired = 1;
}

checktime_timer::~checktime_timer() {}
void checktime_timer::start(fc::time_point tp) {}
void checktime_timer::stop() {}

}}