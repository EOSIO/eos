#include <eos/chain/BlockchainConfiguration.hpp>

#include <boost/range/algorithm.hpp>

#include <fc/io/json.hpp>

namespace eos { namespace chain {

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

template <typename T>
struct comparison_visitor {
   const T& a;
   const T& b;

   template <typename Member, class Class, Member (Class::*member)>
   void operator() (const char*) const {
      if (a.*member != b.*member)
         throw false;
   }
};

bool BlockchainConfiguration::operator==(const BlockchainConfiguration& other) const {
   try {
      fc::reflector<BlockchainConfiguration>::visit(comparison_visitor<BlockchainConfiguration>{*this, other});
   } catch (bool) {
      return false;
   }
   return true;
}

std::ostream& operator<< (std::ostream& s, const BlockchainConfiguration& p) {
   return s << fc::json::to_string(p);
}

} } // namespace eos::chain
