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

#include <eos/chain/get_config.hpp>
#include <eos/chain/config.hpp>
#include <eos/chain/protocol/types.hpp>

namespace eos { namespace chain {

fc::variant_object get_config()
{
   fc::mutable_variant_object result;

   result[ "EOS_KEY_PREFIX" ] = EOS_KEY_PREFIX;
   result[ "EOS_MIN_TRANSACTION_SIZE_LIMIT" ] = EOS_MIN_TRANSACTION_SIZE_LIMIT;
   result[ "EOS_MIN_BLOCK_INTERVAL" ] = EOS_MIN_BLOCK_INTERVAL;
   result[ "EOS_MAX_BLOCK_INTERVAL" ] = EOS_MAX_BLOCK_INTERVAL;
   result[ "EOS_DEFAULT_BLOCK_INTERVAL" ] = EOS_DEFAULT_BLOCK_INTERVAL;
   result[ "EOS_DEFAULT_MAX_TRANSACTION_SIZE" ] = EOS_DEFAULT_MAX_TRANSACTION_SIZE;
   result[ "EOS_DEFAULT_MAX_BLOCK_SIZE" ] = EOS_DEFAULT_MAX_BLOCK_SIZE;
   result[ "EOS_DEFAULT_MAX_TIME_UNTIL_EXPIRATION" ] = EOS_DEFAULT_MAX_TIME_UNTIL_EXPIRATION;
   result[ "EOS_DEFAULT_MAINTENANCE_INTERVAL" ] = EOS_DEFAULT_MAINTENANCE_INTERVAL;
   result[ "EOS_DEFAULT_MAINTENANCE_SKIP_SLOTS" ] = EOS_DEFAULT_MAINTENANCE_SKIP_SLOTS;
   result[ "EOS_MIN_UNDO_HISTORY" ] = EOS_MIN_UNDO_HISTORY;
   result[ "EOS_MAX_UNDO_HISTORY" ] = EOS_MAX_UNDO_HISTORY;
   result[ "EOS_MIN_BLOCK_SIZE_LIMIT" ] = EOS_MIN_BLOCK_SIZE_LIMIT;
   result[ "EOS_MIN_TRANSACTION_EXPIRATION_LIMIT" ] = EOS_MIN_TRANSACTION_EXPIRATION_LIMIT;
   result[ "EOS_100_PERCENT" ] = EOS_100_PERCENT;
   result[ "EOS_1_PERCENT" ] = EOS_1_PERCENT;
   result[ "EOS_DEFAULT_MAX_PRODUCERES" ] = EOS_DEFAULT_MAX_PRODUCERES;
   result[ "EOS_MAX_URL_LENGTH" ] = EOS_MAX_URL_LENGTH;
   result[ "EOS_NEAR_SCHEDULE_CTR_IV" ] = EOS_NEAR_SCHEDULE_CTR_IV;
   result[ "EOS_FAR_SCHEDULE_CTR_IV" ] = EOS_FAR_SCHEDULE_CTR_IV;
   result[ "EOS_CORE_ASSET_CYCLE_RATE" ] = EOS_CORE_ASSET_CYCLE_RATE;
   result[ "EOS_CORE_ASSET_CYCLE_RATE_BITS" ] = EOS_CORE_ASSET_CYCLE_RATE_BITS;

   return result;
}

} } // eos::chain
