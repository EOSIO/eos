#include <eosio/chain/global_property_object.hpp>

namespace eosio { namespace chain {

void dynamic_global_property_object::update_virtual_net_bandwidth( const chain_config& cfg ) {

  if( average_block_size.average()> (cfg.target_block_size * config::rate_limiting_precision) ) {
    virtual_net_bandwidth *= 99;
    virtual_net_bandwidth /= 100;
    wlog( "reducing virtual net bandwidth by 1%, ${vnb}", ("vnb", virtual_net_bandwidth) );
    wdump((average_block_size.average())(cfg.target_block_size*config::rate_limiting_precision));
  } else {
    virtual_net_bandwidth *= 1000;
    virtual_net_bandwidth /= 999;
//    ilog( "increasing virtual net bandwidth by .1%, ${vnb}", ("vnb", virtual_net_bandwidth) );
  }

  auto min = (cfg.max_block_size * config::blocksize_average_window_ms) / config::block_interval_ms;

  if( virtual_net_bandwidth <  min )         virtual_net_bandwidth = min;
  if( virtual_net_bandwidth >  min * 1000 )  virtual_net_bandwidth = min * 1000;
}

void dynamic_global_property_object::update_virtual_act_bandwidth( const chain_config& cfg ) {
  if( average_block_acts.average() > (cfg.target_block_acts * config::rate_limiting_precision) ) {
    virtual_act_bandwidth *= 99;
    virtual_act_bandwidth /= 100;
  } else {
    virtual_act_bandwidth *= 1000;
    virtual_act_bandwidth /= 999;
  }

  auto min = (cfg.max_block_acts * config::blocksize_average_window_ms) / config::block_interval_ms;

  if( virtual_act_bandwidth <  min )         virtual_act_bandwidth = min;
  if( virtual_act_bandwidth >  min * 1000 )  virtual_act_bandwidth = min * 1000;
}

} } /// eosio::chain
