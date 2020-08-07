#pragma once

namespace b1::session
{

template <typename allocator>
class cache;

template <typename allocator>
auto make_cache(std::shared_ptr<allocator> a) -> cache<allocator>;

}
