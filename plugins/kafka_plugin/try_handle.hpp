#pragma once

#include <eosio/chain_plugin/chain_plugin.hpp>

namespace kafka {

void handle(std::function<void()> handler, const std::string& desc);

}
