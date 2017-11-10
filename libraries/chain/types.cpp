/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/chain/types.hpp>

#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/combine.hpp>

namespace eosio { namespace chain {

round_changes operator-(producer_round a, producer_round b) {
   boost::sort(a);
   boost::sort(b);
   auto ToPairs = boost::adaptors::transformed([](const auto& tuple) {
      return std::make_pair(boost::get<0>(tuple), boost::get<1>(tuple));
   });

   // Get a set of producers removed from a, and a set of producers added to b
   // We use set here because it sorts its elements for us
   std::set<account_name> removed, added;
   boost::set_difference(a, b, std::inserter(removed, removed.begin()));
   boost::set_difference(b, a, std::inserter(added, added.begin()));

   // Zip removed and added, converting the tuples to pairs, and return them
   // Boost::combine is essentially python's zip()
   auto changes = boost::combine(std::move(removed), std::move(added)) | ToPairs;
   return round_changes(changes.begin(), changes.end());
}

} } // namespace eosio::chain

namespace fc {
using eosio::chain::shared_vector;
void to_variant(const shared_vector<eosio::types::field>& c, fc::variant& v) {
   fc::mutable_variant_object mvo; mvo.reserve(c.size());
   for(const auto& f : c) {
      mvo.set(f.name, eosio::types::string(f.type));
   }
   v = std::move(mvo);
}
void from_variant(const fc::variant& v, shared_vector<eosio::types::field>& fields) {
   const auto& obj = v.get_object();
   fields.reserve(obj.size());
   for(const auto& f : obj)
      fields.emplace_back(eosio::types::field{ f.key(), f.value().get_string() });
}

void to_variant(const eosio::chain::producer_round& r, variant& v) {
   v = std::vector<eosio::chain::account_name>(r.begin(), r.end());
}
void from_variant(const variant& v, eosio::chain::producer_round& r) {
   auto vector = v.as<std::vector<eosio::chain::account_name>>();
   FC_ASSERT(vector.size() == r.size(), "Cannot extract producer_round from variant `${v}`: sizes do not match",
             ("v", v)("Round Size", r.size()));
   boost::copy(vector, r.begin());
}

} // namespace fc
