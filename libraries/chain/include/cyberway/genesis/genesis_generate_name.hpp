#pragma once
#include <eosio/chain/name.hpp>
#include <fc/crypto/sha1.hpp>

namespace cyberway { namespace genesis {

inline eosio::chain::name generate_name(std::string txt) {
    if (txt.length() == 0) return name();
    auto hash = fc::sha1::hash(txt);    // can implement something faster like MurmurHash3, but sha1 looks enough
    uint64_t data = ((uint64_t*)(hash.data()))[0];
    uint64_t r = 0;
    // names are base32 and 0 maps to '.' symbol. to avoid dots transform to base31 and increment each symbol
    for (int i = 0; i < 12; i++) {
        auto quot = data / 31;      // pair with % so compiler can produce 1 op for both div/mod
        auto rem = data % 31;
        data = quot;
        r |= (rem + 1) << (64 - 5 * (i + 1));
    }
    return eosio::chain::name(r);
}

}} // cyberway::genesis
