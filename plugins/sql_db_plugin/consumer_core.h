/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */

#pragma once

#include <vector>

namespace enumivo {

template<typename T>
class consumer_core {
public:
    virtual ~consumer_core() {}
    virtual void consume(const std::vector<T>& elements) = 0;
};

} // namespace


