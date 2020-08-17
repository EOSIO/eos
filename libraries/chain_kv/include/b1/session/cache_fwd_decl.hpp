#pragma once

namespace eosio::session {

template <typename allocator>
class cache;

template <typename allocator>
cache<allocator> make_cache(std::shared_ptr<allocator> a);

}
