#include <eosio/chain/webassembly/eos-vm-oc/config.hpp>

namespace eosio { namespace chain { namespace eosvmoc {

std::istream& operator>>(std::istream& in, map_mode& runtime) {
   std::string s;
   in >> s;
   if (s == "mapped")
      runtime = map_mode::mapped;
   else if (s == "heap")
      runtime = map_mode::heap;
   else if (s == "locked")
      runtime = map_mode::locked;
   else
      in.setstate(std::ios_base::failbit);
   return in;
}

std::ostream& operator<<(std::ostream& osm, map_mode m) {
   if(m == map_mode::mapped)
      osm << "mapped";
   else if (m == map_mode::heap)
      osm << "heap";
   else if (m == map_mode::locked)
      osm << "locked";

   return osm;
}

}}}