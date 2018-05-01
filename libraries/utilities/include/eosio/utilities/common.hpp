/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *
 */
#pragma once

#ifndef COMMON_HPP
#define COMMON_HPP

namespace eosio { namespace utilities { namespace common {
  template<typename I>
  std::string itoh(I n, size_t hlen = sizeof(I)<<1) {
     static const char* digits = "0123456789abcdef";
     std::string r(hlen, '0');
     for(size_t i = 0, j = (hlen - 1) * 4 ; i < hlen; ++i, j -= 4)
        r[i] = digits[(n>>j) & 0x0f];
     return r;
  }
}}}

#endif // COMMON_HPP
