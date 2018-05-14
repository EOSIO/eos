/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#ifndef RAND_HPP
#define RAND_HPP 1

#include <algorithm>

namespace eosio { namespace utilities { namespace rand {

/// High performance random generator
/// http://xorshift.di.unimi.it/

class random {
private:

    uint64_t seed;

public:

    random(uint64_t seed) {
        this->seed = seed;
    }

    uint64_t next() {
        uint64_t z = (seed += UINT64_C(0x9E3779B97F4A7C15));
        z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
        z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
        return z ^ (z >> 31);
    }

    template <typename Range>
    void shuffle(Range&& range) {
        int idx_count = range.size();
        for (auto idx = range.rbegin(); idx != range.rend() - 1; ++idx , --idx_count) {
            std::swap(range.at(next() % idx_count), *idx);
        }
    }
};

} } } //eosio::utilities::rand

#endif // RAND_HPP
