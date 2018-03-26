#include <eosiolib/eosio.hpp>
#include <eosiolib/privileged.h>
#include <eosiolib/producer_schedule.hpp>

namespace eosio {

   class bios : public contract {
      public:
         bios( action_name self ):contract(self){}

         void setpriv( account_name account, uint8_t ispriv ) {
            require_auth( _self );
            set_privileged( account, ispriv );
         }

         void setalimits( account_name account, uint64_t ram_bytes, uint64_t net_weight, uint64_t cpu_weight ) {
            require_auth( _self );
            set_resource_limits( account, ram_bytes, net_weight, cpu_weight );
         }

         void setglimits( uint64_t ram, uint64_t net, uint64_t cpu ) {
            require_auth( _self );
         }

         void setprods( producer_schedule sch ) {
            char buffer[action_data_size()];
            read_action_data( buffer, sizeof(buffer) );
            set_active_producers(buffer, sizeof(buffer));
         }

         void reqauth( action_name from ) {
            require_auth( from );
         }

      private:
   };

} /// namespace eosio
