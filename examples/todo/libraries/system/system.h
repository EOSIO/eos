#pragma once

#include <eosio/action.hpp>
#include <eosio/crypto.hpp>
#include <eosio/eosio.hpp>
#include <eosio/privileged.hpp>
#include <eosio/producer_schedule.hpp>
#include "../macros.h"

namespace eosio {

struct wasm_config {
    uint32_t version                  = 0;
    uint32_t max_mutable_global_bytes = 1024;
    uint32_t max_table_elements       = 8 * 1024;
    uint32_t max_section_elements     = 8192;
    uint32_t max_linear_memory_init   = 64 * 1024;
    uint32_t max_func_local_bytes     = 8192;
    uint32_t max_nested_structures    = 1024;
    uint32_t max_symbol_bytes         = 8192;
    uint32_t max_code_bytes           = 20 * 1024 * 1024;
    uint32_t max_module_bytes         = 20 * 1024 * 1024;
    uint32_t max_pages                = 528;
    uint32_t max_call_depth           = 251;
};

struct kv_parameters {
    uint32_t version       = 0;
    uint32_t key_limit     = 1024;
    uint32_t value_limit   = 1024 * 1024;
    uint32_t max_iterators = 1024;
};

extern "C" __attribute__((eosio_wasm_import)) void
set_kv_parameters_packed(name db, const kv_parameters& packed_kv_parameters, uint32_t datalen);

extern "C" __attribute__((eosio_wasm_import)) void set_resource_limit(name account, name resource, int64_t limit);

extern "C" __attribute__((eosio_wasm_import)) void set_wasm_parameters_packed(const void*, std::size_t);

struct permission_level_weight {
    permission_level permission;
    uint16_t         weight;
};
EOSIO_REFLECT(permission_level_weight, permission, weight)

struct wait_weight {
    uint32_t wait_sec;
    uint16_t weight;
};
EOSIO_REFLECT(wait_weight, wait_sec, weight)

struct authority {
    uint32_t                             threshold = 0;
    std::vector<key_weight>              keys;
    std::vector<permission_level_weight> accounts;
    std::vector<wait_weight>             waits;
};
EOSIO_REFLECT(authority, threshold, keys, accounts, waits)

struct action_header {
    name                          account;
    name                          name;
    std::vector<permission_level> authorization;
};

} // namespace eosio

namespace sys {

class [[eosio::contract("system")]] system_contract : public eosio::contract {
  public:
    using contract::contract;

    static constexpr eosio::name owner_auth      = "owner"_n;
    static constexpr eosio::name active_auth     = "active"_n;
    static constexpr eosio::name dev_account     = "dev"_n;
    static constexpr eosio::name recover_account = "recover"_n;
    static constexpr eosio::name app_account     = "b1"_n;

    [[eosio::action]] void init() {
        require_auth(get_self());
        eosio::blockchain_parameters params;
        eosio::get_blockchain_parameters(params);
        params.max_inline_action_size    = 0xffff'ffff; // susetcode and susetabi
        params.max_block_net_usage = 4 * 1024 * 1024;
        params.max_transaction_net_usage = params.max_block_net_usage - 10;
        eosio::set_blockchain_parameters(params);
        set_kv_parameters_packed("eosio.kvram"_n, eosio::kv_parameters{}, sizeof(eosio::kv_parameters));
        set_kv_parameters_packed("eosio.kvdisk"_n, eosio::kv_parameters{}, sizeof(eosio::kv_parameters));

        eosio::wasm_config cfg;
        set_wasm_parameters_packed(&cfg, sizeof(cfg));
    }
    using init_action = eosio::action_wrapper<"init"_n, &system_contract::init>;

    [[eosio::action]] void newaccount(eosio::name creator, eosio::name name, eosio::ignore<eosio::authority> owner,
                                      eosio::ignore<eosio::authority> active) {
        if (!has_auth(get_self()))
            eosio::check(false, "newaccount is restricted on this chain");
    }
    using newaccount_action = eosio::action_wrapper<"newaccount"_n, &system_contract::newaccount>;

    [[eosio::action]] void updateauth(eosio::name account, eosio::name permission, eosio::name parent,
                                      eosio::authority auth) {
        eosio::check(permission == owner_auth || permission == active_auth,
                     "only owner and active permissions supported");
        if (account == get_self() || account == dev_account || account == app_account)
            return;
        if (is_corp(account))
            eosio::check(auth.keys.empty(), "auth.keys must be empty on corp accounts");
        else
            eosio::check(auth.accounts.empty(), "auth.accounts must be empty on user accounts");
        eosio::check(auth.waits.empty(), "auth.waits must be empty");
    }
    using updateauth_action = eosio::action_wrapper<"updateauth"_n, &system_contract::updateauth>;

    [[eosio::action]] void deleteauth(eosio::ignore<eosio::name> account, eosio::ignore<eosio::name> permission) {}
    using deleteauth_action = eosio::action_wrapper<"deleteauth"_n, &system_contract::deleteauth>;

    [[eosio::action]] void linkauth(eosio::ignore<eosio::name> account, eosio::ignore<eosio::name> code,
                                    eosio::ignore<eosio::name> type, eosio::ignore<eosio::name> requirement) {
        eosio::check(false, "linkauth is restricted on this chain");
    }
    using linkauth_action = eosio::action_wrapper<"linkauth"_n, &system_contract::linkauth>;

    [[eosio::action]] void unlinkauth(eosio::ignore<eosio::name> account, eosio::ignore<eosio::name> code,
                                      eosio::ignore<eosio::name> type) {}
    using unlinkauth_action = eosio::action_wrapper<"unlinkauth"_n, &system_contract::unlinkauth>;

    [[eosio::action]] void canceldelay(eosio::ignore<eosio::permission_level> canceling_auth,
                                       eosio::ignore<eosio::checksum256>      trx_id) {}
    using canceldelay_action = eosio::action_wrapper<"canceldelay"_n, &system_contract::canceldelay>;

    [[eosio::action]] void setcode(eosio::name account, uint8_t vmtype, uint8_t vmversion,
                                   eosio::ignore<const std::vector<char>&> code) {
        require_auth(get_self());
    }
    using setcode_action = eosio::action_wrapper<"setcode"_n, &system_contract::setcode>;

    [[eosio::action]] void setpriv(eosio::name account, uint8_t is_priv) {
        require_auth(get_self());
        set_privileged(account, is_priv);
    }
    using setpriv_action = eosio::action_wrapper<"setpriv"_n, &system_contract::setpriv>;

    [[eosio::action]] void setalimits(eosio::name account, int64_t ram_bytes, int64_t net_weight, int64_t cpu_weight) {
        require_auth(get_self());
        set_resource_limits(account, ram_bytes, net_weight, cpu_weight);
    }
    using setalimits_action = eosio::action_wrapper<"setalimits"_n, &system_contract::setalimits>;

    [[eosio::action]] void setglimits(uint64_t ram, uint64_t net, uint64_t cpu) {
        (void)ram;
        (void)net;
        (void)cpu;
        require_auth(get_self());
    }
    using setglimits_action = eosio::action_wrapper<"setglimits"_n, &system_contract::setglimits>;

    [[eosio::action]] void setreslimit(eosio::name account, eosio::name resource, int64_t limit) {
        require_auth(get_self());
        set_resource_limit(account, resource, limit);
    }
    using setreslimit_action = eosio::action_wrapper<"setreslimit"_n, &system_contract::setreslimit>;

    [[eosio::action]] void setprods(std::vector<eosio::producer_key> schedule) {
        require_auth(get_self());
        set_proposed_producers(schedule);
    }
    using setprods_action = eosio::action_wrapper<"setprods"_n, &system_contract::setprods>;

    [[eosio::action]] void setparams(const eosio::blockchain_parameters& params) {
        require_auth(get_self());
        set_blockchain_parameters(params);
    }
    using setparams_action = eosio::action_wrapper<"setparams"_n, &system_contract::setparams>;

    [[eosio::action]] void reqauth(eosio::name from) { require_auth(from); }
    using reqauth_action = eosio::action_wrapper<"reqauth"_n, &system_contract::reqauth>;

    [[eosio::action]] void activate(const eosio::checksum256& feature_digest) {
        require_auth(get_self());
        preactivate_feature(feature_digest);
    }

    [[eosio::action]] void reqactivated(const eosio::checksum256& feature_digest) {
        eosio::check(is_feature_activated(feature_digest), "protocol feature is not activated");
    }

    [[eosio::action]] void setabi(eosio::name account, const std::vector<char>& abi) {
        if (!has_auth(get_self()))
            eosio::check(false, "setabi is restricted on this chain");
        abi_hash_table table(get_self(), 0);
        auto           itr = table.find(account.value);
        if (itr == table.end()) {
            table.emplace(account, [&](auto& row) {
                row.owner = account;
                row.hash  = eosio::sha256(const_cast<char*>(abi.data()), abi.size());
            });
        } else {
            table.modify(itr, eosio::same_payer,
                         [&](auto& row) { row.hash = eosio::sha256(const_cast<char*>(abi.data()), abi.size()); });
        }
    }
    using setabi_action = eosio::action_wrapper<"setabi"_n, &system_contract::setabi>;

    static int num_dots(eosio::name n) {
        auto tmp = n.value >> 4;
        int  i;
        for (i = 0; i < 12; ++i) {
            if (tmp & 0x1f)
                break;
            tmp >>= 5;
        }
        int result = 0;
        for (; i < 12; ++i) {
            if (!(tmp & 0x1f))
                ++result;
            tmp >>= 5;
        }
        return result;
    }

    static bool is_corp(eosio::name account) { return account.suffix() == "c"_n && num_dots(account); }

    [[eosio::action]] void newuser(eosio::name user, eosio::public_key active_key) {
        if (!has_auth(get_self()))
            require_auth("signup"_n);
        newuser2(user, active_key, active_key);
    }
    using newuser_action = eosio::action_wrapper<"newuser"_n, &system_contract::newuser>;

    [[eosio::action]] void newuser2(eosio::name user, eosio::public_key owner_key, eosio::public_key active_key) {
        if (!has_auth(get_self()))
            require_auth("signup"_n);
        eosio::check(!num_dots(user), "user accounts may not have dots");
        std::vector<eosio::permission_level> perms{{get_self(), active_auth}, {"signup"_n, active_auth}};
        newaccount_action{get_self(), perms}.send(
            "signup"_n, user,
            eosio::authority{.threshold = 1,
                             .keys      = {eosio::key_weight{.key = owner_key, .weight = 1}},
                             .accounts  = {},
                             .waits     = {}},
            eosio::authority{.threshold = 1,
                             .keys      = {eosio::key_weight{.key = active_key, .weight = 1}},
                             .accounts  = {},
                             .waits     = {}});
        setalimits_action{get_self(), perms}.send(user, 4 * 1024, 1, 1);
    }
    using newuser2_action = eosio::action_wrapper<"newuser2"_n, &system_contract::newuser2>;

    [[eosio::action]] void newcorp(eosio::name corp, eosio::name member) {
        if (!has_auth(get_self()))
            require_auth(app_account);
        eosio::check(corp.suffix() == "c"_n && num_dots(corp) == 1 && corp != ".c"_n, "invalid corp name");
        std::vector<eosio::permission_level> perms{{get_self(), active_auth}, {app_account, active_auth}};
        newaccount_action{get_self(), perms}.send(
            app_account, corp,
            eosio::authority{
                .threshold = 1,
                .keys      = {},
                .accounts  = {eosio::permission_level_weight{.permission = {member, active_auth}, .weight = 1}},
                .waits     = {}},
            eosio::authority{
                .threshold = 1,
                .keys      = {},
                .accounts  = {eosio::permission_level_weight{.permission = {member, active_auth}, .weight = 1}},
                .waits     = {}});
        setalimits_action{get_self(), perms}.send(corp, 4 * 1024, 1, 1);
    }
    using newcorp_action = eosio::action_wrapper<"newcorp"_n, &system_contract::newcorp>;

    [[eosio::action]] void susetcode(eosio::name account, uint8_t vmtype, uint8_t vmversion,
                                     const eosio::ignore<std::vector<char>>& code) {
        if (!has_auth(get_self()))
            require_auth(dev_account);
        eosio::action_header header = {
            .account       = get_self(),
            .name          = "setcode"_n,
            .authorization = {{get_self(), active_auth}, {account, active_auth}},
        };
        auto serialize      = pack(header);
        auto header_size    = serialize.size();
        auto remainder_size = get_datastream().remaining();
        auto size_of_args =
            eosio::pack_size(account) + eosio::pack_size(vmtype) + eosio::pack_size(vmversion) + remainder_size;
        serialize.resize(header_size + eosio::pack_size(eosio::unsigned_int(size_of_args)) + size_of_args);
        eosio::datastream<char*> ds(serialize.data() + header_size, serialize.size() - header_size);
        ds << eosio::unsigned_int(size_of_args) << account << vmtype << vmversion;
        ds.write(get_datastream().pos(), remainder_size);
        eosio::check(ds.remaining() == 0, "serialization error in susetcode");
        eosio::internal_use_do_not_use::send_inline(serialize.data(), serialize.size());
    }
    using susetcode_action = eosio::action_wrapper<"susetcode"_n, &system_contract::susetcode>;

    [[eosio::action]] void susetabi(eosio::name account, const std::vector<char>& abi) {
        if (!has_auth(get_self()))
            require_auth(dev_account);
        std::vector<eosio::permission_level> perms{{get_self(), active_auth}, {account, active_auth}};
        setabi_action{get_self(), perms}.send(account, abi);
    }
    using susetabi_action = eosio::action_wrapper<"susetabi"_n, &system_contract::susetabi>;

    [[eosio::action]] void suupdateauth(eosio::name account, eosio::name permission, eosio::name parent,
                                        eosio::authority auth) {
        if (!has_auth(get_self()) && !has_auth(dev_account))
            require_auth(recover_account);
        eosio::check(account != get_self() && account != dev_account, "suupdateauth can't modify eosio or dev");
        std::vector<eosio::permission_level> perms{{account, owner_auth}};
        updateauth_action{get_self(), perms}.send(account, permission, parent, auth);
    }
    using suupdateauth_action = eosio::action_wrapper<"suupdateauth"_n, &system_contract::suupdateauth>;

    // Stops periodic onblock from triggering "unknown action"
    [[eosio::action]] void onblock() { require_auth(get_self()); }

    struct [[eosio::table]] abi_hash {
        eosio::name        owner;
        eosio::checksum256 hash;
        uint64_t           primary_key() const { return owner.value; }

        EOSLIB_SERIALIZE(abi_hash, (owner)(hash))
    };

    typedef eosio::multi_index<"abihash"_n, abi_hash> abi_hash_table;
};

EOSIO_DECLARE_ACTIONS(system_contract, "eosio"_n, //
                      init, newaccount, updateauth, deleteauth, linkauth, unlinkauth, canceldelay, setcode, setpriv,
                      setalimits, setglimits, setreslimit, setprods, setparams, reqauth, activate, reqactivated, setabi,
                      newuser, newuser2, newcorp, susetcode, susetabi, suupdateauth, onblock)

} // namespace sys
