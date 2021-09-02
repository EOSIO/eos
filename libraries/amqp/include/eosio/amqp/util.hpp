#pragma once

#include <amqpcpp.h>
#include <fc/variant.hpp>

namespace fc {
void to_variant(const AMQP::Address& a, fc::variant& v);
}
