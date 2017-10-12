/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <string>
#include <fc/crypto/elliptic.hpp>
#include <fc/optional.hpp>

namespace eos { namespace utilities {

std::string                        key_to_wif(const fc::sha256& private_secret );
std::string                        key_to_wif(const fc::ecc::private_key& key);
fc::optional<fc::ecc::private_key> wif_to_key( const std::string& wif_key );

} } // end namespace eos::utilities
