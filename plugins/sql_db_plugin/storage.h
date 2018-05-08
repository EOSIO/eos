#ifndef STORAGE_H
#define STORAGE_H

#include <vector>
#include <eosio/chain/block.hpp>

namespace eosio {

template<typename T>
class storage {
public:
    virtual void store(const std::vector<T>& blocks) = 0;
};

} // namespace

#endif // STORAGE_H

