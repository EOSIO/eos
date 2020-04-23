#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>
#include <eosio/chain/transaction_metadata.hpp>
#include <eosio/chain/transaction.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <eosio/chain/database_utils.hpp>
#include <algorithm>

namespace eosio { namespace chain { namespace resource_limits {

using resource_index_set = index_set<
   resource_limits_index,
   resource_usage_index,
   resource_limits_state_index,
   resource_limits_config_index
>;

static_assert( config::rate_limiting_precision > 0, "config::rate_limiting_precision must be positive" );

static uint64_t update_elastic_limit(uint64_t current_limit, uint64_t average_usage, const elastic_limit_parameters& params) {
   uint64_t result = current_limit;
   if (average_usage > params.target ) {
      result = result * params.contract_rate;
   } else {
      result = result * params.expand_rate;
   }
   return std::min(std::max(result, params.max), params.max * params.max_multiplier);
}

void elastic_limit_parameters::validate()const {
   // At the very least ensure parameters are not set to values that will cause divide by zero errors later on.
   // Stricter checks for sensible values can be added later.
   EOS_ASSERT( periods > 0, resource_limit_exception, "elastic limit parameter 'periods' cannot be zero" );
   EOS_ASSERT( contract_rate.denominator > 0, resource_limit_exception, "elastic limit parameter 'contract_rate' is not a well-defined ratio" );
   EOS_ASSERT( expand_rate.denominator > 0, resource_limit_exception, "elastic limit parameter 'expand_rate' is not a well-defined ratio" );
}


void resource_limits_state_object::update_virtual_cpu_limit( const resource_limits_config_object& cfg ) {
   //idump((average_block_cpu_usage.average()));
   virtual_cpu_limit = update_elastic_limit(virtual_cpu_limit, average_block_cpu_usage.average(), cfg.cpu_limit_parameters);
   //idump((virtual_cpu_limit));
}

void resource_limits_state_object::update_virtual_net_limit( const resource_limits_config_object& cfg ) {
   virtual_net_limit = update_elastic_limit(virtual_net_limit, average_block_net_usage.average(), cfg.net_limit_parameters);
}

void resource_limits_manager::add_indices() {
   resource_index_set::add_indices(_db);
}

void resource_limits_manager::initialize_database() {
   const auto& config = _db.create<resource_limits_config_object>([this](resource_limits_config_object& config){
      // see default settings in the declaration

      if (auto dm_logger = _get_deep_mind_logger()) {
         fc_dlog(*dm_logger, "RLIMIT_OP CONFIG INS ${data}",
            ("data", config)
         );
      }
   });

   _db.create<resource_limits_state_object>([this, &config](resource_limits_state_object& state){
      // see default settings in the declaration

      // start the chain off in a way that it is "congested" aka slow-start
      state.virtual_cpu_limit = config.cpu_limit_parameters.max;
      state.virtual_net_limit = config.net_limit_parameters.max;

      if (auto dm_logger = _get_deep_mind_logger()) {
         fc_dlog(*dm_logger, "RLIMIT_OP STATE INS ${data}",
            ("data", state)
         );
      }
   });
}

void resource_limits_manager::add_to_snapshot( const snapshot_writer_ptr& snapshot ) const {
   resource_index_set::walk_indices([this, &snapshot]( auto utils ){
      snapshot->write_section<typename decltype(utils)::index_t::value_type>([this]( auto& section ){
         decltype(utils)::walk(_db, [this, &section]( const auto &row ) {
            section.add_row(row, _db);
         });
      });
   });
}

void resource_limits_manager::read_from_snapshot( const snapshot_reader_ptr& snapshot, uint32_t version ) {
   resource_index_set::walk_indices([this, &snapshot, version]( auto utils ){
      using value_t = typename decltype(utils)::index_t::value_type;
      if constexpr ( !std::is_same_v<typename value_t::v3, value_t> ) {
         using v3 = typename value_t::v3;
         if ( v3::minimum_version <= version && version <= v3::maximum_version ) {
            snapshot->read_section<typename decltype(utils)::index_t::value_type>([this]( auto& section ) {
               bool more = !section.empty();
               while(more) {
                  decltype(utils)::create(_db, [this, &section, &more]( auto &row ) {
                     v3 legacy_row;
                     more = section.read_row(legacy_row, _db);
                     row.initialize_from(legacy_row);
                  });
               }
            });
            return;
         }
      }
      snapshot->read_section<typename decltype(utils)::index_t::value_type>([this]( auto& section ) {
         bool more = !section.empty();
         while(more) {
            decltype(utils)::create(_db, [this, &section, &more]( auto &row ) {
               more = section.read_row(row, _db);
            });
         }
      });
   });
}

void resource_limits_manager::initialize_account(const account_name& account) {
   _db.create<resource_limits_object>([&]( resource_limits_object& bl ) {
      bl.owner = account;

      if (auto dm_logger = _get_deep_mind_logger()) {
         fc_dlog(*dm_logger, "RLIMIT_OP ACCOUNT_LIMITS INS ${data}",
            ("data", bl)
         );
      }
   });

   _db.create<resource_usage_object>([&]( resource_usage_object& bu ) {
      bu.owner = account;

      if (auto dm_logger = _get_deep_mind_logger()) {
         fc_dlog(*dm_logger, "RLIMIT_OP ACCOUNT_USAGE INS ${data}",
            ("data", bu)
         );
      }
   });
}

void resource_limits_manager::set_block_parameters(const elastic_limit_parameters& cpu_limit_parameters, const elastic_limit_parameters& net_limit_parameters ) {
   cpu_limit_parameters.validate();
   net_limit_parameters.validate();
   const auto& config = _db.get<resource_limits_config_object>();
   if( config.cpu_limit_parameters == cpu_limit_parameters && config.net_limit_parameters == net_limit_parameters )
      return;

   _db.modify(config, [&](resource_limits_config_object& c){
      c.cpu_limit_parameters = cpu_limit_parameters;
      c.net_limit_parameters = net_limit_parameters;

      if (auto dm_logger = _get_deep_mind_logger()) {
         fc_dlog(*dm_logger, "RLIMIT_OP CONFIG UPD ${data}",
            ("data", c)
         );
      }
   });
}

void resource_limits_manager::update_account_usage(const flat_set<account_name>& accounts, uint32_t time_slot ) {
   const auto& config = _db.get<resource_limits_config_object>();
   for( const auto& a : accounts ) {
      const auto& usage = _db.get<resource_usage_object,by_owner>( a );
      _db.modify( usage, [&]( auto& bu ){
          bu.net_usage.add( 0, time_slot, config.account_net_usage_average_window );
          bu.cpu_usage.add( 0, time_slot, config.account_cpu_usage_average_window );
      });
   }
}

void resource_limits_manager::add_transaction_usage(const flat_set<account_name>& accounts, uint64_t cpu_usage, uint64_t net_usage, uint32_t time_slot ) {
   const auto& state = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();

   for( const auto& a : accounts ) {

      const auto& usage = _db.get<resource_usage_object,by_owner>( a );
      int64_t unused;
      int64_t net_weight;
      int64_t cpu_weight;
      get_account_limits( a, unused, net_weight, cpu_weight );

      _db.modify( usage, [&]( auto& bu ){
          bu.net_usage.add( net_usage, time_slot, config.account_net_usage_average_window );
          bu.cpu_usage.add( cpu_usage, time_slot, config.account_cpu_usage_average_window );

         if (auto dm_logger = _get_deep_mind_logger()) {
            fc_dlog(*dm_logger, "RLIMIT_OP ACCOUNT_USAGE UPD ${data}",
               ("data", bu)
            );
         }
      });

      if( cpu_weight >= 0 && state.total_cpu_weight > 0 ) {
         uint128_t window_size = config.account_cpu_usage_average_window;
         auto virtual_network_capacity_in_window = (uint128_t)state.virtual_cpu_limit * window_size;
         auto cpu_used_in_window                 = ((uint128_t)usage.cpu_usage.value_ex * window_size) / (uint128_t)config::rate_limiting_precision;

         uint128_t user_weight     = (uint128_t)cpu_weight;
         uint128_t all_user_weight = state.total_cpu_weight;

         auto max_user_use_in_window = (virtual_network_capacity_in_window * user_weight) / all_user_weight;

         EOS_ASSERT( cpu_used_in_window <= max_user_use_in_window,
                     tx_cpu_usage_exceeded,
                     "authorizing account '${n}' has insufficient cpu resources for this transaction",
                     ("n", name(a))
                     ("cpu_used_in_window",cpu_used_in_window)
                     ("max_user_use_in_window",max_user_use_in_window) );
      }

      if( net_weight >= 0 && state.total_net_weight > 0) {

         uint128_t window_size = config.account_net_usage_average_window;
         auto virtual_network_capacity_in_window = (uint128_t)state.virtual_net_limit * window_size;
         auto net_used_in_window                 = ((uint128_t)usage.net_usage.value_ex * window_size) / (uint128_t)config::rate_limiting_precision;

         uint128_t user_weight     = (uint128_t)net_weight;
         uint128_t all_user_weight = state.total_net_weight;

         auto max_user_use_in_window = (virtual_network_capacity_in_window * user_weight) / all_user_weight;

         EOS_ASSERT( net_used_in_window <= max_user_use_in_window,
                     tx_net_usage_exceeded,
                     "authorizing account '${n}' has insufficient net resources for this transaction",
                     ("n", name(a))
                     ("net_used_in_window",net_used_in_window)
                     ("max_user_use_in_window",max_user_use_in_window) );

      }
   }

   // account for this transaction in the block and do not exceed those limits either
   _db.modify(state, [&](resource_limits_state_object& rls){
      rls.pending_cpu_usage += cpu_usage;
      rls.pending_net_usage += net_usage;
   });

   EOS_ASSERT( state.pending_cpu_usage <= config.cpu_limit_parameters.max, block_resource_exhausted, "Block has insufficient cpu resources" );
   EOS_ASSERT( state.pending_net_usage <= config.net_limit_parameters.max, block_resource_exhausted, "Block has insufficient net resources" );
}

void resource_limits_manager::add_pending_ram_usage( const account_name account, int64_t ram_delta, const storage_usage_trace& trace ) {
   if (ram_delta == 0) {
      return;
   }

   const auto& usage  = _db.get<resource_usage_object,by_owner>( account );

   EOS_ASSERT( ram_delta <= 0 || UINT64_MAX - usage.ram_usage >= (uint64_t)ram_delta, transaction_exception,
              "Ram usage delta would overflow UINT64_MAX");
   EOS_ASSERT(ram_delta >= 0 || usage.ram_usage >= (uint64_t)(-ram_delta), transaction_exception,
              "Ram usage delta would underflow UINT64_MAX");

   _db.modify( usage, [&]( auto& u ) {
      u.ram_usage += ram_delta;

      if (auto dm_logger = _get_deep_mind_logger()) {
         fc_dlog(*dm_logger, "RAM_OP ${action_id} ${event_id} ${family} ${operation} ${legacy_tag} ${payer} ${new_usage} ${delta}",
            ("action_id", trace.action_id)
            ("event_id", trace.event_id)
            ("family", trace.family)
            ("operation", trace.operation)
            ("legacy_tag", trace.legacy_tag)
            ("payer", account)
            ("new_usage", u.ram_usage)
            ("delta", ram_delta)
         );
      }
   });
}

void resource_limits_manager::verify_account_ram_usage( const account_name account )const {
   int64_t ram_bytes; int64_t net_weight; int64_t cpu_weight;
   get_account_limits( account, ram_bytes, net_weight, cpu_weight );
   const auto& usage  = _db.get<resource_usage_object,by_owner>( account );

   if( ram_bytes >= 0 ) {
      EOS_ASSERT( usage.ram_usage <= static_cast<uint64_t>(ram_bytes), ram_usage_exceeded,
                  "account ${account} has insufficient ram; needs ${needs} bytes has ${available} bytes",
                  ("account", account)("needs",usage.ram_usage)("available",ram_bytes)              );
   }
}

void resource_limits_manager::add_pending_disk_usage( const account_name account, int64_t disk_delta, const storage_usage_trace& trace ) {
   if (disk_delta == 0) {
      return;
   }

   const auto& usage  = _db.get<resource_usage_object,by_owner>( account );

   EOS_ASSERT( disk_delta <= 0 || UINT64_MAX - usage.disk_usage >= (uint64_t)disk_delta, transaction_exception,
              "Disk usage delta would overflow UINT64_MAX");
   EOS_ASSERT(disk_delta >= 0 || usage.disk_usage >= (uint64_t)(-disk_delta), transaction_exception,
              "Disk usage delta would underflow UINT64_MAX");

   _db.modify( usage, [&]( auto& u ) {
     u.disk_usage += disk_delta;

      if (auto dm_logger = _get_deep_mind_logger()) {
         fc_dlog(*dm_logger, "DISK_OP ${action_id} ${event_id} ${family} ${operation} ${payer} ${new_usage} ${delta}",
            ("action_id", trace.action_id)
            ("event_id", trace.event_id)
            ("family", trace.family)
            ("operation", trace.operation)
            ("payer", account)
            ("new_usage", u.disk_usage)
            ("delta", disk_delta)
         );
      }
   });
}

void resource_limits_manager::verify_account_disk_usage( const account_name account ) const {
   int64_t disk_bytes = get_account_disk_limit( account );

   if( disk_bytes >= 0 ) {
      const auto& usage  = _db.get<resource_usage_object,by_owner>( account );
      EOS_ASSERT( usage.disk_usage <= static_cast<uint64_t>(disk_bytes), disk_usage_exceeded,
                  "account ${account} has insufficient disk; needs ${needs} bytes has ${available} bytes",
                  ("account", account)("needs",usage.disk_usage)("available",disk_bytes)              );
   }
}

int64_t resource_limits_manager::get_account_ram_usage( const account_name& name )const {
   return _db.get<resource_usage_object,by_owner>( name ).ram_usage;
}

int64_t resource_limits_manager::get_account_disk_usage( const account_name& name )const {
   return _db.get<resource_usage_object,by_owner>( name ).disk_usage;
}

const resource_limits_object& resource_limits_manager::get_or_create_pending_account_limits( const account_name& account ) {
   /*
    * Since we need to delay these until the next resource limiting boundary, these are created in a "pending"
    * state or adjusted in an existing "pending" state.  The chain controller will collapse "pending" state into
    * the actual state at the next appropriate boundary.
    */
   const auto* pending_limits = _db.find<resource_limits_object, by_owner>( boost::make_tuple(true, account) );
   if (pending_limits == nullptr) {
      const auto& limits = _db.get<resource_limits_object, by_owner>( boost::make_tuple(false, account));
      return _db.create<resource_limits_object>([&](resource_limits_object& pending_limits){
         pending_limits.owner = limits.owner;
         pending_limits.ram_bytes = limits.ram_bytes;
         pending_limits.net_weight = limits.net_weight;
         pending_limits.cpu_weight = limits.cpu_weight;
         pending_limits.disk_bytes = limits.disk_bytes;
         pending_limits.pending = true;
      });
   } else {
     return *pending_limits;
   }
}

bool resource_limits_manager::set_account_limits( const account_name& account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight) {
   // update the users weights directly
   auto& limits = get_or_create_pending_account_limits( account );

   bool decreased_limit = false;

   if( ram_bytes >= 0 ) {
      decreased_limit = ( (limits.ram_bytes < 0) || (ram_bytes < limits.ram_bytes) );
   }

   _db.modify( limits, [&]( resource_limits_object& pending_limits ){
      pending_limits.ram_bytes = ram_bytes;
      pending_limits.net_weight = net_weight;
      pending_limits.cpu_weight = cpu_weight;

      if (auto dm_logger = _get_deep_mind_logger()) {
         fc_dlog(*dm_logger, "RLIMIT_OP ACCOUNT_LIMITS UPD ${data}",
            ("data", pending_limits)
         );
      }
   });

   return decreased_limit;
}

const resource_limits_object& resource_limits_manager::get_account_limits( const account_name& account ) const {
   const auto* pending_buo = _db.find<resource_limits_object,by_owner>( boost::make_tuple(true, account) );
   if (pending_buo) {
      return *pending_buo;
   } else {
      const auto& buo = _db.get<resource_limits_object,by_owner>( boost::make_tuple( false, account ) );
      return buo;
   }
}

void resource_limits_manager::get_account_limits( const account_name& account, int64_t& ram_bytes, int64_t& net_weight, int64_t& cpu_weight ) const {
   const auto& buo = get_account_limits(account);
   ram_bytes  = buo.ram_bytes;
   net_weight = buo.net_weight;
   cpu_weight = buo.cpu_weight;
}

bool resource_limits_manager::set_account_disk_limit( const account_name& account, int64_t disk_bytes ) {
   auto& limits = get_or_create_pending_account_limits( account );
   bool decreased_limit = disk_bytes >= 0 && (limits.disk_bytes < 0 || disk_bytes < limits.disk_bytes);
   _db.modify(limits, [disk_bytes]( resource_limits_object& pending_limits ) {
      pending_limits.disk_bytes = disk_bytes;
   });
   return decreased_limit;
}

int64_t resource_limits_manager::get_account_disk_limit( const account_name& account ) const {
   return get_account_limits(account).disk_bytes;
}


void resource_limits_manager::process_account_limit_updates() {
   auto& multi_index = _db.get_mutable_index<resource_limits_index>();
   auto& by_owner_index = multi_index.indices().get<by_owner>();

   // convenience local lambda to reduce clutter
   auto update_state_and_value = [](uint64_t &total, int64_t &value, int64_t pending_value, const char* debug_which) -> void {
      if (value > 0) {
         EOS_ASSERT(total >= static_cast<uint64_t>(value), rate_limiting_state_inconsistent, "underflow when reverting old value to ${which}", ("which", debug_which));
         total -= value;
      }

      if (pending_value > 0) {
         EOS_ASSERT(UINT64_MAX - total >= static_cast<uint64_t>(pending_value), rate_limiting_state_inconsistent, "overflow when applying new value to ${which}", ("which", debug_which));
         total += pending_value;
      }

      value = pending_value;
   };

   const auto& state = _db.get<resource_limits_state_object>();
   _db.modify(state, [&](resource_limits_state_object& rso){
      while(!by_owner_index.empty()) {
         const auto& itr = by_owner_index.lower_bound(boost::make_tuple(true));
         if (itr == by_owner_index.end() || itr->pending!= true) {
            break;
         }

         const auto& actual_entry = _db.get<resource_limits_object, by_owner>(boost::make_tuple(false, itr->owner));
         _db.modify(actual_entry, [&](resource_limits_object& rlo){
            update_state_and_value(rso.total_ram_bytes,  rlo.ram_bytes,  itr->ram_bytes, "ram_bytes");
            update_state_and_value(rso.total_disk_bytes, rlo.disk_bytes, itr->disk_bytes, "disk_bytes");
            update_state_and_value(rso.total_cpu_weight, rlo.cpu_weight, itr->cpu_weight, "cpu_weight");
            update_state_and_value(rso.total_net_weight, rlo.net_weight, itr->net_weight, "net_weight");
         });

         multi_index.remove(*itr);
      }

      if (auto dm_logger = _get_deep_mind_logger()) {
         fc_dlog(*dm_logger, "RLIMIT_OP STATE UPD ${data}",
            ("data", state)
         );
      }
   });
}

void resource_limits_manager::process_block_usage(uint32_t block_num) {
   const auto& s = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();
   _db.modify(s, [&](resource_limits_state_object& state){
      // apply pending usage, update virtual limits and reset the pending

      state.average_block_cpu_usage.add(state.pending_cpu_usage, block_num, config.cpu_limit_parameters.periods);
      state.update_virtual_cpu_limit(config);
      state.pending_cpu_usage = 0;

      state.average_block_net_usage.add(state.pending_net_usage, block_num, config.net_limit_parameters.periods);
      state.update_virtual_net_limit(config);
      state.pending_net_usage = 0;

      if (auto dm_logger = _get_deep_mind_logger()) {
         fc_dlog(*dm_logger, "RLIMIT_OP STATE UPD ${data}",
            ("data", state)
         );
      }
   });

}

uint64_t resource_limits_manager::get_virtual_block_cpu_limit() const {
   const auto& state = _db.get<resource_limits_state_object>();
   return state.virtual_cpu_limit;
}

uint64_t resource_limits_manager::get_virtual_block_net_limit() const {
   const auto& state = _db.get<resource_limits_state_object>();
   return state.virtual_net_limit;
}

uint64_t resource_limits_manager::get_block_cpu_limit() const {
   const auto& state = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();
   return config.cpu_limit_parameters.max - state.pending_cpu_usage;
}

uint64_t resource_limits_manager::get_block_net_limit() const {
   const auto& state = _db.get<resource_limits_state_object>();
   const auto& config = _db.get<resource_limits_config_object>();
   return config.net_limit_parameters.max - state.pending_net_usage;
}

std::pair<int64_t, bool> resource_limits_manager::get_account_cpu_limit( const account_name& name, uint32_t greylist_limit ) const {
   auto [arl, greylisted] = get_account_cpu_limit_ex(name, greylist_limit);
   return {arl.available, greylisted};
}

std::pair<account_resource_limit, bool>
resource_limits_manager::get_account_cpu_limit_ex( const account_name& name, uint32_t greylist_limit, const fc::optional<block_timestamp_type>& current_time) const {

   const auto& state = _db.get<resource_limits_state_object>();
   const auto& usage = _db.get<resource_usage_object, by_owner>(name);
   const auto& config = _db.get<resource_limits_config_object>();

   int64_t cpu_weight, x, y;
   get_account_limits( name, x, y, cpu_weight );

   if( cpu_weight < 0 || state.total_cpu_weight == 0 ) {
      return {{ -1, -1, -1, block_timestamp_type(usage.cpu_usage.last_ordinal), -1 }, false};
   }

   account_resource_limit arl;

   uint128_t window_size = config.account_cpu_usage_average_window;

   bool greylisted = false;
   uint128_t virtual_cpu_capacity_in_window = window_size;
   if( greylist_limit < config::maximum_elastic_resource_multiplier ) {
      uint64_t greylisted_virtual_cpu_limit = config.cpu_limit_parameters.max * greylist_limit;
      if( greylisted_virtual_cpu_limit < state.virtual_cpu_limit ) {
         virtual_cpu_capacity_in_window *= greylisted_virtual_cpu_limit;
         greylisted = true;
      } else {
         virtual_cpu_capacity_in_window *= state.virtual_cpu_limit;
      }
   } else {
      virtual_cpu_capacity_in_window *= state.virtual_cpu_limit;
   }

   uint128_t user_weight     = (uint128_t)cpu_weight;
   uint128_t all_user_weight = (uint128_t)state.total_cpu_weight;

   auto max_user_use_in_window = (virtual_cpu_capacity_in_window * user_weight) / all_user_weight;
   auto cpu_used_in_window  = impl::integer_divide_ceil((uint128_t)usage.cpu_usage.value_ex * window_size, (uint128_t)config::rate_limiting_precision);

   if( max_user_use_in_window <= cpu_used_in_window )
      arl.available = 0;
   else
      arl.available = impl::downgrade_cast<int64_t>(max_user_use_in_window - cpu_used_in_window);

   arl.used = impl::downgrade_cast<int64_t>(cpu_used_in_window);
   arl.max = impl::downgrade_cast<int64_t>(max_user_use_in_window);
   arl.last_usage_update_time = block_timestamp_type(usage.cpu_usage.last_ordinal);
   arl.current_used = arl.used;
   if ( current_time.valid() ) {
      if (current_time->slot > usage.cpu_usage.last_ordinal) {
         auto history_usage = usage.cpu_usage;
         history_usage.add(0, current_time->slot, window_size);
         arl.current_used = impl::downgrade_cast<int64_t>(impl::integer_divide_ceil((uint128_t)history_usage.value_ex * window_size, (uint128_t)config::rate_limiting_precision));
      }
   }
   return {arl, greylisted};
}

std::pair<int64_t, bool> resource_limits_manager::get_account_net_limit( const account_name& name, uint32_t greylist_limit ) const {
   auto [arl, greylisted] = get_account_net_limit_ex(name, greylist_limit);
   return {arl.available, greylisted};
}

std::pair<account_resource_limit, bool>
resource_limits_manager::get_account_net_limit_ex( const account_name& name, uint32_t greylist_limit, const fc::optional<block_timestamp_type>& current_time) const {
   const auto& config = _db.get<resource_limits_config_object>();
   const auto& state  = _db.get<resource_limits_state_object>();
   const auto& usage  = _db.get<resource_usage_object, by_owner>(name);

   int64_t net_weight, x, y;
   get_account_limits( name, x, net_weight, y );

   if( net_weight < 0 || state.total_net_weight == 0) {
      return {{ -1, -1, -1, block_timestamp_type(usage.net_usage.last_ordinal), -1 }, false};
   }

   account_resource_limit arl;

   uint128_t window_size = config.account_net_usage_average_window;

   bool greylisted = false;
   uint128_t virtual_network_capacity_in_window = window_size;
   if( greylist_limit < config::maximum_elastic_resource_multiplier ) {
      uint64_t greylisted_virtual_net_limit = config.net_limit_parameters.max * greylist_limit;
      if( greylisted_virtual_net_limit < state.virtual_net_limit ) {
         virtual_network_capacity_in_window *= greylisted_virtual_net_limit;
         greylisted = true;
      } else {
         virtual_network_capacity_in_window *= state.virtual_net_limit;
      }
   } else {
      virtual_network_capacity_in_window *= state.virtual_net_limit;
   }

   uint128_t user_weight     = (uint128_t)net_weight;
   uint128_t all_user_weight = (uint128_t)state.total_net_weight;

   auto max_user_use_in_window = (virtual_network_capacity_in_window * user_weight) / all_user_weight;
   auto net_used_in_window  = impl::integer_divide_ceil((uint128_t)usage.net_usage.value_ex * window_size, (uint128_t)config::rate_limiting_precision);

   if( max_user_use_in_window <= net_used_in_window )
      arl.available = 0;
   else
      arl.available = impl::downgrade_cast<int64_t>(max_user_use_in_window - net_used_in_window);

   arl.used = impl::downgrade_cast<int64_t>(net_used_in_window);
   arl.max = impl::downgrade_cast<int64_t>(max_user_use_in_window);
   arl.last_usage_update_time = block_timestamp_type(usage.net_usage.last_ordinal);
   arl.current_used = arl.used;
   if ( current_time.valid() ) {
      if (current_time->slot > usage.net_usage.last_ordinal) {
         auto history_usage = usage.net_usage;
         history_usage.add(0, current_time->slot, window_size);
         arl.current_used = impl::downgrade_cast<int64_t>(impl::integer_divide_ceil((uint128_t)history_usage.value_ex * window_size, (uint128_t)config::rate_limiting_precision));
      }
   }
   return {arl, greylisted};
}

} } } /// eosio::chain::resource_limits
