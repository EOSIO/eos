#pragma once

#include <random>
#include <memory>

namespace eosio {

    using seed_seq_ptr = std::unique_ptr<std::seed_seq>;
    /* Get a fair random number seed */
    seed_seq_ptr seed_timestamp_txid_signed();

    std::minstd_rand0 minstd_rand0(const seed_seq_ptr& seed);
    std::minstd_rand minstd_rand(const seed_seq_ptr& seed);
    std::mt19937 mt19937(const seed_seq_ptr& seed);
    std::mt19937_64 mt19937_64(const seed_seq_ptr& seed);
    std::ranlux24_base ranlux24_base(const seed_seq_ptr& seed);
    std::ranlux48_base ranlux48_base(const seed_seq_ptr& seed);
    std::ranlux24 ranlux24(const seed_seq_ptr& seed);
    std::ranlux48 ranlux48(const seed_seq_ptr& seed);
    std::knuth_b knuth_b(const seed_seq_ptr& seed);

}
