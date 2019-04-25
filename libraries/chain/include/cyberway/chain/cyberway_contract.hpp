#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/contract_types.hpp>

namespace eosio { namespace chain {
    class apply_context;
} } // namespace eosio::chain

namespace cyberway { namespace chain {
    using eosio::chain::apply_context;

    struct set_ram_state;

    /**
     * @defgroup native_action_handlers Native Action Handlers
     */
    ///@{
    // TODO: requestbw
    // void apply_cyber_requestbw(apply_context&);

    void apply_cyber_providebw(apply_context&);
    void apply_cyber_provideram(apply_context&);

    void apply_cyber_domain_newdomain(apply_context&);
    void apply_cyber_domain_passdomain(apply_context&);
    void apply_cyber_domain_linkdomain(apply_context&);
    void apply_cyber_domain_unlinkdomain(apply_context&);
    void apply_cyber_domain_newusername(apply_context&);

    /*
    void apply_cyber_postrecovery(apply_context&);
    void apply_cyber_passrecovery(apply_context&);
    void apply_cyber_vetorecovery(apply_context&);
    */

    void apply_cyber_setrampayer(apply_context&);
    void apply_cyber_setramstate(apply_context&);

    void change_ram_state(apply_context&, const set_ram_state&);

} } // namespace cyberway::chain