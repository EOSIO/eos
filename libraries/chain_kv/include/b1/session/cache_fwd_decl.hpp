#pragma once

namespace eosio::session {

template <typename Allocator>
class cache;

template <typename Allocator>
cache<Allocator> make_cache(std::shared_ptr<Allocator> a);

} // namespace eosio::session
