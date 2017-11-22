/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <boost/test/unit_test.hpp>

#include <eos/chain/block_schedule.hpp>
#include "../common/expect.hpp"

using namespace eosio;
using namespace chain;

/*
 * templated meta schedulers for fuzzing 
 */
template<typename NEXT>
struct shuffled_functor {
   shuffled_functor(NEXT &&_next) : next(_next){};

   block_schedule operator()(const vector<pending_transaction>& transactions, const global_property_object& properties) {
      std::random_device rd;
      std::mt19937 rng(rd());
      auto copy = std::vector<pending_transaction>(transactions);
      std::shuffle(copy.begin(), copy.end(), rng);
      return next(copy, properties);
   }

   NEXT &&next;
};

template<typename NEXT> 
static shuffled_functor<NEXT> shuffled(NEXT &&next) {
   return shuffled_functor<NEXT>(next);
}

template<int NUM, int DEN, typename NEXT>
struct lossy_functor {
   lossy_functor(NEXT &&_next) : next(_next){};
   
   block_schedule operator()(const vector<pending_transaction>& transactions, const global_property_object& properties) {
      std::random_device rd;
      std::mt19937 rng(rd());
      std::uniform_real_distribution<> dist(0, 1);
      double const cutoff = (double)NUM / (double)DEN;

      auto copy = std::vector<pending_transaction>();
      copy.reserve(transactions.size());
      std::copy_if (transactions.begin(), transactions.end(), copy.begin(), [&](const pending_transaction& trx){
         return dist(rng) >= cutoff;
      });

      return next(copy, properties);
   }

   NEXT &&next;
};

template<int NUM, int DEN, typename NEXT> 
static lossy_functor<NUM, DEN, NEXT> lossy(NEXT &&next) {
   return lossy_functor<NUM, DEN, NEXT>(next);
}

/*
 * Policy based Fixtures for chain properties
 */
class common_fixture {
public:
   struct test_transaction {
      test_transaction(const std::initializer_list<account_name>& _scopes)
         : scopes(_scopes)
      {
      }

      const std::initializer_list<account_name>& scopes;
   };

protected:
   auto create_transactions( const std::initializer_list<test_transaction>& transactions ) {
      std::vector<signed_transaction> result;
      for (const auto& t: transactions) {
         signed_transaction st;
         st.scope.reserve(t.scopes.size());
         st.scope.insert(st.scope.end(), t.scopes.begin(), t.scopes.end());
         result.emplace_back(st);
      }
      return result;
   }

   auto create_pending( const std::vector<signed_transaction>& transactions ) {
      std::vector<pending_transaction> result;
      for (const auto& t: transactions) {
         result.emplace_back(std::reference_wrapper<signed_transaction const> {t});
      }
      return result;
   }
};

template<typename PROPERTIES_POLICY>
class compose_fixture: public common_fixture {
public:
   template<typename SCHED_FN, typename ...VALIDATORS>
   void schedule_and_validate(SCHED_FN sched_fn, const std::initializer_list<test_transaction>& transactions, VALIDATORS ...validators) {
      try {
         auto signed_transactions = create_transactions(transactions);
         auto pending = create_pending(signed_transactions);
         auto schedule = sched_fn(pending, properties_policy.properties);
         validate(schedule, validators...);
      } FC_LOG_AND_RETHROW()
   }

private:
   template<typename VALIDATOR>
   void validate(const block_schedule& schedule, VALIDATOR validator) {
      validator(schedule);
   }

   template<typename VALIDATOR, typename ...VALIDATORS>
   void validate(const block_schedule& schedule, VALIDATOR validator, VALIDATORS ... others) {
      validate(schedule, validator);
      validate(schedule, others...);
   }


   PROPERTIES_POLICY properties_policy;
};

static void null_global_property_object_constructor(const global_property_object& )
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
      properties.configuration.max_blk_size = 256 * 1024;
   }
};

struct small_block_properties : public base_properties {
   small_block_properties() {
      properties.configuration.max_blk_size = 512;
   }
};

typedef compose_fixture<default_properties> default_fixture;

/*
 * Evaluators for expect
 */
static uint transaction_count(const block_schedule& schedule) {
   uint result = 0;
   for (const auto& c : schedule.cycles) {
      for (const auto& t: c) {
         result += t.transactions.size();
      }
   }

   return result;
}

static uint cycle_count(const block_schedule& schedule) {
   return schedule.cycles.size();
}



static bool schedule_is_valid(const block_schedule& schedule) {
   for (const auto& c : schedule.cycles) {
      std::vector<bool> scope_in_use;
      for (const auto& t: c) {
         std::set<size_t> thread_bits;
         size_t max_bit = 0;
         for(const auto& pt: t.transactions) {
            auto scopes = pt.visit(scope_extracting_visitor());
            for (const auto& s : scopes) {
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

BOOST_FIXTURE_TEST_CASE(no_conflicts_shuffled, default_fixture) {
   // stochastically verify that the order of non-conflicting transactions
   // does not affect the ability to schedule them in a single cycle
   for (int i = 0; i < 3000; i++) {      
      schedule_and_validate(
         shuffled(block_schedule::by_threading_conflicts),
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
         EXPECT(transaction_count, 21),
         EXPECT(cycle_count, 1)
      );
   }
}

BOOST_FIXTURE_TEST_CASE(some_conflicts_shuffled, default_fixture) {
   // stochastically verify that the order of conflicted transactions
   // does not affect the ability to schedule them in multiple cycles
   for (int i = 0; i < 3000; i++) {      
      schedule_and_validate(
         shuffled(block_schedule::by_threading_conflicts),
         {
            {0x1ULL, 0x2ULL},
            {0x3ULL, 0x2ULL},
            {0x5ULL, 0x1ULL},
            {0x7ULL, 0x1ULL},
            {0x1ULL, 0x7ULL},
            {0x11ULL, 0x12ULL},
            {0x13ULL, 0x12ULL},
            {0x15ULL, 0x11ULL},
            {0x17ULL, 0x11ULL},
            {0x11ULL, 0x17ULL},
            {0x21ULL, 0x22ULL},
            {0x23ULL, 0x22ULL},
            {0x25ULL, 0x21ULL},
            {0x27ULL, 0x21ULL},
            {0x21ULL, 0x27ULL}
         },
         EXPECT(schedule_is_valid),
         EXPECT(transaction_count, 15),
         EXPECT(std::greater<uint>, cycle_count, 1)
      );
   }
}

BOOST_AUTO_TEST_SUITE_END()