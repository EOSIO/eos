#include <eosio/chain/chain_controller.hpp>

using namespace eosio::chain;

int main( int argc, char** argv ) {
   try {
      chain_controller::controller_config cfg;
      cfg.block_log_dir      = "testdir/blocklog";
      cfg.shared_memory_dir  = "testdir/shared";
      cfg.shared_memory_size = 1024*1024*8;
      cfg.genesis.initial_timestamp = block_timestamp_type(fc::time_point::now());

      chain_controller ctrl( cfg );

      auto head_time = ctrl.head_block_time();
      auto next_time = head_time + fc::milliseconds(config::block_interval_ms);
      uint32_t slot = ctrl.get_slot_at_time( next_time );

      auto str = string(next_time);
      auto tt  = fc::variant(str).as<time_point>();
      idump((str)(tt));

      idump( (ctrl.head_block_time()) );
      idump( (ctrl.head_block_num()) );
      idump( (ctrl.head_block_id()) );
      idump( (next_time)(slot)(ctrl.get_slot_at_time(next_time)) );
      idump( (ctrl.get_scheduled_producer( slot ) ) );
      idump((next_time.time_since_epoch().count()));
      next_time += fc::milliseconds(config::block_interval_ms);
      idump((next_time)(next_time.time_since_epoch().count()));

   } catch ( const fc::exception& e ) {
      elog( "${e}", ("e",e.to_detail_string()) );
   }
   return 0;
}
