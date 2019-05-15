#pragma once
#include "ofstream_sha256.hpp"
#include "genesis_create.hpp"
#include <cyberway/chaindb/common.hpp>
#include <cyberway/genesis/genesis_container.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>  // public interface is not enough
#include <eosio/chain/generated_transaction_object.hpp>


// #define IGNORE_SYSTEM_ABI

namespace cyberway { namespace genesis {

using namespace chaindb;
using resource_limits::resource_usage_object;
const fc::microseconds abi_serializer_max_time = fc::seconds(10);


enum class stored_contract_tables: int {
    domains,        usernames,
    permissionlink, sys_permissionlink,
    token_stats,    vesting_stats,
    token_balance,  vesting_balance,
    delegation,     rdelegation,
    withdrawal,
    witness_vote,   witness_info,
    reward_pool,    post_limits,
    gtransaction,   bandwidths,
    messages,       permlinks,
    votes,
    // the following are system tables, but it's simpler to have them here
    stake_agents,   stake_grants,
    stake_stats,    stake_params,

    _max                // to simplify setting tables_count of genesis container
};


struct genesis_serializer {
    fc::flat_map<uint16_t,uint64_t> autoincrement;
    std::map<account_name, abi_serializer> abis;

private:
    ofstream_sha256 out;
    int _row_count = 0;
    int _section_count = 0;
    table_header _section;
    bytes _buffer;

public:
    void start(const bfs::path& out_file, int n_sections) {
        out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        out.open(out_file, std::ios_base::binary);
        genesis_header hdr;
        hdr.tables_count = n_sections;
        out.write((char*)(&hdr), sizeof(hdr));
        _section_count = n_sections;
        _buffer.resize(1024*1024);
    }

    void finalize() {
        finish_section();
        EOS_ASSERT(_section_count == 0, genesis_exception, "Genesis contains wrong number of sections",
            ("diff",_section_count));
        std::cout << "Genesis hash: " << std::endl << out.hash().str() << std::endl;
    }

    void start_section(account_name code, table_name name, std::string abi_type, uint32_t count) {
        wlog("Starting section: ${s}", ("s", table_header{code, name, abi_type, count}));
        finish_section();
        table_header h{code, name, abi_type, count};
        fc::raw::pack(out, h);
        _row_count = count;
        _section = h;
        _section_count--;
    }

    void finish_section() {
        EOS_ASSERT(_row_count == 0, genesis_exception, "Section contains wrong number of rows",
            ("section",_section)("diff",_row_count));
    }

    void prepare_serializers(const contracts_map& contracts) {
        abi_def abi;
        abis[name()] = abis[config::system_account_name] =
            abi_serializer(eosio_contract_abi(abi), abi_serializer_max_time);
        for (const auto& c: contracts) {
            auto abi_bytes = c.second.abi;
            if (abi_bytes.size() > 0) {
                auto acc = c.first;
                if (abi_serializer::to_abi(abi_bytes, abi)) {
                    abis[acc] = abi_serializer(abi, abi_serializer_max_time);
                } else {
                    elog("ABI for contract ${a) not found", ("a", acc.to_string()));
                }
            }
        }
    }

    template<typename T>
    void set_autoincrement(uint64_t val) {
        constexpr auto tid = T::type_id;
        autoincrement[tid] = val;
    }

    template<typename T>
    uint64_t get_autoincrement() {
        constexpr auto tid = T::type_id;
        return autoincrement[tid];
    }

    template<typename T, typename Lambda>
    const T emplace(const name ram_payer, Lambda&& constructor) {
        T obj(constructor, 0);
        constexpr auto tid = T::type_id;
        auto& id = autoincrement[tid];
        if (obj.id == 0) {
            obj.id = id++;
        }
#ifndef IGNORE_SYSTEM_ABI
        static abi_serializer& ser = abis[name()];
        variant v;
        to_variant(obj, v);
        fc::datastream<char*> ds(_buffer.data(), _buffer.size());
        ser.variant_to_binary(_section.abi_type, v, ds, abi_serializer_max_time);
        sys_table_row record{{}, {_buffer.begin(), _buffer.begin() + ds.tellp()}};
#else
        sys_table_row record{{}, fc::raw::pack(obj)};
#endif
        fc::raw::pack(out, record);
        _row_count--;
        return obj;
    }

    void insert(primary_key_t pk, uint64_t scope, const variant& v) {   // common case where scope is owner account
        insert (pk, scope, scope, v);
    }

    void insert(primary_key_t pk, uint64_t scope, name ram_payer, const variant& v) {
        EOS_ASSERT(abis.count(_section.code) > 0, genesis_exception, "ABI not found");
        fc::datastream<char*> ds(_buffer.data(), _buffer.size());
        auto& ser = abis[_section.code];
        ser.variant_to_binary(_section.abi_type, v, ds, abi_serializer_max_time);
        fc::raw::pack(out, table_row{ram_payer, bytes{_buffer.begin(), _buffer.begin() + ds.tellp()}, pk, scope});
        _row_count--;
    }
};


}} // cyberway::genesis
