/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/chain_config.hpp>

#include <boost/range/algorithm/nth_element.hpp>

#include <fc/io/json.hpp>

namespace eosio { namespace chain {


template <typename T, typename Range>
struct properties_median_calculator_visitor {
   properties_median_calculator_visitor (T& medians, Range votes)
      : medians(medians), votes(votes)
   {}

   template <typename Member, class Class, Member (Class::*member)>
   void operator() (const char*) const {
      auto median_itr = boost::begin(votes) + boost::distance(votes)/2;
      boost::nth_element(votes, median_itr, [](const T& a, const T& b) { return a.*member < b.*member; });
      medians.*member = (*median_itr).*member;
   }

   T& medians;
   mutable Range votes;
};


template <typename T, typename Range>
properties_median_calculator_visitor<T, Range> get_median_properties_calculator(T& medians, Range&& votes) {
   return properties_median_calculator_visitor<T, Range>(medians, std::move(votes));
}

chain_config chain_config::get_median_values( vector<chain_config> votes) {
   chain_config results;
   fc::reflector<chain_config>::visit(get_median_properties_calculator(results, std::move(votes)));
   return results;
}

template <typename T>
struct comparison_visitor {
   const T& a;
   const T& b;

   template <typename Member, class Class, Member (Class::*member)>
   void operator() (const char*) const {
      if (a.*member != b.*member)
         // Throw to stop comparing fields: the structs are unequal
         throw false;
   }
};

bool operator==(const chain_config& a, const chain_config& b) {
   // Yes, it's gross, I'm using a boolean exception to direct normal control flow... that's why it's buried deep in an
   // implementation detail file. I think it's worth it for the generalization, though: this code keeps working no
   // matter what updates happen to chain_config
   //
   // TODO: this hack gives us short circuit evaluation when we could just use &= on a mutable variable and check at the
   // end, but that would require visiting all members rather than aborting on first fail.
   if (&a != &b) {
      try {
         fc::reflector<chain_config>::visit(comparison_visitor<chain_config>{a, b});
      } catch (bool) {
         return false;
      }
   }
   return true;
}

} } // namespace eosio::chain
