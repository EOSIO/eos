#include <proxy/proxy.hpp>
#include <currency/currency.hpp>

namespace proxy {
   using namespace eos;

   template<typename T>
   void apply_transfer(AccountName code, const T& transfer) {
      const auto self = currentCode();
      Config config;
      assert(Configs::get(config, self), "Attempting to use unconfigured proxy");
      if (transfer.from == self) {
         assert(transfer.to == config.owner,  "proxy may only pay its owner" );
      } else {
         assert(transfer.to == self, "proxy is not involved in this transfer");
         T newTransfer = T(transfer);
         newTransfer.from = self;
         newTransfer.to = config.owner;

         auto outMsg = Message(code, N(transfer), newTransfer, self, N(code));
         Transaction out;
         out.addMessage(outMsg);
         out.addScope(self);
         out.addScope(config.owner);
         out.send();
      }
   }

   void apply_setowner(AccountName owner) {
      const auto self = currentCode();
      Config config;
      bool configured = Configs::get(config, self);
      config.owner = owner;
      if (configured) {
         Configs::update(config, self);
      } else {
         Configs::store(config, self);
      }
   }
 
}

using namespace proxy;
using namespace eos;

extern "C" {
    void init()  {
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action ) {
       if( code == N(currency) ) {
          if( action == N(transfer) ) {
             apply_transfer(code, currentMessage<currency::Transfer>());
          }
       } else if ( code == N(eos) ) {
          if( action == N(transfer) ) {
             apply_transfer(code, currentMessage<eos::Transfer>());
          }
       } else if (code == N(proxy) ) {
          if ( action == N(setowner)) {
             apply_setowner(currentMessage<AccountName>());
          }
       }
    }
}
