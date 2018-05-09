#ifndef STORAGE_H
#define STORAGE_H

#include <vector>
#include <eosio/chain/block.hpp>

namespace eosio {

template<typename T>
class consumer_core {
public:
    virtual ~consumer_core() {}
    virtual void consume(const std::vector<T>& blocks) = 0;
};

} // namespace

#endif // STORAGE_H

