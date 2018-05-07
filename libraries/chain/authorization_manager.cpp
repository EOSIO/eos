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
      const auto& perm = _db.create<permission_object>([&](auto& p) {
         p.name   = name;
         p.parent = parent;
         p.owner  = account;
         p.auth   = auth;
         p.delay  = fc::seconds(auth.delay_sec);
         if( initial_creation_time == time_point())
            p.last_updated = _control.pending_block_time();
         else
            p.last_updated = initial_creation_time;
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
      const auto& perm = _db.create<permission_object>([&](auto& p) {
         p.name   = name;
         p.parent = parent;
         p.owner  = account;
         p.auth   = std::move(auth);
         p.delay  = fc::seconds(auth.delay_sec);
         if( initial_creation_time == time_point())
            p.last_updated = _control.pending_block_time();
         else
            p.last_updated = initial_creation_time;
      });
      return perm;
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

   optional<fc::microseconds> authorization_manager::check_updateauth_authorization( const updateauth& update,
                                                                                     const vector<permission_level>& auths
                                                                                   )const
   {
      EOS_ASSERT( auths.size() == 1, irrelevant_auth_exception,
                  "updateauth action should only have one declared authorization" );
      const auto& auth = auths[0];
      EOS_ASSERT( auth.actor == update.account, irrelevant_auth_exception,
                  "the owner of the affected permission needs to be the actor of the declared authorization" );

      const auto* min_permission = find_permission({update.account, update.permission});
      bool ignore_delay = false;
      if( !min_permission ) { // creating a new permission
         ignore_delay = true;
         min_permission = &get_permission({update.account, update.parent});

      }
      const auto delay = get_permission(auth).satisfies( *min_permission,
                                                         _db.get_index<permission_index>().indices() );
      EOS_ASSERT( delay.valid(),
                  irrelevant_auth_exception,
                  "updateauth action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                  ("auth", auth)("min", permission_level{update.account, min_permission->name}) );

      return (ignore_delay ? optional<fc::microseconds>() : *delay);
   }

   fc::microseconds authorization_manager::check_deleteauth_authorization( const deleteauth& del,
                                                                           const vector<permission_level>& auths
                                                                         )const
   {
      EOS_ASSERT( auths.size() == 1, irrelevant_auth_exception,
                  "deleteauth action should only have one declared authorization" );
      const auto& auth = auths[0];
      EOS_ASSERT( auth.actor == del.account, irrelevant_auth_exception,
                  "the owner of the permission to delete needs to be the actor of the declared authorization" );

      const auto& min_permission = get_permission({del.account, del.permission});
      const auto delay = get_permission(auth).satisfies( min_permission,
                                                         _db.get_index<permission_index>().indices() );
      EOS_ASSERT( delay.valid(),
                  irrelevant_auth_exception,
                  "updateauth action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                  ("auth", auth)("min", permission_level{min_permission.owner, min_permission.name}) );

      return *delay;
   }

   fc::microseconds authorization_manager::check_linkauth_authorization( const linkauth& link,
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
         return fc::microseconds(0);

      const auto delay = get_permission(auth).satisfies( get_permission({link.account, *linked_permission_name}),
                                                         _db.get_index<permission_index>().indices() );

      EOS_ASSERT( delay.valid(),
                  irrelevant_auth_exception,
                  "link action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                  ("auth", auth)("min", permission_level{link.account, *linked_permission_name}) );

      return *delay;
   }

   fc::microseconds authorization_manager::check_unlinkauth_authorization( const unlinkauth& unlink,
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
         return fc::microseconds(0);

      const auto delay = get_permission(auth).satisfies( get_permission({unlink.account, *unlinked_permission_name}),
                                                         _db.get_index<permission_index>().indices() );

      EOS_ASSERT( delay.valid(),
                  irrelevant_auth_exception,
                  "unlink action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                  ("auth", auth)("min", permission_level{unlink.account, *unlinked_permission_name}) );

      return *delay;
   }

   void authorization_manager::check_canceldelay_authorization( const canceldelay& cancel,
                                                                const vector<permission_level>& auths
                                                              )const
   {
      EOS_ASSERT( auths.size() == 1, irrelevant_auth_exception,
                  "canceldelay action should only have one declared authorization" );
      const auto& auth = auths[0];

      const auto delay = get_permission(auth).satisfies( get_permission(cancel.canceling_auth),
                                                         _db.get_index<permission_index>().indices() );
      EOS_ASSERT( delay.valid(),
                  irrelevant_auth_exception,
                  "canceldelay action declares irrelevant authority '${auth}'; specified authority to satisfy is ${min}",
                  ("auth", auth)("min", cancel.canceling_auth) );
   }

   class permission_visitor {
   public:
      permission_visitor( const authorization_manager& authorization, fc::microseconds delay_max_limit )
      :_authorization(authorization)
      ,_delay_max_limit(delay_max_limit)
      ,_track_delay(true)
      {
         _max_delay_stack.emplace_back();
      }

      void operator()( const permission_level& perm_level ) {}

      void operator()( const permission_level& perm_level, bool repeat ) {
         if( repeat ) return;

         const auto obj = _authorization.get_permission(perm_level);
         if( _track_delay ) {
            _max_delay_stack.back() = std::max( _max_delay_stack.back(), std::min( obj.delay, _delay_max_limit ) );
         }
      }

      void push_undo() {
         _max_delay_stack.emplace_back( _max_delay_stack.back() );
      }

      void pop_undo() {
         FC_ASSERT( _max_delay_stack.size() >= 2, "invariant failure in permission_visitor" );
         _max_delay_stack.pop_back();
      }

      void squash_undo() {
         FC_ASSERT( _max_delay_stack.size() >= 2, "invariant failure in permission_visitor" );
         auto delay_to_keep = _max_delay_stack.back();
         _max_delay_stack.pop_back();
         _max_delay_stack.back() = delay_to_keep;
      }

      fc::microseconds get_max_delay()const {
         FC_ASSERT( _max_delay_stack.size() == 1, "invariant failure in permission_visitor" );
         return _max_delay_stack.back();
      }

      void pause_delay_tracking() {
         _track_delay = false;
      }

      void resume_delay_tracking() {
         _track_delay = true;
      }

   private:
      const authorization_manager& _authorization;
      vector<fc::microseconds> _max_delay_stack;
      fc::microseconds _delay_max_limit;
      bool _track_delay;
   };

   void noop_checktime( uint32_t ) {}

   std::function<void(uint32_t)> authorization_manager::_noop_checktime{std::bind(&noop_checktime, std::placeholders::_1)};

   fc::microseconds
   authorization_manager::check_authorization( const vector<action>&                actions,
                                               const flat_set<public_key_type>&     provided_keys,
                                               const flat_set<permission_level>&    provided_permissions,
                                               fc::microseconds                     provided_delay,
                                               const std::function<void(uint32_t)>& _checktime,
                                               bool                                 allow_unused_keys
                                             )const
   {
      const auto& checktime = ( static_cast<bool>(_checktime) ? _checktime : _noop_checktime );

      auto delay_max_limit = fc::seconds( _control.get_global_properties().configuration.max_transaction_delay );

      auto checker = make_auth_checker( [&](const permission_level& p){ return get_permission(p).auth; },
                                        permission_visitor(*this, delay_max_limit),
                                        _control.get_global_properties().configuration.max_authority_depth,
                                        provided_keys,
                                        provided_permissions,
                                        ( provided_delay >= delay_max_limit ) ? fc::microseconds::maximum() : provided_delay,
                                        checktime
                                      );

      fc::microseconds max_delay;

      map<permission_level, bool> permissions_to_satisfy;
      // bool value indicates whether the delay encountered in the check should not contribute to max_delay.

      for( const auto& act : actions ) {
         bool special_case = false;
         bool ignore_delay = false;

         if( act.account == config::system_account_name ) {
            special_case = true;

            if( act.name == updateauth::get_name() ) {
               const auto delay = check_updateauth_authorization(act.data_as<updateauth>(), act.authorization);
               if( delay.valid() ) // update auth is used to modify an existing permission
                  max_delay = std::max( max_delay, *delay );
               else // updateauth is used to create a new permission
                  ignore_delay = true;
            } else if( act.name == deleteauth::get_name() ) {
               max_delay = std::max( max_delay,
                                     check_deleteauth_authorization(act.data_as<deleteauth>(), act.authorization) );
            } else if( act.name == linkauth::get_name() ) {
               max_delay = std::max( max_delay,
                                     check_linkauth_authorization(act.data_as<linkauth>(), act.authorization) );
            } else if( act.name == unlinkauth::get_name() ) {
               max_delay = std::max( max_delay,
                                     check_unlinkauth_authorization(act.data_as<unlinkauth>(), act.authorization) );
            } else if( act.name ==  canceldelay::get_name() ) {
               check_canceldelay_authorization(act.data_as<canceldelay>(), act.authorization);
               ignore_delay = true;
            } else {
               special_case = false;
            }
         }

         for( const auto& declared_auth : act.authorization ) {

            checktime( config::base_check_authorization_cpu_per_authorization );

            if( !special_case ) {
               auto min_permission_name = lookup_minimum_permission(declared_auth.actor, act.account, act.name);
               if( min_permission_name ) { // since special cases were already handled, it should only be false if the permission is eosio.any
                  const auto& min_permission = get_permission({declared_auth.actor, *min_permission_name});
                  auto delay = get_permission(declared_auth).satisfies( min_permission,
                                                                        _db.get_index<permission_index>().indices() );
                  EOS_ASSERT( delay.valid(),
                              irrelevant_auth_exception,
                              "action declares irrelevant authority '${auth}'; minimum authority is ${min}",
                              ("auth", declared_auth)("min", permission_level{min_permission.owner, min_permission.name}) );
               }
            }

            auto res = permissions_to_satisfy.emplace( declared_auth, ignore_delay );
            if( !res.second && res.first->second ) { // if the declared_auth was already in the map and without delay tracking specified
               res.first->second = ignore_delay;
            }
         }
      }

      max_delay = std::min( max_delay, delay_max_limit );

      // Now verify that all the declared authorizations are satisfied:

      // Although this can be made parallel (especially for input transactions) with the optimistic assumption that the
      // CPU limit is not reached, because of the CPU limit the protocol must officially specify a sequential algorithm
      // for checking the set of declared authorizations.
      // The permission_levels are traversed in ascending order, which is:
      // ascending order of the actor name with ties broken by ascending order of the permission name.
      for( const auto& p : permissions_to_satisfy ) {
         checktime( config::base_authority_checker_cpu_per_permission ); // TODO: this should eventually move into authority_checker instead
         bool res = false;
         if( p.second ) {
            checker.get_permission_visitor().pause_delay_tracking();
            res = checker.satisfied( p.first, fc::microseconds::maximum() );
            checker.get_permission_visitor().resume_delay_tracking();
         } else {
            res = checker.satisfied( p.first );
         }
         EOS_ASSERT( res, tx_missing_sigs,
                     "transaction declares authority '${auth}', "
                     "but does not have signatures for it under a provided delay of ${provided_delay} ms.",
                     ("auth", p.first)("provided_delay", provided_delay.count()/1000) );

      }

      max_delay = std::max( max_delay, checker.get_permission_visitor().get_max_delay() );
      EOS_ASSERT( provided_delay >= max_delay, insufficient_delay_exception,
                  "authorization imposes a delay (${required_delay} ms) greater than the provided delay (${provided_delay} ms)",
                  ("required_delay", max_delay.count()/1000)("provided_delay", provided_delay.count()/1000) );

      if( !allow_unused_keys ) {
         EOS_ASSERT( checker.all_keys_used(), tx_irrelevant_sig,
                     "transaction bears irrelevant signatures from these keys: ${keys}",
                     ("keys", checker.unused_keys()) );
      }

      return max_delay;
   }

   fc::microseconds
   authorization_manager::check_authorization( account_name                         account,
                                               permission_name                      permission,
                                               const flat_set<public_key_type>&     provided_keys,
                                               const flat_set<permission_level>&    provided_permissions,
                                               fc::microseconds                     provided_delay,
                                               const std::function<void(uint32_t)>& _checktime,
                                               bool                                 allow_unused_keys
                                             )const
   {
      const auto& checktime = ( static_cast<bool>(_checktime) ? _checktime : _noop_checktime );

      auto delay_max_limit = fc::seconds( _control.get_global_properties().configuration.max_transaction_delay );

      auto checker = make_auth_checker( [&](const permission_level& p){ return get_permission(p).auth; },
                                        permission_visitor(*this, delay_max_limit),
                                        _control.get_global_properties().configuration.max_authority_depth,
                                        provided_keys,
                                        provided_permissions,
                                        ( provided_delay >= delay_max_limit ) ? fc::microseconds::maximum() : provided_delay,
                                        checktime
                                      );

      EOS_ASSERT( checker.satisfied({account, permission}), tx_missing_sigs, "permission was not satisfied" );

      fc::microseconds max_delay = checker.get_permission_visitor().get_max_delay();

      EOS_ASSERT( provided_delay >= max_delay, insufficient_delay_exception,
                  "permission imposes a delay (${required_delay} ms) greater than the provided delay (${provided_delay} ms)",
                  ("required_delay", max_delay.count()/1000)("provided_delay", provided_delay.count()/1000) );

      if( !allow_unused_keys ) {
         EOS_ASSERT( checker.all_keys_used(), tx_irrelevant_sig,
                     "irrelevant keys provided: ${keys}",
                     ("keys", checker.unused_keys()) );
      }

      return max_delay;
   }

   flat_set<public_key_type> authorization_manager::get_required_keys( const transaction& trx,
                                                                       const flat_set<public_key_type>& candidate_keys,
                                                                       fc::microseconds delay_threshold
                                                                     )const
   {
      auto checker = make_auth_checker( [&](const permission_level& p){ return get_permission(p).auth; },
                                        noop_permission_visitor(),
                                        _control.get_global_properties().configuration.max_authority_depth,
                                        candidate_keys,
                                        {},
                                        delay_threshold,
                                        _noop_checktime
                                      );

      for (const auto& act : trx.actions ) {
         for (const auto& declared_auth : act.authorization) {
            if( !checker.satisfied(declared_auth) ) {
               EOS_ASSERT(checker.satisfied(declared_auth), tx_missing_sigs,
                          "transaction declares authority '${auth}', but does not have signatures for it.",
                          ("auth", declared_auth));
            }
         }
      }

      return checker.used_keys();
   }

} } /// namespace eosio::chain
