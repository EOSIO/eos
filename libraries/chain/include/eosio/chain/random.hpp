/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

namespace eosio { namespace chain {

template<typename RandomAccessIterator>
void shuffle(RandomAccessIterator first, RandomAccessIterator last, uint32_t seed) {
    auto s = std::distance(first, last);
    if (1 >= s) return;

    auto hi = static_cast<uint64_t>(seed) << 32;
    for (uint32_t i = 0; i < s; ++i) {
        /// High performance random generator
        /// http://xorshift.di.unimi.it/
        uint64_t k = hi + uint64_t(i) * 2685821657736338717ULL;
        k ^= (k >> 12);
        k ^= (k << 25);
        k ^= (k >> 27);
        k *= 2685821657736338717ULL;

        uint32_t jmax = s - i;
        uint32_t j = i + k % jmax;
        std::swap(*(first + i), *(first + j));
    }
}

template<typename T>
void shuffle(std::vector<T>& items, uint32_t seed) {
    shuffle(items.begin(), items.end(), seed);
}

}} // namespace eosio::chain

