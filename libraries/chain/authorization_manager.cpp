/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/authority_checker.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/contract_types.hpp>
#include <eosio/chain/generated_transaction_object.hpp>

namespace eosio { namespace chain {

   authorization_manager::authorization_manager(controller& c, database& d)
   :_control(c),_db(d){}

   void authorization_manager::add_indices() {
      _db.add_index<permission_index>();
      _db.add_index<permission_usage_index>();
      _db.add_index<permission_link_index>();
   }

   void authorization_manager::initialize_database() {
      _db.create<permission_object>([](auto&){}); /// reserve perm 0 (used else where)
   }

   const permission_object& authorization_manager::create_permission( account_name account,
                                                                      permission_name name,
                                                                      permission_id_type parent,
                                                                      const authority& auth,
                                                                      time_point initial_creation_time
                                                                    )
   {
      auto creation_time = initial_creation_time;
      if( creation_time == time_point() ) {
         creation_time = _control.pending_block_time();
      }

      const auto& perm_usage = _db.create<permission_usage_object>([&](auto& p) {
         p.last_used = creation_time;
      });

      const auto& perm = _db.create<permission_object>([&](auto& p) {
         p.usage_id     = perm_usage.id;
         p.parent       = parent;
         p.owner        = account;
         p.name         = name;
         p.last_updated = creation_time;
         p.auth         = auth;
      });
      return perm;
   }

   const permission_object& authorization_manager::create_permission( account_name account,
                                                                      permission_name name,
                                                                      permission_id_type parent,
                                                                      authority&& auth,
                                                                      time_point initial_creation_time
                                                                    )
   {
      auto creation_time = initial_creation_time;
      if( creation_time == time_point() ) {
         creation_time = _control.pending_block_time();
      }

      const auto& perm_usage = _db.create<permission_usage_object>([&](auto& p) {
         p.last_used = creation_time;
      });

      const auto& perm = _db.create<permission_object>([&](auto& p) {
         p.usage_id     = perm_usage.id;
         p.parent       = parent;
         p.owner        = account;
         p.name         = name;
         p.last_updated = creation_time;
         p.auth         = std::move(auth);
      });
      return perm;
   }

   void authorization_manager::modify_permission( const permission_object& permission, const authority& auth ) {
      _db.modify( permission, [&](permission_object& po) {
         po.auth = auth;
         po.last_updated = _control.pending_block_time();
      });
   }

   void authorization_manager::remove_permission( const permission_object& permission ) {
      const auto& index = _db.template get_index<permission_index, by_parent>();
      auto range = index.equal_range(permission.id);
      EOS_ASSERT( range.first == range.second, action_validate_exception,
                  "Cannot remove a permission which has children. Remove the children first.");

      _db.get_mutable_index<permission_usage_index>().remove_object( permission.usage_id._id );
      _db.remove( permission );
   }

   void authorization_manager::update_permission_usage( const permission_object& permission ) {
      const auto& puo = _db.get<permission_usage_object, by_id>( permission.usage_id );
      _db.modify( puo, [&](permission_usage_object& p) {
         p.last_used = _control.pending_block_time();
      });
   }

   fc::time_point authorization_manager::get_permission_last_used( const permission_object& permission )const {
      return _db.get<permission_usage_object, by_id>( permission.usage_id ).last_used;
   }

   const permission_object*  authorization_manager::find_permission( const permission_level& level )const
   { try {
      FC_ASSERT( !level.actor.empty() && !level.permission.empty(), "Invalid permission" );
      return _db.find<permission_object, by_owner>( boost::make_tuple(level.actor,level.permission) );
   } EOS_RETHROW_EXCEPTIONS( chain::permission_query_exception, "Failed to retrieve permission: ${level}", ("level", level) ) }

   const permission_object&  authorization_manager::get_permission( const permission_level& level )const
   { try {
      FC_ASSERT( !level.actor.empty() && !level.permission.empty(), "Invalid permission" );
      return _db.get<permission_object, by_owner>( boost::make_tuple(level.actor,level.permission) );
   } EOS_RETHROW_EXCEPTIONS( chain::permission_query_exception, "Failed to retrieve permission: ${level}", ("level", level) ) }

   optional<permission_name> authorization_manager::lookup_linked_permission( account_name authorizer_account,
                                                                              account_name scope,
                                                                              action_name act_name
                                                                            )const
   {
      try {
         // First look up a specific link for this message act_name
         auto key = boost::make_tuple(authorizer_account, scope, act_name);
         auto link = _db.find<permission_link_object, by_action_name>(key);
         // If no specific link found, check for a contract-wide default
         if (link == nullptr) {
            boost::get<2>(key) = "";
            link = _db.find<permission_link_object, by_action_name>(key);
         }

         // If no specific or default link found, use active permission
         if (link != nullptr) {
            return link->required_permission;
         }
         return optional<permission_name>();

       //  return optional<permission_name>();
      } FC_CAPTURE_AND_RETHROW((authorizer_account)(scope)(act_name))
   }

   optional<permission_name> authorization_manager::lookup_minimum_permission( account_name authorizer_account,
                                                                               account_name scope,
                                                                               action_name act_name
                                                                             )const
   {
      // Special case native actions cannot be linked to a minimum permission, so there is no need to check.
      if( scope == config::system_account_name ) {
          FC_ASSERT( act_name != updateauth::get_name() &&
                     act_name != deleteauth::get_name() &&
                     act_name != linkauth::get_name() &&
                     act_name != unlinkauth::get_name() &&
                     act_name != canceldelay::get_name(),
                     "cannot call lookup_minimum_permission on native actions that are not allowed to be linked to minimum permissions" );
      }

      try {
         optional<permission_name> linked_permission = lookup_linked_permission(authorizer_account, scope, act_name);
         if( !linked_permission )
            return config::active_name;

         if( *linked_permission == config::eosio_any_name )
            return optional<permission_name>();

         return linked_permission;
      } FC_CAPTURE_AND_RETHROW((authorizer_account)(scope)(act_name))
   }

   void authorization_manager::check_updateauth_authorization( const updateauth& update,
                                                               const vector<permission_level>& auths
                                                             )const
   {
      EOS_ASSERT( auths.size() == 1, irrelevant_auth_exception,
                  "updateauth action should only have one declared authorization" );
      const auto& auth = auths[0];
      EOS_ASSERT( auth.actor == update.account, irrelevant_auth_exception,
                  "the owner of the affected permission needs to be the actor of the declared authorization" );

      const auto* min_permission = find_permission({update.account, update.permission});
      if( !min_permission ) { // creating a new permission
         min_permission = &get_permission({update.account, update.parent});
      }

      EOS_ASSERT( get_permission(auth).satisfies( *min_permission,
                                                  _db.get_index<permission_index>().indices() ),
                  irrelevant_auth_exception,
                  "updateauth action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                  ("auth", auth)("min", permission_level{update.account, min_permission->name}) );
   }

   void authorization_manager::check_deleteauth_authorization( const deleteauth& del,
                                                               const vector<permission_level>& auths
                                                             )const
   {
      EOS_ASSERT( auths.size() == 1, irrelevant_auth_exception,
                  "deleteauth action should only have one declared authorization" );
      const auto& auth = auths[0];
      EOS_ASSERT( auth.actor == del.account, irrelevant_auth_exception,
                  "the owner of the permission to delete needs to be the actor of the declared authorization" );

      const auto& min_permission = get_permission({del.account, del.permission});

      EOS_ASSERT( get_permission(auth).satisfies( min_permission,
                                                  _db.get_index<permission_index>().indices() ),
                  irrelevant_auth_exception,
                  "updateauth action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                  ("auth", auth)("min", permission_level{min_permission.owner, min_permission.name}) );
   }

   void authorization_manager::check_linkauth_authorization( const linkauth& link,
                                                             const vector<permission_level>& auths
                                                           )const
   {
      EOS_ASSERT( auths.size() == 1, irrelevant_auth_exception,
                  "link action should only have one declared authorization" );
      const auto& auth = auths[0];
      EOS_ASSERT( auth.actor == link.account, irrelevant_auth_exception,
                  "the owner of the linked permission needs to be the actor of the declared authorization" );

      EOS_ASSERT( link.type != updateauth::get_name(),  action_validate_exception,
                  "Cannot link eosio::updateauth to a minimum permission" );
      EOS_ASSERT( link.type != deleteauth::get_name(),  action_validate_exception,
                  "Cannot link eosio::deleteauth to a minimum permission" );
      EOS_ASSERT( link.type != linkauth::get_name(),    action_validate_exception,
                  "Cannot link eosio::linkauth to a minimum permission" );
      EOS_ASSERT( link.type != unlinkauth::get_name(),  action_validate_exception,
                  "Cannot link eosio::unlinkauth to a minimum permission" );
      EOS_ASSERT( link.type != canceldelay::get_name(), action_validate_exception,
                  "Cannot link eosio::canceldelay to a minimum permission" );

      const auto linked_permission_name = lookup_minimum_permission(link.account, link.code, link.type);

      if( !linked_permission_name ) // if action is linked to eosio.any permission
         return;

      EOS_ASSERT( get_permission(auth).satisfies( get_permission({link.account, *linked_permission_name}),
                                                  _db.get_index<permission_index>().indices()              ),
                  irrelevant_auth_exception,
                  "link action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                  ("auth", auth)("min", permission_level{link.account, *linked_permission_name}) );
   }

   void authorization_manager::check_unlinkauth_authorization( const unlinkauth& unlink,
                                                               const vector<permission_level>& auths
                                                             )const
   {
      EOS_ASSERT( auths.size() == 1, irrelevant_auth_exception,
                  "unlink action should only have one declared authorization" );
      const auto& auth = auths[0];
      EOS_ASSERT( auth.actor == unlink.account, irrelevant_auth_exception,
                  "the owner of the linked permission needs to be the actor of the declared authorization" );

      const auto unlinked_permission_name = lookup_linked_permission(unlink.account, unlink.code, unlink.type);
      EOS_ASSERT( unlinked_permission_name.valid(), transaction_exception,
                  "cannot unlink non-existent permission link of account '${account}' for actions matching '${code}::${action}'",
                  ("account", unlink.account)("code", unlink.code)("action", unlink.type) );

      if( *unlinked_permission_name == config::eosio_any_name )
         return;

      EOS_ASSERT( get_permission(auth).satisfies( get_permission({unlink.account, *unlinked_permission_name}),
                                                  _db.get_index<permission_index>().indices()                  ),
                  irrelevant_auth_exception,
                  "unlink action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                  ("auth", auth)("min", permission_level{unlink.account, *unlinked_permission_name}) );
   }

   fc::microseconds authorization_manager::check_canceldelay_authorization( const canceldelay& cancel,
                                                                            const vector<permission_level>& auths
                                                                          )const
   {
      EOS_ASSERT( auths.size() == 1, irrelevant_auth_exception,
                  "canceldelay action should only have one declared authorization" );
      const auto& auth = auths[0];

      EOS_ASSERT( get_permission(auth).satisfies( get_permission(cancel.canceling_auth),
                                                  _db.get_index<permission_index>().indices() ),
                  irrelevant_auth_exception,
                  "canceldelay action declares irrelevant authority '${auth}'; specified authority to satisfy is ${min}",
                  ("auth", auth)("min", cancel.canceling_auth) );

      const auto& trx_id = cancel.trx_id;

      const auto& generated_transaction_idx = _control.db().get_index<generated_transaction_multi_index>();
      const auto& generated_index = generated_transaction_idx.indices().get<by_trx_id>();
      const auto& itr = generated_index.lower_bound(trx_id);
      FC_ASSERT( itr != generated_index.end() && itr->sender == account_name() && itr->trx_id == trx_id,
                 "cannot cancel trx_id=${tid}, there is no deferred transaction with that transaction id",
                 ("tid", trx_id) );

      auto trx = fc::raw::unpack<transaction>(itr->packed_trx.data(), itr->packed_trx.size());
      bool found = false;
      for( const auto& act : trx.actions ) {
         for( const auto& auth : act.authorization ) {
            if( auth == cancel.canceling_auth ) {
               found = true;
               break;
            }
         }
         if( found ) break;
      }

      EOS_ASSERT( found, action_validate_exception,
                  "canceling_auth in canceldelay action was not found as authorization in the original delayed transaction" );

      return (itr->delay_until - itr->published);
   }

   void noop_checktime() {}

   std::function<void()> authorization_manager::_noop_checktime{&noop_checktime};

   void
   authorization_manager::check_authorization( const vector<action>&                actions,
                                               const flat_set<public_key_type>&     provided_keys,
                                               const flat_set<permission_level>&    provided_permissions,
                                               fc::microseconds                     provided_delay,
                                               const std::function<void()>&         _checktime,
                                               bool                                 allow_unused_keys
                                             )const
   {
      const auto& checktime = ( static_cast<bool>(_checktime) ? _checktime : _noop_checktime );

      auto delay_max_limit = fc::seconds( _control.get_global_properties().configuration.max_transaction_delay );

      auto effective_provided_delay =  (provided_delay >= delay_max_limit) ? fc::microseconds::maximum() : provided_delay;

      auto checker = make_auth_checker( [&](const permission_level& p){ return get_permission(p).auth; },
                                        _control.get_global_properties().configuration.max_authority_depth,
                                        provided_keys,
                                        provided_permissions,
                                        effective_provided_delay,
                                        checktime
                                      );

      map<permission_level, fc::microseconds> permissions_to_satisfy;

      for( const auto& act : actions ) {
         bool special_case = false;
         fc::microseconds delay = effective_provided_delay;

         if( act.account == config::system_account_name ) {
            special_case = true;

            if( act.name == updateauth::get_name() ) {
               check_updateauth_authorization( act.data_as<updateauth>(), act.authorization );
            } else if( act.name == deleteauth::get_name() ) {
               check_deleteauth_authorization( act.data_as<deleteauth>(), act.authorization );
            } else if( act.name == linkauth::get_name() ) {
               check_linkauth_authorization( act.data_as<linkauth>(), act.authorization );
            } else if( act.name == unlinkauth::get_name() ) {
               check_unlinkauth_authorization( act.data_as<unlinkauth>(), act.authorization );
            } else if( act.name ==  canceldelay::get_name() ) {
               delay = std::max( delay, check_canceldelay_authorization(act.data_as<canceldelay>(), act.authorization) );
            } else {
               special_case = false;
            }
         }

         for( const auto& declared_auth : act.authorization ) {

            checktime();

            if( !special_case ) {
               auto min_permission_name = lookup_minimum_permission(declared_auth.actor, act.account, act.name);
               if( min_permission_name ) { // since special cases were already handled, it should only be false if the permission is eosio.any
                  const auto& min_permission = get_permission({declared_auth.actor, *min_permission_name});
                  EOS_ASSERT( get_permission(declared_auth).satisfies( min_permission,
                                                                       _db.get_index<permission_index>().indices() ),
                              irrelevant_auth_exception,
                              "action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                              ("auth", declared_auth)("min", permission_level{min_permission.owner, min_permission.name}) );
               }
            }

            auto res = permissions_to_satisfy.emplace( declared_auth, delay );
            if( !res.second && res.first->second > delay) { // if the declared_auth was already in the map and with a higher delay
               res.first->second = delay;
            }
         }
      }

      // Now verify that all the declared authorizations are satisfied:

      // Although this can be made parallel (especially for input transactions) with the optimistic assumption that the
      // CPU limit is not reached, because of the CPU limit the protocol must officially specify a sequential algorithm
      // for checking the set of declared authorizations.
      // The permission_levels are traversed in ascending order, which is:
      // ascending order of the actor name with ties broken by ascending order of the permission name.
      for( const auto& p : permissions_to_satisfy ) {
         checktime(); // TODO: this should eventually move into authority_checker instead
         EOS_ASSERT( checker.satisfied( p.first, p.second ), unsatisfied_authorization,
                     "transaction declares authority '${auth}', "
                     "but does not have signatures for it under a provided delay of ${provided_delay} ms",
                     ("auth", p.first)("provided_delay", provided_delay.count()/1000)
                     ("delay_max_limit_ms", delay_max_limit.count()/1000)
                   );

      }

      if( !allow_unused_keys ) {
         EOS_ASSERT( checker.all_keys_used(), tx_irrelevant_sig,
                     "transaction bears irrelevant signatures from these keys: ${keys}",
                     ("keys", checker.unused_keys()) );
      }
   }

   void
   authorization_manager::check_authorization( account_name                         account,
                                               permission_name                      permission,
                                               const flat_set<public_key_type>&     provided_keys,
                                               const flat_set<permission_level>&    provided_permissions,
                                               fc::microseconds                     provided_delay,
                                               const std::function<void()>&         _checktime,
                                               bool                                 allow_unused_keys
                                             )const
   {
      const auto& checktime = ( static_cast<bool>(_checktime) ? _checktime : _noop_checktime );

      auto delay_max_limit = fc::seconds( _control.get_global_properties().configuration.max_transaction_delay );

      auto checker = make_auth_checker( [&](const permission_level& p){ return get_permission(p).auth; },
                                        _control.get_global_properties().configuration.max_authority_depth,
                                        provided_keys,
                                        provided_permissions,
                                        ( provided_delay >= delay_max_limit ) ? fc::microseconds::maximum() : provided_delay,
                                        checktime
                                      );

      EOS_ASSERT( checker.satisfied({account, permission}), unsatisfied_authorization,
                  "permission '${auth}' was not satisfied under a provided delay of ${provided_delay} ms",
                  ("auth", permission_level{account, permission})("provided_delay", provided_delay.count()/1000) );

      if( !allow_unused_keys ) {
         EOS_ASSERT( checker.all_keys_used(), tx_irrelevant_sig,
                     "irrelevant keys provided: ${keys}",
                     ("keys", checker.unused_keys()) );
      }
   }

   flat_set<public_key_type> authorization_manager::get_required_keys( const transaction& trx,
                                                                       const flat_set<public_key_type>& candidate_keys,
                                                                       fc::microseconds provided_delay
                                                                     )const
   {
      auto checker = make_auth_checker( [&](const permission_level& p){ return get_permission(p).auth; },
                                        _control.get_global_properties().configuration.max_authority_depth,
                                        candidate_keys,
                                        {},
                                        provided_delay,
                                        _noop_checktime
                                      );

      for (const auto& act : trx.actions ) {
         for (const auto& declared_auth : act.authorization) {
            EOS_ASSERT( checker.satisfied(declared_auth), unsatisfied_authorization,
                        "transaction declares authority '${auth}', but does not have signatures for it.",
                        ("auth", declared_auth) );
         }
      }

      return checker.used_keys();
   }

} } /// namespace eosio::chain
