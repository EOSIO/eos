#pragma once

namespace eosio { namespace chain {

struct platform_timer;
void compute_and_print_timer_accuracy(platform_timer& t);

}}