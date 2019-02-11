extern "C" {

void set_resource_limits( uint64_t account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight ) {
   EOS_ASSERT(ram_bytes >= -1, wasm_execution_error, "invalid value for ram resource limit expected [-1,INT64_MAX]");
   EOS_ASSERT(net_weight >= -1, wasm_execution_error, "invalid value for net resource weight expected [-1,INT64_MAX]");
   EOS_ASSERT(cpu_weight >= -1, wasm_execution_error, "invalid value for cpu resource weight expected [-1,INT64_MAX]");
   if( ctx().control.get_mutable_resource_limits_manager().set_account_limits(account, ram_bytes, net_weight, cpu_weight) ) {
      ctx().trx_context.validate_ram_usage.insert( account );
   }
}

void get_resource_limits( uint64_t account, int64_t* ram_bytes, int64_t* net_weight, int64_t* cpu_weight ) {
   ctx().control.get_resource_limits_manager().get_account_limits( account, *ram_bytes, *net_weight, *cpu_weight);
}

int64_t set_proposed_producers( char *packed_producer_schedule, uint32_t datalen ) {
   datastream<const char*> ds( packed_producer_schedule, datalen );
   vector<producer_key> producers;
   fc::raw::unpack(ds, producers);
   EOS_ASSERT(producers.size() <= config::max_producers, wasm_execution_error, "Producer schedule exceeds the maximum producer count for this chain");
   // check that producers are unique
   std::set<uint64_t> unique_producers;
   for (const auto& p: producers) {
      EOS_ASSERT( ctx().is_account(p.producer_name), wasm_execution_error, "producer schedule includes a nonexisting account" );
      EOS_ASSERT( p.block_signing_key.valid(), wasm_execution_error, "producer schedule includes an invalid key" );
      unique_producers.insert(p.producer_name);
   }
   EOS_ASSERT( producers.size() == unique_producers.size(), wasm_execution_error, "duplicate producer name in producer schedule" );
   return ctx().control.set_proposed_producers( std::move(producers) );
}

bool is_privileged( uint64_t n )  {
   return ctx().db.get<account_object, by_name>( n ).privileged;
}

void set_privileged( uint64_t n, bool is_priv ) {
   const auto& a = ctx().db.get<account_object, by_name>( n );
   ctx().db.modify( a, [&]( auto& ma ){
      ma.privileged = is_priv;
   });
}

void set_blockchain_parameters_packed(char* packed_blockchain_parameters, uint32_t datalen) {
   datastream<const char*> ds( packed_blockchain_parameters, datalen );
   chain::chain_config cfg;
   fc::raw::unpack(ds, cfg);
   cfg.validate();
   ctx().db.modify( ctx().control.get_global_properties(),
      [&]( auto& gprops ) {
           gprops.configuration = cfg;
   });

}

uint32_t get_blockchain_parameters_packed(char* packed_blockchain_parameters, uint32_t buffer_size) {
   auto& gpo = ctx().control.get_global_properties();

   auto s = fc::raw::pack_size( gpo.configuration );
   if( buffer_size == 0 ) return s;

   if ( s <= buffer_size ) {
      datastream<char*> ds( packed_blockchain_parameters, s );
      fc::raw::pack(ds, gpo.configuration);
      return s;
   }
   return 0;
}

void activate_feature( int64_t f ) {
   EOS_ASSERT( false, unsupported_feature, "Unsupported Hardfork Detected" );
}

}
