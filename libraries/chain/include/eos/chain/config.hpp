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

namespace eos { namespace config {
const static char KeyPrefix[] = "EOS";

const static int BlockIntervalSeconds = 3;

/** Percentages are fixed point with a denominator of 10,000 */
const static int Percent100 = 10000;
const static int Percent1 = 100;

const static int MaxBlockSize = 256 * 1024;
const static int MaxSecondsUntilExpiration = 60*60;

const static int ProducerCount = 21;
const static int IrreversibleThresholdPercent = 70 * Percent1;
} } // namespace eos::config

template<typename Number>
Number EOS_PERCENT(Number value, int percentage) {
   return value * percentage / eos::config::Percent100;
}
