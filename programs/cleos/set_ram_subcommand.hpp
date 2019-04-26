#pragma once

#include <boost/algorithm/string.hpp>

#include <cyberway/chain/cyberway_contract_types.hpp>

struct base_ram_subcommand {
    using account_name = eosio::chain::account_name;
    using name = eosio::chain::name;
    using primary_key_t = cyberway::chaindb::primary_key_t;

    account_name  code;
    string        scope;
    name          table;
    primary_key_t pk;

    CLI::App*     command;

    base_ram_subcommand(CLI::App* root, string name, string description) {
        command = root->add_subcommand(name, description);
        command->add_option("code",    code,  localized("The account who owns the table"))->required();
        command->add_option("scope",   scope, localized("The scope within the contract in which the table is found") )->required();
        command->add_option("table",   table, localized("The name of the table as specified by the contract abi") )->required();
        command->add_option("primary", pk,    localized("The primary key of the object in the table") )->required();
    }
};

struct set_ram_payer_subcommand final: protected base_ram_subcommand {
    eosio::chain::account_name payer;

    set_ram_payer_subcommand(CLI::App* root)
    : base_ram_subcommand(root, "payer", localized("Set the RAM payer of the object")) {
        command->add_option("payer", payer, localized("The new RAM payer for the table object") )->required();
        add_standard_transaction_options(command, "payer@active");

        command->set_callback([this] {
            cyberway::chain::set_ram_payer action;
            action.code      = code;
            action.scope     = scope;
            action.table     = table;
            action.pk        = pk;
            action.new_payer = payer;

            auto payer_perms = get_account_permissions(tx_permission, {payer, config::active_name});
            send_actions({ eosio::chain::action(payer_perms, action) });
        });
    }
};

struct set_ram_state_subcommand final: protected base_ram_subcommand {
    string in_ram = "true";

    set_ram_state_subcommand(CLI::App* root)
        : base_ram_subcommand(root, "state", localized("Set the RAM state of the object")) {
        command->add_option("in_ram", in_ram, localized("If true the table object should be cached"), true );
        add_standard_transaction_options(command);

        command->set_callback([this] {
            cyberway::chain::set_ram_state action;
            action.code   = code;
            action.scope  = scope;
            action.table  = table;
            action.pk     = pk;

            boost::trim(in_ram);
            boost::to_lower(in_ram);

            if (in_ram == "true" || in_ram == "on" || in_ram == "1") {
                action.in_ram = true;
            } else if (in_ram == "false" || in_ram == "off" || in_ram == "0") {
                action.in_ram = false;
            } else {
                EOSC_ASSERT(false, "ERROR: Wrong value for in_ram parameter");
            }

            auto payer_perms = get_account_permissions(tx_permission, {});
            send_actions({ eosio::chain::action(payer_perms, action) });
        });
    }
};

struct set_ram_subcommand final {
    CLI::App* command;
    set_ram_payer_subcommand payer;
    set_ram_state_subcommand state;

    set_ram_subcommand(CLI::App* root)
    : command(root->add_subcommand("ram", localized("Change RAM state of the object"))),
      payer(command),
      state(command) {
        command->require_subcommand();
    }
};