#include <fc/crypto/sha256.hpp>

namespace eosio {

fc::sha256 validate_compressed_merkle_proof(const std::vector<char>& input, std::function<void(uint64_t)> on_action_receiver);

}