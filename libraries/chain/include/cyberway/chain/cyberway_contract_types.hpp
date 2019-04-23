#pragma once

#include <eosio/chain/authority.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>

namespace cyberway { namespace chain {

    using eosio::chain::account_name;
    using eosio::chain::action_name;

    struct providebw final {
        account_name    provider;
        account_name    account;

        providebw() = default;
        providebw(const account_name& provider, const account_name& account)
            : provider(provider), account(account)
        {}

        static account_name get_account() {
            return eosio::chain::config::system_account_name;
        }

        static action_name get_name() {
            return N(providebw);
        }
    };

// TODO: requestbw
//struct requestbw final {
//    account_name    provider;
//    account_name    account;
//
//    requestbw() = default;
//    requestbw(const account_name& provider, const account_name& account)
//    : provider(provider), account(account)
//    {}
//
//    static account_name get_account() {
//        return eosio::chain::config::system_account_name;
//    }
//
//    static action_name get_name() {
//        return N(requestbw);
//    }
//};
//
//struct approvebw final {
//    account_name    account;
//
//    approvebw() = default;
//    approvebw(const account_name& account)
//    : account(account)
//    {}
//
//    static account_name get_account() {
//       return eosio::chain::config::system_account_name;
//    }
//
//    static action_name get_name() {
//        return N(approvebw);
//    }
//};

    struct provideram final {
        account_name provider;
        account_name account;

        provideram() = default;
        provideram(const account_name& provider, const account_name& account)
            : provider(provider), account(account) {
        }

        static account_name get_account() {
            return eosio::chain::config::system_account_name;
        }

        static action_name get_name() {
            return N(provideram);
        }
    };


// it's ugly, but removes boilerplate. TODO: write better
#define SYS_ACTION_STRUCT(NAME) struct NAME final { \
    NAME() = default; \
    static account_name get_account() { return eosio::chain::config::domain_account_name; } \
    static action_name get_name()     { return N(NAME); }
#define SYS_ACTION_STRUCT_END };

    SYS_ACTION_STRUCT(newdomain)
        account_name creator;
        domain_name name;
    SYS_ACTION_STRUCT_END;

    SYS_ACTION_STRUCT(passdomain)
        account_name from;
        account_name to;
        domain_name name;
    SYS_ACTION_STRUCT_END;

    SYS_ACTION_STRUCT(linkdomain)
        account_name owner;
        account_name to;
        domain_name name;
    SYS_ACTION_STRUCT_END;

    SYS_ACTION_STRUCT(unlinkdomain)
        account_name owner;
        domain_name name;
    SYS_ACTION_STRUCT_END;

    SYS_ACTION_STRUCT(newusername)
        account_name creator;
        account_name owner;
        username name;
    SYS_ACTION_STRUCT_END;

#undef SYS_ACTION_STRUCT
#undef SYS_ACTION_STRUCT_END

} } // namespace cyberway::chain

FC_REFLECT(cyberway::chain::providebw                        , (provider)(account))
//TODO: requestbw
//FC_REFLECT(cyberway::chain::requestbw                        , (provider)(account))
//FC_REFLECT(cyberway::chain::approvebw                        , (account) )
FC_REFLECT(cyberway::chain::provideram                       , (provider)(account))

FC_REFLECT(cyberway::chain::newdomain,    (creator)(name))
FC_REFLECT(cyberway::chain::passdomain,   (from)(to)(name))
FC_REFLECT(cyberway::chain::linkdomain,   (owner)(to)(name))
FC_REFLECT(cyberway::chain::unlinkdomain, (owner)(name))
FC_REFLECT(cyberway::chain::newusername,  (creator)(owner)(name))