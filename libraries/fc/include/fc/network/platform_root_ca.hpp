/**
 *  @file
 *  @copyright defined in LICENSE.txt
 */
#pragma once

#include <boost/asio/ssl/context.hpp>

namespace fc {

//Add the platform's trusted root CAs to the ssl context
void add_platform_root_cas_to_context(boost::asio::ssl::context& ctx);
}