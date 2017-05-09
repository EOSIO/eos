#include <eos/chain/BlockchainConfiguration.hpp>

#include <boost/range/algorithm/nth_element.hpp>

#include <fc/io/json.hpp>

namespace eos { namespace chain {

template <typename T>
struct assign_visitor {
   T& a;
   const T& b;

   template <typename Member, class Class, Member (Class::*member)>
   void operator() (const char*) const {
      a.*member = b.*member;
   }
};

BlockchainConfiguration& BlockchainConfiguration::operator=(const types::BlockchainConfiguration& other) {
   if (&other != this)
      fc::reflector<BlockchainConfiguration>::visit(assign_visitor<types::BlockchainConfiguration>{*this, other});
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

BlockchainConfiguration BlockchainConfiguration::get_median_values(
      std::vector<BlockchainConfiguration> votes) {
   BlockchainConfiguration results;
   fc::reflector<BlockchainConfiguration>::visit(get_median_properties_calculator(results, std::move(votes)));
   return results;
}

std::ostream& operator<< (std::ostream& s, const BlockchainConfiguration& p) {
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

bool operator==(const types::BlockchainConfiguration& a, const types::BlockchainConfiguration& b) {
   // Yes, it's gross, I'm using a boolean exception to direct normal control flow... that's why it's buried deep in an
   // implementation detail file. I think it's worth it for the generalization, though: this code keeps working no
   // matter what updates happen to BlockchainConfiguration
   if (&a != &b) {
      try {
         fc::reflector<BlockchainConfiguration>::visit(comparison_visitor<types::BlockchainConfiguration>{a, b});
      } catch (bool) {
         return false;
      }
   }
   return true;
}

} } // namespace eos::chain
