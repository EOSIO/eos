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

#include <boost/test/unit_test.hpp>

#include <eos/chain/block_schedule.hpp>
#include "../common/expect.hpp"

using namespace eos;
using namespace chain;

/*
 * Policy based Fixtures for chain properties
 */
class common_fixture {
public:
   struct test_transaction {
      test_transaction(std::initializer_list<AccountName> const &_scopes)
         : scopes(_scopes)
      {
      }

      std::initializer_list<AccountName> const &scopes;
   };

protected:
   auto create_transactions( std::initializer_list<test_transaction> const &transactions ) {
      std::vector<SignedTransaction> result;
      for (auto const &t: transactions) {
         SignedTransaction st;
         st.scope.reserve(t.scopes.size());
         st.scope.insert(st.scope.end(), t.scopes.begin(), t.scopes.end());
         result.emplace_back(st);
      }
      return result;
   }

   auto create_pending( std::vector<SignedTransaction> const &transactions ) {
      std::vector<pending_transaction> result;
      for (auto const &t: transactions) {
         auto const *ptr = &t;
         result.emplace_back(ptr);
      }
      return result;
   }
};

template<typename PROPERTIES_POLICY>
class compose_fixture: public common_fixture {
public:
   template<typename SCHED_FN, typename ...VALIDATORS>
   void schedule_and_validate(SCHED_FN sched_fn, std::initializer_list<test_transaction> const &transactions, VALIDATORS ...validators) {
      try {
         auto signed_transactions = create_transactions(transactions);
         auto pending = create_pending(signed_transactions);
         auto schedule = sched_fn(pending, properties_policy.properties);
         validate(schedule, validators...);
      } FC_LOG_AND_RETHROW()
   }

private:
   template<typename VALIDATOR>
   void validate(block_schedule const &schedule, VALIDATOR validator) {
      validator(schedule);
   }

   template<typename VALIDATOR, typename ...VALIDATORS>
   void validate(block_schedule const &schedule, VALIDATOR validator, VALIDATORS ... others) {
      validate(schedule, validator);
      validate(schedule, others...);
   }


   PROPERTIES_POLICY properties_policy;
};

static void null_global_property_object_constructor(global_property_object const &)
{}

static chainbase::allocator<global_property_object> null_global_property_object_allocator(nullptr);

struct base_properties {
   base_properties() 
      : properties(null_global_property_object_constructor, null_global_property_object_allocator)
   {
   }

   global_property_object properties;
};

struct default_properties : public base_properties {
   default_properties() {
      properties.configuration.maxBlockSize = 256 * 1024;
   }
};

struct small_block_properties : public base_properties {
   small_block_properties() {
      properties.configuration.maxBlockSize = 512;
   }
};

typedef compose_fixture<default_properties> default_fixture;

/*
 * Evaluators for expect
 */
static uint transaction_count(block_schedule const &schedule) {
   uint result = 0;
   for (auto const &c : schedule.cycles) {
      for (auto const &t: c) {
         result += t.transactions.size();
      }
   }

   return result;
}

static uint cycle_count(block_schedule const &schedule) {
   return schedule.cycles.size();
}



static bool schedule_is_valid(block_schedule const &schedule) {
   for (auto const &c : schedule.cycles) {
      std::vector<bool> scope_in_use;
      for (auto const &t: c) {
         std::set<size_t> thread_bits;
         size_t max_bit = 0;
         for(auto pt: t.transactions) {
            auto scopes = pt->visit(scope_extracting_visitor());
            for (auto const &s : scopes) {
               size_t bit = boost::numeric_cast<size_t>((uint64_t)s);
               thread_bits.emplace(bit);
               max_bit = std::max(max_bit, bit);
            }
         }

         if (scope_in_use.size() <= max_bit) {
            scope_in_use.resize(max_bit + 1);
         }

         for ( auto b: thread_bits ) {
            if(scope_in_use.at(b)) {
               return false;
            };
            scope_in_use.at(b) = true;
         }
      }
   }

   return true;
}

/*
 * Test Cases
 */

BOOST_AUTO_TEST_SUITE(block_schedule_tests)

BOOST_FIXTURE_TEST_CASE(no_conflicts, default_fixture) {
   // ensure the scheduler can handle basic non-conflicted transactions
   // in a single cycle
   schedule_and_validate(
      block_schedule::by_threading_conflicts,
      {
         {0x1ULL, 0x2ULL},
         {0x3ULL, 0x4ULL},
         {0x5ULL, 0x6ULL},
         {0x7ULL, 0x8ULL},
         {0x9ULL, 0xAULL}
      },
      EXPECT(schedule_is_valid),
      EXPECT(transaction_count, 5),
      EXPECT(cycle_count, 1)
   );
}

BOOST_FIXTURE_TEST_CASE(some_conflicts, default_fixture) {
   // ensure the scheduler can handle conflicted transactions
   // using multiple cycles
   schedule_and_validate(
      block_schedule::by_threading_conflicts,
      {
         {0x1ULL, 0x2ULL},
         {0x3ULL, 0x2ULL},
         {0x5ULL, 0x1ULL},
         {0x7ULL, 0x1ULL},
         {0x1ULL, 0x7ULL}
      },
      EXPECT(schedule_is_valid),
      EXPECT(transaction_count, 5),
      EXPECT(std::greater<uint>, cycle_count, 1)
   );
}

BOOST_FIXTURE_TEST_CASE(basic_cycle, default_fixture) {
   // ensure the scheduler can handle a basic scope cycle
   schedule_and_validate(
      block_schedule::by_threading_conflicts,
      {
         {0x1ULL, 0x2ULL},
         {0x2ULL, 0x3ULL},
         {0x3ULL, 0x1ULL},
      },
      EXPECT(schedule_is_valid),
      EXPECT(transaction_count, 3)
   );
}

BOOST_FIXTURE_TEST_CASE(small_block, compose_fixture<small_block_properties>) {
   // ensure the scheduler can handle basic block size restrictions
   schedule_and_validate(
      block_schedule::by_threading_conflicts,
      {
         {0x1ULL, 0x2ULL},
         {0x3ULL, 0x4ULL},
         {0x5ULL, 0x6ULL},
         {0x7ULL, 0x8ULL},
         {0x9ULL, 0xAULL},
         {0xBULL, 0xCULL},
         {0xDULL, 0xEULL},
         {0x11ULL, 0x12ULL},
         {0x13ULL, 0x14ULL},
         {0x15ULL, 0x16ULL},
         {0x17ULL, 0x18ULL},
         {0x19ULL, 0x1AULL},
         {0x1BULL, 0x1CULL},
         {0x1DULL, 0x1EULL},
         {0x21ULL, 0x22ULL},
         {0x23ULL, 0x24ULL},
         {0x25ULL, 0x26ULL},
         {0x27ULL, 0x28ULL},
         {0x29ULL, 0x2AULL},
         {0x2BULL, 0x2CULL},
         {0x2DULL, 0x2EULL},
      },
      EXPECT(schedule_is_valid),
      EXPECT(std::less<uint>, transaction_count, 21)
   );
}

BOOST_AUTO_TEST_SUITE_END()