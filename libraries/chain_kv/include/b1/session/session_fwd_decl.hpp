#pragma once

namespace eosio::session
{

template <typename Persistent_data_store, typename Cache_data_store>
class session;

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store> make_session();

template <typename Cache_data_store, typename Persistent_data_store>
session<Persistent_data_store, Cache_data_store> make_session(Persistent_data_store store);

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store> make_session(Persistent_data_store store, Cache_data_store cache);

template <typename Persistent_data_store, typename Cache_data_store>
session<Persistent_data_store, Cache_data_store> make_session(session<Persistent_data_store, Cache_data_store>& s);

}
