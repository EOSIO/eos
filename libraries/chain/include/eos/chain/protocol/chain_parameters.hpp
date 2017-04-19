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
#include <eos/chain/protocol/base.hpp>
#include <eos/chain/protocol/types.hpp>
#include <fc/smart_ref_fwd.hpp>

namespace eos { namespace chain {

   typedef static_variant<>  parameter_extension; 
   struct chain_parameters
   {
      uint32_t                maximum_block_size              = config::MaxBlockSize; ///< maximum allowable size in bytes for a block
      uint32_t                maximum_time_until_expiration   = config::MaxSecondsUntilExpiration; ///< maximum lifetime in seconds for transactions to be valid, before expiring

      /** defined in fee_schedule.cpp */
      void validate()const;
   };

} }  // eos::chain

FC_REFLECT( eos::chain::chain_parameters,
            (maximum_block_size)
            (maximum_time_until_expiration)
          )
