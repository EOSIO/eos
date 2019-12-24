#include <apifiny/chain/webassembly/apifiny-vm-oc/intrinsic.hpp>

namespace apifiny { namespace chain { namespace apifinyvmoc {

static intrinsic_map_t& the_intrinsic_map() {
   static intrinsic_map_t intrinsic_map;
   return intrinsic_map;
}

const intrinsic_map_t& get_intrinsic_map() {
   return the_intrinsic_map();
}

intrinsic::intrinsic(const char* n, const IR::FunctionType* t, void* f, size_t o) {
   the_intrinsic_map().erase(n);
   the_intrinsic_map().emplace(n, intrinsic_entry{t, f, o});
}

}}}