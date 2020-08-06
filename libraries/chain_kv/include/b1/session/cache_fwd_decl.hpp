#ifndef cache_data_store_fwd_decl_h
#define cache_data_store_fwd_decl_h

namespace b1::session
{

template <typename allocator>
class cache;

template <typename allocator>
auto make_cache(std::shared_ptr<allocator> a) -> cache<allocator>;

}

#endif /* cache_data_store_fwd_decl_h */
