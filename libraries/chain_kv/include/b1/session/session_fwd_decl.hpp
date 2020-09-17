#pragma once

namespace eosio::session {

template <typename Parent>
class session;

template <typename Parent>
session<Parent> make_session();

template <typename Parent>
session<Parent> make_session(Parent& parent);

} // namespace eosio::session
