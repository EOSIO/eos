#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/protocol_state_object.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/apply_context.hpp>

#include <vector>
#include <set>

namespace eosio { namespace chain { namespace webassembly {

   int interface::is_feature_active( int64_t feature_name ) const { return false; }

   void interface::activate_feature( int64_t feature_name ) const {
      EOS_ASSERT( false, unsupported_feature, "Unsupported Hardfork Detected" );
   }

   void interface::preactivate_feature( legacy_ptr<const digest_type> feature_digest ) {
      context.control.preactivate_feature( context.get_action_id(), *feature_digest );
   }

   /**
    * Deprecated in favor of set_resource_limit.
    */
   void interface::set_resource_limits( account_name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight ) {
      EOS_ASSERT(ram_bytes >= -1, wasm_execution_error, "invalid value for ram resource limit expected [-1,INT64_MAX]");
      EOS_ASSERT(net_weight >= -1, wasm_execution_error, "invalid value for net resource weight expected [-1,INT64_MAX]");
      EOS_ASSERT(cpu_weight >= -1, wasm_execution_error, "invalid value for cpu resource weight expected [-1,INT64_MAX]");
      if( context.control.get_mutable_resource_limits_manager().set_account_limits(account, ram_bytes, net_weight, cpu_weight) ) {
         context.trx_context.validate_ram_usage.insert( account );
      }
   }

   /**
    * Deprecated in favor of get_resource_limit.
    */
   void interface::get_resource_limits( account_name account, legacy_ptr<int64_t> ram_bytes, legacy_ptr<int64_t> net_weight, legacy_ptr<int64_t> cpu_weight ) const {
      context.control.get_resource_limits_manager().get_account_limits( account, *ram_bytes, *net_weight, *cpu_weight);
      (void)legacy_ptr<int64_t>(std::move(ram_bytes));
      (void)legacy_ptr<int64_t>(std::move(net_weight));
      (void)legacy_ptr<int64_t>(std::move(cpu_weight));
   }

   void interface::set_resource_limit( account_name account, name resource, int64_t limit ) {
      EOS_ASSERT(limit >= -1, wasm_execution_error, "invalid value for ${resource} resource limit expected [-1,INT64_MAX]", ("resource", resource));
      auto& manager = context.control.get_mutable_resource_limits_manager();
      if( resource == string_to_name("ram") ) {
         int64_t ram, net, cpu;
         manager.get_account_limits(account, ram, net, cpu);
         if( manager.set_account_limits( account, limit, net, cpu ) ) {
            context.trx_context.validate_ram_usage.insert( account );
         }
      } else if( resource == string_to_name("net") ) {
         int64_t ram, net, cpu;
         manager.get_account_limits(account, ram, net, cpu);
         manager.set_account_limits( account, ram, limit, cpu );
      } else if( resource == string_to_name("cpu") ) {
         int64_t ram, net, cpu;
         manager.get_account_limits(account, ram, net, cpu);
         manager.set_account_limits( account, ram, net, limit );
      } else {
         EOS_THROW(wasm_execution_error, "unknown resource ${resource}", ("resource", resource));
      }
   }

   int64_t interface::get_resource_limit( account_name account, name resource ) const {
      const auto& manager = context.control.get_resource_limits_manager();
      if( resource == string_to_name("ram") ) {
         int64_t ram, net, cpu;
         manager.get_account_limits( account, ram, net, cpu );
         return ram;
      } else if( resource == string_to_name("net") ) {
         int64_t ram, net, cpu;
         manager.get_account_limits( account, ram, net, cpu );
         return net;
      } else if( resource == string_to_name("cpu") ) {
         int64_t ram, net, cpu;
         manager.get_account_limits( account, ram, net, cpu );
         return cpu;
      } else {
         EOS_THROW(wasm_execution_error, "unknown resource ${resource}", ("resource", resource));
      }
   }

   int64_t set_proposed_producers_common( apply_context& context, vector<producer_authority> && producers, bool validate_keys ) {
      EOS_ASSERT(producers.size() <= config::max_producers, wasm_execution_error, "Producer schedule exceeds the maximum producer count for this chain");
      EOS_ASSERT( producers.size() > 0
                  || !context.control.is_builtin_activated( builtin_protocol_feature_t::disallow_empty_producer_schedule ),
                  wasm_execution_error,
                  "Producer schedule cannot be empty"
      );

      const int64_t num_supported_key_types = context.db.get<protocol_state_object>().num_supported_key_types;

      // check that producers are unique
      std::set<account_name> unique_producers;
      for (const auto& p: producers) {
         EOS_ASSERT( context.is_account(p.producer_name), wasm_execution_error, "producer schedule includes a nonexisting account" );
         std::visit([&p, num_supported_key_types, validate_keys](const auto& a) {
            uint32_t sum_weights = 0;
            std::set<public_key_type> unique_keys;
            for (const auto& kw: a.keys ) {
               EOS_ASSERT( kw.key.which() < num_supported_key_types, unactivated_key_type,
                           "Unactivated key type used in proposed producer schedule");

               if( validate_keys ) {
                  EOS_ASSERT( kw.key.valid(), wasm_execution_error, "producer schedule includes an invalid key" );
               }

               if (std::numeric_limits<uint32_t>::max() - sum_weights <= kw.weight) {
                  sum_weights = std::numeric_limits<uint32_t>::max();
               } else {
                  sum_weights += kw.weight;
               }

               unique_keys.insert(kw.key);
            }

            EOS_ASSERT( a.keys.size() == unique_keys.size(), wasm_execution_error, "producer schedule includes a duplicated key for ${account}", ("account", p.producer_name));
            EOS_ASSERT( a.threshold > 0, wasm_execution_error, "producer schedule includes an authority with a threshold of 0 for ${account}", ("account", p.producer_name));
            EOS_ASSERT( sum_weights >= a.threshold, wasm_execution_error, "producer schedule includes an unsatisfiable authority for ${account}", ("account", p.producer_name));
         }, p.authority);

         unique_producers.insert(p.producer_name);
      }
      EOS_ASSERT( producers.size() == unique_producers.size(), wasm_execution_error, "duplicate producer name in producer schedule" );

      return context.control.set_proposed_producers( std::move(producers) );
   }

   uint32_t interface::get_wasm_parameters_packed( span<char> packed_parameters, uint32_t max_version ) const {
      auto& gpo = context.control.get_global_properties();
      auto& params = gpo.wasm_configuration;
      uint32_t version = std::min( max_version, uint32_t(0) );

      auto s = fc::raw::pack_size( params );

      if(context.control.is_builtin_activated( builtin_protocol_feature_t::get_wasm_parameters_packed_fix )) {
         s += fc::raw::pack_size( version );
      }

      if ( packed_parameters.size() == 0 )
         return s;

      if ( s <= packed_parameters.size() ) {
         datastream<char*> ds( packed_parameters.data(), s );
         fc::raw::pack(ds, version);
         fc::raw::pack(ds, params);
      }
      return s;
   }
   void interface::set_wasm_parameters_packed( span<const char> packed_parameters ) {
      datastream<const char*> ds( packed_parameters.data(), packed_parameters.size() );
      uint32_t version;
      chain::wasm_config cfg;
      fc::raw::unpack(ds, version);
      EOS_ASSERT(version == 0, wasm_config_unknown_version, "set_wasm_parameters_packed: Unknown version: ${version}", ("version", version));
      fc::raw::unpack(ds, cfg);
      cfg.validate();
      context.db.modify( context.control.get_global_properties(),
         [&]( auto& gprops ) {
            gprops.wasm_configuration = cfg;
         }
      );
   }
   int64_t interface::set_proposed_producers( legacy_span<const char> packed_producer_schedule) {
      datastream<const char*> ds( packed_producer_schedule.data(), packed_producer_schedule.size() );
      std::vector<producer_authority> producers;
      std::vector<legacy::producer_key> old_version;
      fc::raw::unpack(ds, old_version);

      /*
       * Up-convert the producers
       */
      for ( const auto& p : old_version ) {
         producers.emplace_back( producer_authority{ p.producer_name, block_signing_authority_v0{ 1, {{p.block_signing_key, 1}} } } );
      }

      return set_proposed_producers_common( context, std::move(producers), true );
   }

   int64_t interface::set_proposed_producers_ex( uint64_t packed_producer_format, legacy_span<const char> packed_producer_schedule) {
      if (packed_producer_format == 0) {
         return set_proposed_producers(std::move(packed_producer_schedule));
      } else if (packed_producer_format == 1) {
         datastream<const char*> ds( packed_producer_schedule.data(), packed_producer_schedule.size() );
         vector<producer_authority> producers;

         fc::raw::unpack(ds, producers);
         return set_proposed_producers_common( context, std::move(producers), false);
      } else {
         EOS_THROW(wasm_execution_error, "Producer schedule is in an unknown format!");
      }
   }

   uint32_t interface::get_blockchain_parameters_packed( legacy_span<char> packed_blockchain_parameters ) const {
      auto& gpo = context.control.get_global_properties();

      auto s = fc::raw::pack_size( gpo.configuration.v0() );
      if( packed_blockchain_parameters.size() == 0 ) return s;

      if ( s <= packed_blockchain_parameters.size() ) {
         datastream<char*> ds( packed_blockchain_parameters.data(), s );
         fc::raw::pack(ds, gpo.configuration.v0());
         return s;
      }
      return 0;
   }

   void interface::set_blockchain_parameters_packed( legacy_span<const char> packed_blockchain_parameters ) {
      datastream<const char*> ds( packed_blockchain_parameters.data(), packed_blockchain_parameters.size() );
      chain::chain_config_v0 cfg;
      fc::raw::unpack(ds, cfg);
      cfg.validate();
      context.db.modify( context.control.get_global_properties(),
         [&]( auto& gprops ) {
              gprops.configuration = cfg;
      });
   }
   
   enum {
      max_block_net_usage_id,
      target_block_net_usage_pct_id,
      max_transaction_net_usage_id,
      base_per_transaction_net_usage_id,
      net_usage_leeway_id,
      context_free_discount_net_usage_num_id,
      context_free_discount_net_usage_den_id,
      max_block_cpu_usage_id,
      target_block_cpu_usage_pct_id,
      max_transaction_cpu_usage_id,
      min_transaction_cpu_usage_id,
      max_transaction_lifetime_id,
      deferred_trx_expiration_window_id,
      max_transaction_delay_id,
      max_inline_action_size_id,
      max_inline_action_depth_id,
      max_authority_depth_id,
      max_action_return_value_size_id
   };

   inline std::variant<uint16_t, uint32_t, uint64_t> __get_data_by_id(uint32_t id, const chain::chain_config & cfg){
      std::variant<uint16_t, uint32_t, uint64_t> data;
        switch (id){
         case max_block_net_usage_id:
            data = cfg.max_block_net_usage;
            break;
         case target_block_net_usage_pct_id:
            data = cfg.target_block_net_usage_pct;
            break;
         case max_transaction_net_usage_id:
            data = cfg.max_transaction_net_usage;
            break;
         case base_per_transaction_net_usage_id:
            data = cfg.base_per_transaction_net_usage;
            break;
         case net_usage_leeway_id:
            data = cfg.net_usage_leeway;
            break;
         case context_free_discount_net_usage_num_id:
            data = cfg.context_free_discount_net_usage_num;
            break;
         case context_free_discount_net_usage_den_id:
            data = cfg.context_free_discount_net_usage_den;
            break;
         case max_block_cpu_usage_id:
            data = cfg.max_block_cpu_usage;
            break;
         case target_block_cpu_usage_pct_id:
            data = cfg.target_block_cpu_usage_pct;
            break;
         case max_transaction_cpu_usage_id:
            data = cfg.max_transaction_cpu_usage;
            break;
         case min_transaction_cpu_usage_id:
            data = cfg.min_transaction_cpu_usage;
            break;
         case max_transaction_lifetime_id:
            data = cfg.max_transaction_lifetime;
            break;
         case deferred_trx_expiration_window_id:
            data = cfg.deferred_trx_expiration_window;
            break;
         case max_transaction_delay_id:
            data = cfg.max_transaction_delay;
            break;
         case max_inline_action_size_id:
            data = cfg.max_inline_action_size;
            break;
         case max_inline_action_depth_id:
            data = cfg.max_inline_action_depth;
            break;
         case max_authority_depth_id:
            data = cfg.max_authority_depth;
            break;
         case max_action_return_value_size_id:
            data = cfg.max_action_return_value_size;
            break;
      }
      return data;
   }

   uint32_t interface::get_parameters_packed( span<const char> packed_parameter_ids, span<char> packed_parameters) const{
      datastream<const char*> ds_ids( packed_parameter_ids.data(), packed_parameter_ids.size() );

      chain::chain_config cfg = context.control.get_global_properties().configuration;
      std::vector<uint32_t> ids;
      uint32_t size;
      fc::raw::unpack(ds_ids, size);
      for(uint32_t i = 0; i < size; ++i){
         uint32_t id;
         fc::raw::unpack(ds_ids, id);
         ids.push_back(id);
      }
      const int estimate_params_buff_size = 256;
      char paras_buff[estimate_params_buff_size];
      datastream<char*> ds_paras( paras_buff, sizeof(paras_buff) );
      fc::raw::pack(ds_paras, size);
      for( auto  id : ids){
         uint32_t _id = id;
         std::variant<uint16_t, uint32_t, uint64_t>  var = __get_data_by_id(id, cfg);
         fc::raw::pack(ds_paras, _id);
         fc::raw::pack(ds_paras, var);
      }
      // if no output buffer, return the the least output buffer size required.
      if( packed_parameters.size() == 0 ) return ds_paras.tellp();
      EOS_ASSERT(ds_paras.tellp() <= packed_parameters.size(),
                 chain::config_parse_error,
                 "get_parameters_packed: output buffer size is smaller than ${size}", ("size", ds_paras.tellp()));
      
      std::memcpy(packed_parameters.data(), paras_buff, ds_paras.tellp());
      return ds_paras.tellp();
   }

   inline void __set_parameter_id_data(chain::chain_config & cfg, uint32_t id, const std::variant<uint16_t, uint32_t, uint64_t> & data){
      switch (id){
         case max_block_net_usage_id:
            cfg.max_block_net_usage = std::get<uint64_t>(data);
            break;
         case target_block_net_usage_pct_id:
            cfg.target_block_net_usage_pct = std::get<uint32_t>(data);
            break;
         case max_transaction_net_usage_id:
            cfg.max_transaction_net_usage = std::get<uint32_t>(data);
            break;
         case base_per_transaction_net_usage_id:
            cfg.base_per_transaction_net_usage = std::get<uint32_t>(data);
            break;
         case net_usage_leeway_id:
            cfg.net_usage_leeway = std::get<uint32_t>(data);
            break;
         case context_free_discount_net_usage_num_id:
            cfg.context_free_discount_net_usage_num = std::get<uint32_t>(data);
            break;
         case context_free_discount_net_usage_den_id:
            cfg.context_free_discount_net_usage_den = std::get<uint32_t>(data);
            break;
         case max_block_cpu_usage_id:
            cfg.max_block_cpu_usage = std::get<uint32_t>(data);
            break;
         case target_block_cpu_usage_pct_id:
            cfg.target_block_cpu_usage_pct = std::get<uint32_t>(data);
            break;
         case max_transaction_cpu_usage_id:
            cfg.max_transaction_cpu_usage = std::get<uint32_t>(data);
            break;
         case min_transaction_cpu_usage_id:
            cfg.min_transaction_cpu_usage = std::get<uint32_t>(data);
            break;
         case max_transaction_lifetime_id:
            cfg.max_transaction_lifetime = std::get<uint32_t>(data);
            break;
         case deferred_trx_expiration_window_id:
            cfg.deferred_trx_expiration_window = std::get<uint32_t>(data);
            break;
         case max_transaction_delay_id:
            cfg.max_transaction_delay = std::get<uint32_t>(data);
            break;
         case max_inline_action_size_id:
            cfg.max_inline_action_size = std::get<uint32_t>(data);
            break;
         case max_inline_action_depth_id:
            cfg.max_inline_action_depth  = std::get<uint16_t>(data);
            break;
         case max_authority_depth_id:
            cfg.max_authority_depth = std::get<uint16_t>(data);
            break;
         case max_action_return_value_size_id:
            cfg.max_action_return_value_size = std::get<uint32_t>(data);
            break;
      }
   }

   void interface::set_parameters_packed( span<const char> packed_parameters ){
      datastream<const char*> ds( packed_parameters.data(), packed_parameters.size() );
      chain::chain_config cfg = context.control.get_global_properties().configuration;

      uint32_t size;
      fc::raw::unpack(ds, size);
      for(uint32_t i = 0; i < size; ++i){
         uint32_t id;
         fc::raw::unpack(ds, id);
         std::variant<uint16_t, uint32_t, uint64_t>  var;
         fc::raw::unpack(ds, var);
         __set_parameter_id_data(cfg, id, var);
      }
      
      cfg.validate();
      context.db.modify( context.control.get_global_properties(),
         [&]( auto& gprops ) {
              gprops.configuration = cfg;
      });
   }

   uint32_t interface::get_kv_parameters_packed( span<char> packed_kv_parameters, uint32_t max_version ) const {
      const auto& gpo = context.control.get_global_properties();
      const auto& params = gpo.kv_configuration;
      uint32_t version = std::min( max_version, uint32_t(0) );

      auto s = fc::raw::pack_size( version ) + fc::raw::pack_size( params );

      if ( s <= packed_kv_parameters.size() ) {
         datastream<char*> ds( packed_kv_parameters.data(), s );
         fc::raw::pack(ds, version);
         fc::raw::pack(ds, params);
      }
      return s;
   }

   void interface::set_kv_parameters_packed( span<const char> packed_kv_parameters ) {
      datastream<const char*> ds( packed_kv_parameters.data(), packed_kv_parameters.size() );
      uint32_t version;
      chain::kv_database_config cfg;
      fc::raw::unpack(ds, version);
      EOS_ASSERT(version == 0, kv_unknown_parameters_version, "set_kv_parameters_packed: Unknown version: ${version}", ("version", version));
      fc::raw::unpack(ds, cfg);
      context.db.modify( context.control.get_global_properties(),
         [&]( auto& gprops ) {
               gprops.kv_configuration = cfg;
      });
   }

   bool interface::is_privileged( account_name n ) const {
      return context.db.get<account_metadata_object, by_name>( n ).is_privileged();
   }

   void interface::set_privileged( account_name n, bool is_priv ) {
      const auto& a = context.db.get<account_metadata_object, by_name>( n );
      context.db.modify( a, [&]( auto& ma ){
         ma.set_privileged( is_priv );
      });
   }
}}} // ns eosio::chain::webassembly
