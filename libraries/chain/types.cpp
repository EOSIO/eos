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
#include <eos/chain/types.hpp>

#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/combine.hpp>

namespace eos { namespace chain {

RoundChanges operator-(ProducerRound a, ProducerRound b) {
   boost::sort(a);
   boost::sort(b);
   auto ToPairs = boost::adaptors::transformed([](const auto& tuple) {
      return std::make_pair(boost::get<0>(tuple), boost::get<1>(tuple));
   });

   // Get a set of producers removed from a, and a set of producers added to b
   // We use set here because it sorts its elements for us
   std::set<AccountName> removed, added;
   boost::set_difference(a, b, std::inserter(removed, removed.begin()));
   boost::set_difference(b, a, std::inserter(added, added.begin()));

   // Zip removed and added, converting the tuples to pairs, and return them
   // Boost::combine is essentially python's zip()
   auto changes = boost::combine(std::move(removed), std::move(added)) | ToPairs;
   return RoundChanges(changes.begin(), changes.end());
}

} } // namespace eos::chain

namespace fc {
using eos::chain::shared_vector;
void to_variant(const shared_vector<eos::types::Field>& c, fc::variant& v) {
   fc::mutable_variant_object mvo; mvo.reserve(c.size());
   for(const auto& f : c) {
      mvo.set(f.name, eos::types::String(f.type));
   }
   v = std::move(mvo);
}
void from_variant(const fc::variant& v, shared_vector<eos::types::Field>& fields) {
   const auto& obj = v.get_object();
   fields.reserve(obj.size());
   for(const auto& f : obj)
      fields.emplace_back(eos::types::Field{ f.key(), f.value().get_string() });
}

void to_variant(const eos::chain::ProducerRound& r, variant& v) {
   v = std::vector<eos::chain::AccountName>(r.begin(), r.end());
}
void from_variant(const variant& v, eos::chain::ProducerRound& r) {
   auto vector = v.as<std::vector<eos::chain::AccountName>>();
   FC_ASSERT(vector.size() == r.size(), "Cannot extract ProducerRound from variant `${v}`: sizes do not match",
             ("v", v)("Round Size", r.size()));
   boost::copy(vector, r.begin());
}

} // namespace fc
