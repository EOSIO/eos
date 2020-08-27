#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/snapshot.hpp>

#include <utility>
#include <functional>

namespace eosio { namespace chain {

   class controller;
   struct updateauth;
   struct deleteauth;
   struct linkauth;
   struct unlinkauth;
   struct canceldelay;

   class authorization_manager {
      public:
         using permission_id_type = permission_object::id_type;

         explicit authorization_manager(controller& c, chainbase::database& d);

         void add_indices();
         void initialize_database();
         void add_to_snapshot( const snapshot_writer_ptr& snapshot ) const;
         void read_from_snapshot( const snapshot_reader_ptr& snapshot );

         const permission_object& create_permission( account_name account,
                                                     permission_name name,
                                                     permission_id_type parent,
                                                     const authority& auth,
                                                     uint32_t action_id,
                                                     time_point initial_creation_time = time_point()
                                                   );

         const permission_object& create_permission( account_name account,
                                                     permission_name name,
                                                     permission_id_type parent,
                                                     authority&& auth,
                                                     uint32_t action_id,
                                                     time_point initial_creation_time = time_point()
                                                   );

         void modify_permission( const permission_object& permission, const authority& auth, uint32_t action_id );

         void remove_permission( const permission_object& permission, uint32_t action_id );

         void update_permission_usage( const permission_object& permission );

         fc::time_point get_permission_last_used( const permission_object& permission )const;

         const permission_object*  find_permission( const permission_level& level )const;
         const permission_object&  get_permission( const permission_level& level )const;

         /**
          * @brief Find the lowest authority level required for @ref authorizer_account to authorize a message of the
          * specified type
          * @param authorizer_account The account authorizing the message
          * @param code_account The account which publishes the contract that handles the message
          * @param type The type of message
          */
         std::optional<permission_name> lookup_minimum_permission( account_name authorizer_account,
                                                                   scope_name code_account,
                                                                   action_name type
                                                                 )const;

         /**
          *  @brief Check authorizations of a vector of actions with provided keys, permission levels, and delay
          *
          *  @param actions - the actions to check authorization across
          *  @param provided_keys - the set of public keys which have authorized the transaction
          *  @param provided_permissions - the set of permissions which have authorized the transaction (empty permission name acts as wildcard)
          *  @param provided_delay - the delay satisfied by the transaction
          *  @param checktime - the function that can be called to track CPU usage and time during the process of checking authorization
          *  @param allow_unused_keys - true if method should not assert on unused keys
          */
         void
         check_authorization( const vector<action>&                actions,
                              const flat_set<public_key_type>&     provided_keys,
                              const flat_set<permission_level>&    provided_permissions = flat_set<permission_level>(),
                              fc::microseconds                     provided_delay = fc::microseconds(0),
                              const std::function<void()>&         checktime = std::function<void()>(),
                              bool                                 allow_unused_keys = false,
                              const flat_set<permission_level>&    satisfied_authorizations = flat_set<permission_level>()
                            )const;


         /**
          *  @brief Check authorizations of a permission with provided keys, permission levels, and delay
          *
          *  @param account - the account owner of the permission
          *  @param permission - the permission name to check for authorization
          *  @param provided_keys - a set of public keys
          *  @param provided_permissions - the set of permissions which can be considered satisfied (empty permission name acts as wildcard)
          *  @param provided_delay - the delay considered to be satisfied for the authorization check
          *  @param checktime - the function that can be called to track CPU usage and time during the process of checking authorization
          *  @param allow_unused_keys - true if method does not require all keys to be used
          */
         void
         check_authorization( account_name                         account,
                              permission_name                      permission,
                              const flat_set<public_key_type>&     provided_keys,
                              const flat_set<permission_level>&    provided_permissions = flat_set<permission_level>(),
                              fc::microseconds                     provided_delay = fc::microseconds(0),
                              const std::function<void()>&         checktime = std::function<void()>(),
                              bool                                 allow_unused_keys = false
                            )const;

         flat_set<public_key_type> get_required_keys( const transaction& trx,
                                                      const flat_set<public_key_type>& candidate_keys,
                                                      fc::microseconds provided_delay = fc::microseconds(0)
                                                    )const;


         static std::function<void()> _noop_checktime;

      private:
         const controller&    _control;
         chainbase::database& _db;

         void             check_updateauth_authorization( const updateauth& update, const vector<permission_level>& auths )const;
         void             check_deleteauth_authorization( const deleteauth& del, const vector<permission_level>& auths )const;
         void             check_linkauth_authorization( const linkauth& link, const vector<permission_level>& auths )const;
         void             check_unlinkauth_authorization( const unlinkauth& unlink, const vector<permission_level>& auths )const;
         fc::microseconds check_canceldelay_authorization( const canceldelay& cancel, const vector<permission_level>& auths )const;

         std::optional<permission_name> lookup_linked_permission( account_name authorizer_account,
                                                                  scope_name code_account,
                                                                  action_name type
                                                                )const;
   };

} } /// namespace eosio::chain
