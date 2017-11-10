/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/chain/blockchain_configuration.hpp>

#include <boost/range/algorithm/nth_element.hpp>

#include <fc/io/json.hpp>

namespace eosio { namespace chain {

template <typename T>
struct assign_visitor {
   T& a;
   const T& b;

   template <typename Member, class Class, Member (Class::*member)>
   void operator() (const char*) const {
      a.*member = b.*member;
   }
};

blockchain_configuration& blockchain_configuration::operator=(const types::blockchain_configuration& other) {
   if (&other != this)
      fc::reflector<blockchain_configuration>::visit(assign_visitor<types::blockchain_configuration>{*this, other});
   return *this;
}

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

blockchain_configuration blockchain_configuration::get_median_values(
      std::vector<blockchain_configuration> votes) {
   blockchain_configuration results;
   fc::reflector<blockchain_configuration>::visit(get_median_properties_calculator(results, std::move(votes)));
   return results;
}

std::ostream& operator<< (std::ostream& s, const blockchain_configuration& p) {
   return s << fc::json::to_string(p);
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

bool operator==(const types::blockchain_configuration& a, const types::blockchain_configuration& b) {
   // Yes, it's gross, I'm using a boolean exception to direct normal control flow... that's why it's buried deep in an
   // implementation detail file. I think it's worth it for the generalization, though: this code keeps working no
   // matter what updates happen to blockchain_configuration
   if (&a != &b) {
      try {
         fc::reflector<blockchain_configuration>::visit(comparison_visitor<types::blockchain_configuration>{a, b});
      } catch (bool) {
         return false;
      }
   }
   return true;
}

} } // namespace eosio::chain
