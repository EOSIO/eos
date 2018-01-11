/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eoslib/transaction.h>
#include <eoslib/print.hpp>
#include <eoslib/string.hpp>
#include <eoslib/types.hpp>
#include <eoslib/raw.hpp>

namespace eosio {

   /**
    * @defgroup transactioncppapi Transaction C++ API
    * @ingroup transactionapi
    * @brief Type-safe C++ wrappers for transaction C API
    *
    * @note There are some methods from the @ref transactioncapi that can be used directly from C++
    *
    * @{ 
    */


   template<size_t MaxPayloadSize = 64, size_t MaxPermissions = 2>
   class action {
   public:
      template<typename Payload, typename ...Permissions>
      action(const account_name& account, const action_name& name, const Payload& payload, Permissions... permissions )
      :_account(account), _name(name), _num_permissions(0)
      {
         assert(sizeof(payload) <= MaxPayloadSize, "Payload exceeds maximum size");
         memcpy(_payload, &payload, sizeof(payload));
         _payload_size = sizeof(payload);
         add_permissions(permissions...);
      }

      template<typename Payload>
      action(const account_name& account, const action_name& name, const Payload& payload )
      : _account(account), _name(name),  _num_permissions(0)
      {
         assert(sizeof(payload) <= MaxPayloadSize, "payload exceeds maximum size");
         _payload_size = sizeof(payload);
         memcpy(_payload, &payload, sizeof(payload));
      }

      action(const account_name& account, const action_name& name)
      :_account(account), _name(name),  _payload_size(0), _num_permissions(0)
      {
      }


      void add_permissions(account_name account, permission_name permission) {
         assert(_num_permissions < MaxPermissions, "Too many permissions" );
         _permissions[_num_permissions].account = account;
         _permissions[_num_permissions].permission = permission;
         _num_permissions++;
      }

      template<typename ...Permissions>
      void add_permissions(account_name account, permission_name permission, Permissions... permissions) {
         add_permissions(account, permission);
         add_permissions(permissions...);
      }

      void send() {
         char buffer[max_buffer_size()];
         size_t used = pack(buffer, max_buffer_size());
         send_inline(buffer, used);
      }

      account_name      _account;
      action_name       _name;
      char              _payload[MaxPayloadSize];
      size_t            _payload_size;
      account_permission  _permissions[MaxPermissions];
      size_t            _num_permissions;

      static constexpr size_t max_buffer_size() {
         return sizeof(size_t) + MaxPayloadSize + sizeof(size_t) + (MaxPermissions * sizeof(account_permission)) + sizeof(account_name) + sizeof(action_name);
      }

      size_t pack( char *buffer, size_t buffer_size ) const {
         datastream<char*>  ds( buffer, buffer_size );
         eosio::raw::pack(ds, _account);
         eosio::raw::pack(ds, _name);
         eosio::raw::pack(ds, _permissions, _num_permissions);
         eosio::raw::pack(ds, _payload, _payload_size);
         return ds.tellp();
      }

      template<typename T>
      T as() {
         assert(_payload_size == sizeof(T), "Action type mismatch");
         T temp;
         memcpy(_payload, &temp, sizeof(T));
         return temp;
      }

   private:
      action()
      {
      }

      template<size_t MaxActionsSize>
      friend class deferred_transaction;
   };

   template<size_t MaxActionsSize = 256>
   class transaction {
   public:
      transaction(time expiration = now() + 60, region_id region = 0)
      :_expiration(expiration),_region(region), _action_buffer_size(0), _num_actions(0)
      {}

      template<size_t ...ActArgs>
      void add_action(const action<ActArgs...> &act) {
         _action_buffer_size += act.pack(_action_buffer + _action_buffer_size, MaxActionsSize - _action_buffer_size );
         _num_actions++;
      }

      void send(uint32_t sender_id, time delay_until = 0) const {
         char buffer[max_buffer_size()];
         size_t packed_size = pack(buffer, max_buffer_size());
         send_deferred(sender_id, delay_until, buffer, packed_size);
      }

      static constexpr size_t max_buffer_size() {
         return sizeof(time) + sizeof(region_id) + sizeof(uint16_t) + sizeof(uint32_t) +
                sizeof(size_t) + MaxActionsSize;
      }

      size_t pack(char *buffer, size_t buffer_size) const {
         datastream<char*> ds(buffer, buffer_size);
         eosio::raw::pack(ds, _expiration);
         eosio::raw::pack(ds, _region);
         eosio::raw::pack(ds, uint16_t(0));
         eosio::raw::pack(ds, uint32_t(0));
         eosio::raw::pack(ds, unsigned_int(_num_actions));
         ds.write(_action_buffer, _action_buffer_size);
         return ds.tellp();
      }

      template<size_t MaxPayloadSize = 64, size_t MaxPermissions = 2>
      action<MaxPayloadSize, MaxPermissions> get_action(size_t index) const {
         assert(index < _num_actions, "Index out of range");
         size_t offset = 0;
         action<MaxPayloadSize, MaxPermissions> temp;
         for (size_t idx = 0; idx <= index; idx++) {
            offset = unpack_action(_action_buffer, _action_buffer_size, offset, temp);
         }
         return temp;
      }

      time            _expiration;
      region_id       _region;
      uint16_t        _ref_block_num;
      uint32_t        _ref_block_id;

      char            _action_buffer[MaxActionsSize];
      size_t          _action_buffer_size;

      size_t          _num_actions;

   protected:
      template<size_t MaxPayloadSize, size_t MaxPermissions>
      static size_t unpack_action(char* buffer, size_t size, size_t offset, action<MaxPayloadSize, MaxPermissions> &act) {
         datastream<char*> ds(buffer + offset, size - offset);
         eosio::raw::unpack(ds, act._account);
         eosio::raw::unpack(ds, act._name);
         eosio::raw::unpack(ds, act._permissions, act._num_permissions, MaxPermissions);
         eosio::raw::unpack(ds, act._payload, act._payload_size, MaxPayloadSize);
         return ds.tellp() + offset;
      }
   };

   template<size_t MaxActionsSize = 256>
   class deferred_transaction : public transaction<MaxActionsSize> {
      public:
         uint32_t     _sender_id;
         account_name _sender;
         time         _delay_until;

         static constexpr size_t max_buffer_size() {
            return sizeof(uint32_t) + sizeof(account_name) + sizeof(time) + transaction<MaxActionsSize>::max_buffer_size();
         }

         template< size_t MaxPayloadSize = 64, size_t MaxPermissions = 2 >
         static deferred_transaction from_current_action() {
            deferred_transaction result;
            static const size_t buffer_size = max_buffer_size();
            char buffer[buffer_size];

            auto read = read_action( buffer, buffer_size );

            datastream<char*> ds(buffer, read);
            eosio::raw::unpack(ds, result._expiration);
            eosio::raw::unpack(ds, result._region);
            eosio::raw::unpack(ds, result._ref_block_num);
            eosio::raw::unpack(ds, result._ref_block_id);
            unsigned_int packed_num_actions;
            eosio::raw::unpack(ds, packed_num_actions);
            result._num_actions = packed_num_actions.value;
            char* temp_actions = buffer + ds.tellp();
            size_t temp_action_size = read - ds.tellp();

            size_t action_offset = 0;
            action<MaxPayloadSize, MaxPermissions> temp;
            for (size_t idx; idx < result._num_actions; idx++) {
               action_offset = transaction<MaxActionsSize>::unpack_action(temp_actions, temp_action_size, action_offset, temp);
            }
            assert(action_offset <= MaxActionsSize, "Actions are too large to parse");
            result._action_buffer_size = action_offset;
            ds.read(result._action_buffer, result._action_buffer_size);

            eosio::raw::unpack(ds, result._sender_id);
            eosio::raw::unpack(ds, result._sender);
            eosio::raw::unpack(ds, result._delay_until);

            return result;
         }
      private:
         deferred_transaction()
         {}


   };

 ///@} transactioncpp api

} // namespace eos
