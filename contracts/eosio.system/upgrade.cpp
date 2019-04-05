#include <eosio.system/eosio.system.hpp>

namespace eosiosystem {

    void system_contract::setupgrade( const eosio::upgrade_parameters& params) {
        require_auth( _self );

        (eosio::upgrade_parameters&)(_ustate) = params;
        set_upgrade_parameters( params );

        _ustate.current_version += 1;
        _ustate.target_block_num = params.target_block_num;
    }
}
