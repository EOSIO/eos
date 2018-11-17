#include "random.hpp"
#include "random.h"


namespace eosio {

    // Warning: It should be noted that the potential risk point of the random number scheme is that a block node may cheat.
    // For DAPP developers, they can provide random parameters and then combine them to make it safer and more reliable.
    std::unique_ptr<std::seed_seq> seed_timestamp_txid_signed() {
        size_t szBuff = sizeof(signature)+4;
        char buf[szBuff];
        memset(buf,0,szBuff);
        size_t size = producer_random_seed(buf, sizeof(buf));
        eosio_assert(size > 0 && size <= sizeof(buf), "buffer is too small");
        if (size % 4 > 0)
        {
            size = (size/4 + 1)*4; // add 0000, Guaranteed digital signature completion
        }
        uint32_t* seq = reinterpret_cast<uint32_t*>(buf);
        return std::make_unique<std::seed_seq>(seq, seq + size/4);
    }

    std::minstd_rand0 minstd_rand0(const seed_seq_ptr& seed) {
        return std::minstd_rand0(*seed);
    }
    std::minstd_rand minstd_rand(const seed_seq_ptr& seed) {
        return std::minstd_rand(*seed);
    }
    std::mt19937 mt19937(const seed_seq_ptr& seed) {
        return std::mt19937(*seed);
    }
    std::mt19937_64 mt19937_64(const seed_seq_ptr& seed) {
        return std::mt19937_64(*seed);
    }
    std::ranlux24_base ranlux24_base(const seed_seq_ptr& seed) {
        return std::ranlux24_base(*seed);
    }
    std::ranlux48_base ranlux48_base(const seed_seq_ptr& seed) {
        return std::ranlux48_base(*seed);
    }
    std::ranlux24 ranlux24(const seed_seq_ptr& seed) {
        return std::ranlux24(*seed);
    }
    std::ranlux48 ranlux48(const seed_seq_ptr& seed) {
        return std::ranlux48(*seed);
    }
    std::knuth_b knuth_b(const seed_seq_ptr& seed) {
        return std::knuth_b(*seed);
    }

}
