#pragma once
// declares types and unpackers before <fc/io/raw.hpp> included (to be visible by compiler)

namespace cyberway { namespace golos {

struct comment_object;

}}


namespace fc { namespace raw {

template<typename S> void unpack(S&, cyberway::golos::comment_object&);

}}
