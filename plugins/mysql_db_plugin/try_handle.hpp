#pragma once

#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {

void handle(std::function<void()> handler, const std::string& desc);
void loop_handle(const std::atomic<bool>& done, const std::string& desc, std::function<void()> handler);

}
