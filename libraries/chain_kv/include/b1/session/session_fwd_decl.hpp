#ifndef session_fwd_decl_h
#define session_fwd_decl_h

namespace b1::session
{

template <typename persistent_data_store, typename cache_data_store>
class session;

template <typename persistent_data_store, typename cache_data_store>
auto make_session() -> session<persistent_data_store, cache_data_store>;

template <typename persistent_data_store, typename cache_data_store>
auto make_session(persistent_data_store store) -> session<persistent_data_store, cache_data_store>;

template <typename persistent_data_store, typename cache_data_store>
auto make_session(persistent_data_store store, cache_data_store cache) -> session<persistent_data_store, cache_data_store>;

template <typename persistent_data_store, typename cache_data_store>
auto make_session(session<persistent_data_store, cache_data_store>& s) -> session<persistent_data_store, cache_data_store>;

}

#endif /* session_fwd_decl_h */
