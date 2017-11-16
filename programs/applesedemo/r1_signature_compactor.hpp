#pragma once

#include <fc/crypto/elliptic_r1.hpp>
#include <fc/crypto/openssl.hpp>
#include <fc/crypto/sha256.hpp>

fc::crypto::r1::compact_signature compact_r1(fc::crypto::r1::public_key_data& pubkey, fc::ecdsa_sig& sig, fc::sha256& digest);