#pragma once

namespace eosio::session {

template <typename Data_store>
class session;

template <typename Data_store>
session<Data_store> make_session();

template <typename Data_store>
session<Data_store> make_session(Data_store store);

template <typename Data_store>
session<Data_store> make_session(session<Data_store>& s);

} // namespace eosio::session
