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
#include <stdexcept>

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

static constexpr auto get_ordinal_map() {
   constexpr auto static_result = boost::hana::sort(boost::hana::zip(intrinsic_table, boost::hana::to_tuple(boost::hana::make_range(boost::hana::int_c<0>, boost::hana::length(intrinsic_table)))));
   constexpr std::size_t array_size = boost::hana::length(intrinsic_table);
   using result_type = std::array<std::pair<std::string_view, std::size_t>, array_size>;
   constexpr auto construct_array = [](auto... a) {
      return result_type{
         std::pair<std::string_view, std::size_t>{
            boost::hana::at(a, boost::hana::int_c<0>).c_str(),
            boost::hana::at(a, boost::hana::int_c<1>)
         }...
      };
   };
   return boost::hana::unpack(static_result, construct_array);
}

std::size_t get_intrinsic_ordinal(std::string_view name) {
   static constexpr auto my_map = get_ordinal_map();
   auto iter = std::partition_point(my_map.begin(), my_map.end(), [name](const auto& elem){ return elem.first < name; });
   if(iter == my_map.end() || iter->first != name) throw std::runtime_error("unknown intrinsic");
   return iter->second;
}

}}}
