#pragma once

namespace eosio { namespace chain {

/**
 * @struct event
 * @brief defines event from smart-contract
 */
struct event {
    account_name   account;
    event_name     name;
    bytes          data;
};

} } // namespace eosio::chain

FC_REFLECT( eosio::chain::event, (account)(name)(data))
