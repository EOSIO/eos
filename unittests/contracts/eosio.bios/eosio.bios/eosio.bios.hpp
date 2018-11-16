#pragma once
#include <eosiolib/crypto.h>
#include <eosiolib/eosio.hpp>
#include <eosiolib/privileged.hpp>

namespace eosio {

   class [[eosio::contract("eosio.bios")]] bios : public contract {
      public:
         using contract::contract;

         [[eosio::action]]
         void setpriv( name account, uint8_t is_priv ) {
            require_auth( _self );
            set_privileged( account.value, is_priv );
         }

         [[eosio::action]]
         void setalimits( name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight ) {
            require_auth( _self );
            set_resource_limits( account.value, ram_bytes, net_weight, cpu_weight );
         }

         [[eosio::action]]
         void setglimits( uint64_t ram, uint64_t net, uint64_t cpu ) {
            (void)ram; (void)net; (void)cpu;
            require_auth( _self );
         }

         [[eosio::action]]
         void setprods( std::vector<eosio::producer_key> schedule ) {
            (void)schedule; // schedule argument just forces the deserialization of the action data into vector<producer_key> (necessary check)
            require_auth( _self );

            constexpr size_t max_stack_buffer_size = 512;
            size_t size = action_data_size();
            char* buffer = (char*)( max_stack_buffer_size < size ? malloc(size) : alloca(size) );
            read_action_data( buffer, size );
            set_proposed_producers(buffer, size);
         }

         [[eosio::action]]
         void setparams( const eosio::blockchain_parameters& params ) {
            require_auth( _self );
            set_blockchain_parameters( params );
         }

         [[eosio::action]]
         void reqauth( name from ) {
            require_auth( from );
         }

         [[eosio::action]]
         void setabi( name account, const std::vector<char>& abi ) {
            abi_hash_table table(_self, _self.value);
            auto itr = table.find( account.value );
            if( itr == table.end() ) {
               table.emplace( account, [&]( auto& row ) {
                  row.owner = account;
                  sha256( const_cast<char*>(abi.data()), abi.size(), &row.hash );
               });
            } else {
               table.modify( itr, same_payer, [&]( auto& row ) {
                  sha256( const_cast<char*>(abi.data()), abi.size(), &row.hash );
               });
            }
         }

         struct [[eosio::table]] abi_hash {
            name              owner;
            capi_checksum256  hash;
            uint64_t primary_key()const { return owner.value; }

            EOSLIB_SERIALIZE( abi_hash, (owner)(hash) )
         };

         typedef eosio::multi_index< "abihash"_n, abi_hash > abi_hash_table;
   };

} /// namespace eosio
