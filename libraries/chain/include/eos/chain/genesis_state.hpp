/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <eos/chain/protocol/chain_parameters.hpp>
#include <eos/chain/protocol/types.hpp>
#include <eos/chain/immutable_chain_parameters.hpp>

#include <fc/crypto/sha256.hpp>

#include <string>
#include <vector>

namespace eos { namespace chain {
using std::string;
using std::vector;

struct genesis_state_type {
   struct initial_account_type {
      initial_account_type(const string& name = string(),
                           const public_key_type& owner_key = public_key_type(),
                           const public_key_type& active_key = public_key_type())
         : name(name),
           owner_key(owner_key),
           active_key(active_key == public_key_type()? owner_key : active_key)
      {}
      string name;
      public_key_type owner_key;
      public_key_type active_key;
   };
   struct initial_producer_type {
      initial_producer_type(const string& name = string(),
                            const public_key_type& signing_key = public_key_type())
         : owner_name(name), block_signing_key(signing_key)
      {}
      /// Must correspond to one of the initial accounts
      string owner_name;
      public_key_type block_signing_key;
   };

   time_point_sec                           initial_timestamp;
   chain_parameters                         initial_parameters;
   immutable_chain_parameters               immutable_parameters;
   vector<initial_account_type>             initial_accounts;
   vector<initial_producer_type>            initial_producers;
   uint64_t                                 initial_producer_count = EOS_DEFAULT_MIN_PRODUCER_COUNT;

   /**
    * Temporary, will be moved elsewhere.
    */
   chain_id_type                            initial_chain_id;

   /**
    * Get the chain_id corresponding to this genesis state.
    *
    * This is the SHA256 serialization of the genesis_state.
    */
   chain_id_type compute_chain_id() const;
};

} } // namespace eos::chain

FC_REFLECT(eos::chain::genesis_state_type::initial_account_type, (name)(owner_key)(active_key))

FC_REFLECT(eos::chain::genesis_state_type::initial_producer_type, (owner_name)(block_signing_key))

FC_REFLECT(eos::chain::genesis_state_type,
           (initial_timestamp)(initial_parameters)(immutable_parameters)(initial_accounts)
           (initial_producers)(initial_producer_count)(initial_chain_id))
