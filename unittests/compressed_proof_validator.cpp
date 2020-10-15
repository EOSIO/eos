#include "compressed_proof_validator.hpp"

#include <eosio/from_bin.hpp>
#include <eosio/ship_protocol.hpp>

#include <openssl/sha.h>

namespace eosio {

namespace proof_validator {

using sha256 = std::array<uint8_t, 32>;

//no copy sha256 input
struct sha256_input {
   constexpr size_t size() const { return sizeof(sha256); };

   const sha256& get() const {
      return *reinterpret_cast<const sha256*>(pos);
   }

   const char* pos;
};

template <typename S>
void from_bin(sha256_input& sha, S& stream) {
   sha.pos = stream.pos;
   return stream.skip(sha.size());
}

struct stream_range_capture {
   const char* start = nullptr;
   const char* end = nullptr;

   size_t size() const {
      check(start && end, "called stream_range_capture.size() without start and end");
      return end-start;
   }
};

template <typename S>
void from_bin(stream_range_capture& range, S& stream) {
   check(!range.start || !range.end, "stream_range_capture called too many times");
   if(range.start == nullptr)
      range.start = stream.pos;
   else
      range.end = stream.pos;
}

//used instead of ship_protocol::action so that the action base range can be easily capture
struct action {
   eosio::name                                  account       = {};
   eosio::name                                  name          = {};
   std::vector<ship_protocol::permission_level> authorization = {};
   eosio::input_stream                          data          = {};

   stream_range_capture action_base_range;
};
EOSIO_REFLECT(action, action_base_range, account, name, authorization, action_base_range, data);

//used instead of ship_protocol::action_receipt for efficient act_digest input
struct action_receipt {
   name                                              receiver        = {};
   sha256_input                                      act_digest      = {};
   uint64_t                                          global_sequence = {};
   uint64_t                                          recv_sequence   = {};
   std::vector<ship_protocol::account_auth_sequence> auth_sequence   = {};
   varuint32                                         code_sequence   = {};
   varuint32                                         abi_sequence    = {};
};
EOSIO_REFLECT(action_receipt, receiver, act_digest, global_sequence, recv_sequence, auth_sequence, code_sequence, abi_sequence)

struct action_entry {
   proof_validator::action         action;
   eosio::input_stream             return_value;
   proof_validator::action_receipt action_receipt;

   stream_range_capture action_range;
   stream_range_capture action_receipt_range;
};
EOSIO_REFLECT(action_entry, action_range, action, action_range, return_value, action_receipt_range, action_receipt, action_receipt_range);

struct combine {};
EOSIO_REFLECT(combine);

using merkle_cmd_t = std::variant<sha256_input, action_entry, combine>;

static sha256 action_hash_pre_rv(const action_entry& input) {
   sha256 ret;
   SHA256((const unsigned char*)input.action_range.start, input.action_range.size(), ret.data());
   return ret;
}

static sha256 action_hash_post_rv(const action_entry& input) {
   sha256 hashpair[2];
   SHA256((const unsigned char*)input.action.action_base_range.start, input.action.action_base_range.size(), hashpair[0].data());
   SHA256((const unsigned char*)input.action.action_base_range.end, input.return_value.end - input.action.action_base_range.end, hashpair[1].data());
   SHA256(hashpair->data(), sizeof(hashpair), hashpair[0].data());
   return hashpair[0];
}

static sha256 validate_single_action(bool return_value_active, const action_entry& input, std::function<void(uint64_t, uint64_t)> on_action_receiver) {
   const sha256 action_hash = return_value_active ? action_hash_post_rv(input) : action_hash_pre_rv(input);

   check(input.action_receipt.act_digest.get() == action_hash, "action digest in action receipt does not match computed action digest");

   //For a real validation application, you would do something more "interesting" here. In this example code,
   // just make a call out to a function with the receiver & action as a uint64_t so that it can be validated. Anything more
   // advanced is tricky with abieos vs fc/chain together.
   on_action_receiver(input.action_receipt.receiver.value, input.action.name.value);

   sha256 ret;
   SHA256((const unsigned char*)input.action_receipt_range.start, input.action_receipt_range.size(), ret.data());
   return ret;
}

}

using namespace proof_validator;

fc::sha256 validate_compressed_merkle_proof(const std::vector<char>& input, std::function<void(uint64_t, uint64_t)> on_action_receiver) {
   input_stream is(input);
   bool arv_activated;
   unsigned_int num_cmds;
   from_bin(arv_activated, is);
   from_bin(num_cmds, is);

   //Used stack size will be the same as the merkle tree depth. So something like 24 is vastly overkill
   // because you're not going to have 2^24 actions in a block any time soon.
   const unsigned merkle_stack_size = 24;
   sha256* merkle_stack = (sha256*)alloca(sizeof(sha256)*merkle_stack_size);
   unsigned merkle_stack_top = 0;

   merkle_cmd_t cmd;
   while(num_cmds.value--) {
      from_bin(cmd, is);

      if(const action_entry* act_input = std::get_if<action_entry>(&cmd)) {
         check(merkle_stack_top != merkle_stack_size, "merkle stack is full");
         merkle_stack[merkle_stack_top++] = validate_single_action(arv_activated, *act_input, on_action_receiver);
      }
      else if(const sha256_input* hash_input = std::get_if<sha256_input>(&cmd)) {
         check(merkle_stack_top != merkle_stack_size, "merkle stack is full");
         merkle_stack[merkle_stack_top++] = hash_input->get();
      }
      else if(std::holds_alternative<combine>(cmd)) {
         check(merkle_stack_top >= 2, "merkle stack combine operator found when stack size less than 2");
         merkle_stack[merkle_stack_top-2][0] &= 0x7fu;
         merkle_stack[merkle_stack_top-1][0] |= 0x80u;
         SHA256(merkle_stack[merkle_stack_top-2].data(), sizeof(sha256)*2, merkle_stack[merkle_stack_top-2].data());
         merkle_stack_top--;
      }
   }

   return fc::sha256((const char*)merkle_stack[0].data(), sizeof(sha256));
}

}