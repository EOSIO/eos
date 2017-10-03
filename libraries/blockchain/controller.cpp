#include <eosio/blockchain/controller.hpp>

namespace eosio { namespace blockchain {

   controller::controller( io_service& ios )
   :_ios(ios), _strand( new strand(ios) ) {

   }

   controller::~controller() {
      _strand.reset(); /// finish all outstanding tasks
   }

} } /// eosio::blockchain
