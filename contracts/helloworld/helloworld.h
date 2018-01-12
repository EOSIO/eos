#ifndef HELLOWORLD_H
#define HELLOWORLD_H

#include <eoslib/eos.hpp>

namespace helloworld {

struct transfer {
   /**
   * account to transfer from
   */
   uint64_t       from;
   /**
   * account to transfer to
   */
   uint64_t       to;
   /**
   *  quantity to transfer
   */
   uint64_t    quantity;
};

} // namespace

//namespace tester {
//   struct Message {
//     char message = 0;
//   };
//}

#endif // HELLOWORLD_H
