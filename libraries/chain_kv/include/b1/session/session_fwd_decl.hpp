#pragma once

namespace eosio::session
{

template <typename persistent_data_store, typename cache_data_store>
class session;

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store> make_session();

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store> make_session(persistent_data_store store);

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store> make_session(persistent_data_store store, cache_data_store cache);

template <typename persistent_data_store, typename cache_data_store>
session<persistent_data_store, cache_data_store> make_session(session<persistent_data_store, cache_data_store>& s);

}
