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

namespace eos { namespace test {

template<typename Pred, typename Eval, typename T>
struct expector {
   expector(Eval _eval, const T& _expected, const char* const _msg)
      : expected(_expected)
      , eval(_eval)
      , msg(_msg)
   {}

   template<typename Input>
   void operator() (const Input& input) {
      BOOST_TEST_INFO(msg);
      auto actual = eval(input);
      BOOST_CHECK_EQUAL(Pred()(actual, expected), true);
   }

   T expected;
   Eval eval;
   char const * const msg;
};


template<typename Pred, typename Eval, typename T>
auto expect(Eval&& eval, T expected, char const * const msg) {
   return expector<Pred, Eval, T>(eval, expected, msg);
}

template<typename Eval, typename T>
auto expect(Eval&& eval, T expected, char const * const msg) {
   return expector<std::equal_to<T>, Eval, T>(eval, expected, msg);
}

template<typename Eval>
auto expect(Eval&& eval, char const * const msg) {
   return expector<std::equal_to<bool>, Eval, bool>(eval, true, msg);
}

#define _NUM_ARGS(A,B,C,N, ...) N
#define NUM_ARGS(...) _NUM_ARGS(__VA_ARGS__, 3, 2, 1)
#define EXPECT(...) EXPECT_(NUM_ARGS(__VA_ARGS__),__VA_ARGS__)
#define EXPECT_(N, ...) EXPECT__(N, __VA_ARGS__)
#define EXPECT__(N, ...) EXPECT_##N(__VA_ARGS__)
#define EXPECT_3(P, E, T) eos::test::expect<P>(E,T,"EXPECT<" #P ">(" #E "," #T ")")
#define EXPECT_2(E, T) eos::test::expect(E,T,"EXPECT(" #E "," #T ")")
#define EXPECT_1(E) eos::test::expect(E,"EXPECT(" #E ")")

}}