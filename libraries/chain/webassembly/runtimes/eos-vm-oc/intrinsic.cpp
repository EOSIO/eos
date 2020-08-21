#include <eosio/chain/webassembly/eos-vm-oc/intrinsic.hpp>

#include <boost/hana/sort.hpp>
#include <boost/hana/zip.hpp>
#include <boost/hana/range.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/unpack.hpp>

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

namespace eosio { namespace chain { namespace eosvmoc {

static intrinsic_map_t& the_intrinsic_map() {
   static intrinsic_map_t intrinsic_map;
   return intrinsic_map;
}

const intrinsic_map_t& get_internal_intrinsic_map() {
   return the_intrinsic_map();
}

intrinsic::intrinsic(const char* n, void* f, size_t o) {
   the_intrinsic_map().erase(n);
   the_intrinsic_map().emplace(n, intrinsic_entry{f, o});
}

struct ordinal_entry {
   std::string_view first;
   std::size_t second;
};
constexpr bool operator<(const ordinal_entry& lhs, const ordinal_entry& rhs) { return lhs.first < rhs.first; }
// C++20: using std::sort;
constexpr ordinal_entry* partition(ordinal_entry* first, ordinal_entry* last) {
   ordinal_entry pivot = *first;
   while(true) {
      --last;
      while(first < last && pivot < *last) --last;
      if(first == last) break;
      *first = *last;

      ++first;
      while(first < last && *first < pivot) ++first;
      if(first == last) break;
      *last = *first;
   }
   *first = pivot;
   return first;
}
constexpr void sort(ordinal_entry* first, ordinal_entry* last) {
   if(last - first <= 1) return;
   // basic quicksort
   auto mid = eosvmoc::partition(first, last);
   sort(first, mid);
   sort(mid + 1, last);
}

static constexpr auto get_ordinal_map() {
   constexpr auto intrinsic_table = get_intrinsic_table();
   std::array<ordinal_entry, intrinsic_table.size()> result{};
   for(std::size_t i = 0; i < intrinsic_table.size(); ++i) {
     result[i] = { intrinsic_table[i], i };
   }
   eosvmoc::sort(result.data(), result.data() + result.size());
   return result;
}

std::size_t get_intrinsic_ordinal(std::string_view name) {
   static constexpr auto my_map = get_ordinal_map();
   auto iter = std::partition_point(my_map.begin(), my_map.end(), [name](const auto& elem){ return elem.first < name; });
   if(iter == my_map.end() || iter->first != name) throw std::runtime_error("unknown intrinsic");
   return iter->second;
}

}}}
