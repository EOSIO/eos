/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

namespace eosio { namespace chain {

template<typename T>
void shuffle(std::vector<T>& items, uint32_t seed) {
    auto hi = static_cast<uint64_t>(seed) << 32;
    auto s = items.size();
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
        std::swap(items[i], items[j]);
    }
}

}} // namespace eosio::chain

