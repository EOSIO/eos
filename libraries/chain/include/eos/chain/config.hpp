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

#define EOS_KEY_PREFIX "GPH"

/**
 * Don't allow the committee_members to publish a limit that would
 * make the network unable to operate.
 */
#define EOS_MIN_TRANSACTION_SIZE_LIMIT 1024
#define EOS_MIN_BLOCK_INTERVAL   1 /* seconds */
#define EOS_MAX_BLOCK_INTERVAL  30 /* seconds */

#define EOS_DEFAULT_BLOCK_INTERVAL  5 /* seconds */
#define EOS_DEFAULT_MAX_TRANSACTION_SIZE 2048
#define EOS_DEFAULT_MAX_BLOCK_SIZE  (EOS_DEFAULT_MAX_TRANSACTION_SIZE*EOS_DEFAULT_BLOCK_INTERVAL*200000)
#define EOS_DEFAULT_MAX_TIME_UNTIL_EXPIRATION (60*60*24) // seconds,  aka: 1 day
#define EOS_DEFAULT_MAINTENANCE_INTERVAL  (60*60*24) // seconds, aka: 1 day
#define EOS_DEFAULT_MAINTENANCE_SKIP_SLOTS 3  // number of slots to skip for maintenance interval

#define EOS_MIN_UNDO_HISTORY 10
#define EOS_MAX_UNDO_HISTORY 10000

#define EOS_MIN_BLOCK_SIZE_LIMIT (EOS_MIN_TRANSACTION_SIZE_LIMIT*5) // 5 transactions per block
#define EOS_MIN_TRANSACTION_EXPIRATION_LIMIT (EOS_MAX_BLOCK_INTERVAL * 5) // 5 transactions per block

/** percentage fields are fixed point with a denominator of 10,000 */
#define EOS_100_PERCENT                                    10000
#define EOS_1_PERCENT                                      (EOS_100_PERCENT/100)

#define EOS_DEFAULT_MIN_PRODUCER_COUNT                    (11)
#define EOS_DEFAULT_MAX_PRODUCERES                        (1001) // SHOULD BE ODD

#define EOS_MAX_URL_LENGTH                               127

// counter initialization values used to derive near and far future seeds for shuffling produceres
// we use the fractional bits of sqrt(2) in hex
#define EOS_NEAR_SCHEDULE_CTR_IV                    ( (uint64_t( 0x6a09 ) << 0x30)    \
                                                         | (uint64_t( 0xe667 ) << 0x20)    \
                                                         | (uint64_t( 0xf3bc ) << 0x10)    \
                                                         | (uint64_t( 0xc908 )        ) )

// and the fractional bits of sqrt(3) in hex
#define EOS_FAR_SCHEDULE_CTR_IV                     ( (uint64_t( 0xbb67 ) << 0x30)    \
                                                         | (uint64_t( 0xae85 ) << 0x20)    \
                                                         | (uint64_t( 0x84ca ) << 0x10)    \
                                                         | (uint64_t( 0xa73b )        ) )

/**
 * every second, the fraction of burned core asset which cycles is
 * EOS_CORE_ASSET_CYCLE_RATE / (1 << EOS_CORE_ASSET_CYCLE_RATE_BITS)
 */
#define EOS_CORE_ASSET_CYCLE_RATE                        17
#define EOS_CORE_ASSET_CYCLE_RATE_BITS                   32

#define EOS_DEFAULT_PRODUCER_PAY_PER_BLOCK            (EOS_BLOCKCHAIN_PRECISION * int64_t( 10) )

#define EOS_RECENTLY_MISSED_COUNT_INCREMENT             4
#define EOS_RECENTLY_MISSED_COUNT_DECREMENT             3

#define EOS_CURRENT_DB_VERSION                          "GPH0.1"

#define EOS_IRREVERSIBLE_THRESHOLD                      (70 * EOS_1_PERCENT)
