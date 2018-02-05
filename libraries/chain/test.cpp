#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/producer_object.hpp>
#include <fc/io/json.hpp>

using namespace eosio::chain;

int main( int argc, char** argv ) {
   try {
      chain_controller::controller_config cfg;
      cfg.block_log_dir      = "testdir/blocklog";
      cfg.shared_memory_dir  = "testdir/shared";
      cfg.shared_memory_size = 1024*1024*8;

      cfg.genesis = fc::json::from_file("genesis.json").as<contracts::genesis_state_type>();
      cfg.genesis.initial_timestamp = block_timestamp_type(fc::time_point::now());
      idump((cfg.genesis));

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


      private_key_type default_priv_key = private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("nathan")));
      public_key_type  pub_key = default_priv_key.get_public_key();


      /*
      auto private_key_default = std::make_pair(eosio::chain::public_key_type(default_priv_key.get_public_key()),
                                                eosio::utilities::key_to_wif(default_priv_key));
                                                */

      idump((pub_key)(default_priv_key));

      for( uint32_t i = 0; i < 50; ++i )
      {
         next_time = next_time + fc::milliseconds(config::block_interval_ms);
         uint32_t slot = ctrl.get_slot_at_time( next_time );
         auto sch_pro = ctrl.get_scheduled_producer(slot);
         const auto&    pro = ctrl.get_producer( sch_pro );
         auto b = ctrl.generate_block( next_time, sch_pro, default_priv_key );
         idump((b.block_num()));
     //    idump((b));
      }
      //idump((pro.signing_key));

   } catch ( const fc::exception& e ) {
      elog( "${e}", ("e",e.to_detail_string()) );
   }
   return 0;
}
