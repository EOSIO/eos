/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

namespace eosio { namespace test {

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
#define EXPECT_3(P, E, T) eosio::test::expect<P>(E,T,"EXPECT<" #P ">(" #E "," #T ")")
#define EXPECT_2(E, T) eosio::test::expect(E,T,"EXPECT(" #E "," #T ")")
#define EXPECT_1(E) eosio::test::expect(E,"EXPECT(" #E ")")

}}