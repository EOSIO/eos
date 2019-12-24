#pragma once

#include <apifiny/chain/webassembly/apifiny-vm-oc/config.hpp>

#include <boost/asio/local/datagram_protocol.hpp>
#include <apifiny/chain/webassembly/apifiny-vm-oc/ipc_helpers.hpp>

namespace apifiny { namespace chain { namespace apifinyvmoc {

wrapped_fd get_connection_to_compile_monitor(int cache_fd);

}}}