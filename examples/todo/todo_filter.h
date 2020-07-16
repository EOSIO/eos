#pragma once

#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/varint.hpp>
#include <eosio/ship_protocol.hpp>
#include <macros.h>
#include "./todo_contract.h"
#include "./filter_table.h"

namespace ship = eosio::ship_protocol;

inline eosio::checksum256 str_to_hash(const std::string& hash_str) {
    if (hash_str.empty() || hash_str.length() != 64) {
        eosio::checksum256 hash{};
        return hash;
    }

    std::array<uint8_t, 32> bytes;
    for (unsigned int i = 0; i < hash_str.length(); i += 2) {
        std::string byte_str = hash_str.substr(i, 2);
        uint8_t     byte     = (uint8_t)strtoul(byte_str.c_str(), NULL, 16);
        bytes[i / 2]         = byte;
    }

    eosio::checksum256 hash{bytes};
    return hash;
}

inline void push_data(const void* data, uint32_t size) { 
    eosio::wasm_helpers::push_data(data, size);
}

inline void push_data(const std::string_view& v) { 
    push_data(v.data(), v.size());
}

namespace todo {

class todo_filter {
private:
    uint32_t block_num;
    ship::signed_block_v1& block;
    ship::transaction_trace_v0& ttrace;
    ship::action_trace_v1& atrace;
    eosio::name sender;

public:

    todo_filter(
        uint32_t block_num,
        ship::signed_block_v1& block,
        ship::transaction_trace_v0& ttrace,
        ship::action_trace_v1& atrace
    ) : block_num{block_num}, block{block}, ttrace{ttrace}, atrace{atrace} {
        if (atrace.creator_action_ordinal.value) {
            sender = std::get<ship::action_trace_v1>(
                ttrace.action_traces[atrace.creator_action_ordinal.value - 1]
            ).receiver;
        }
    }

    uint32_t get_block_num() { return block_num; }
    ship::signed_block_v1&  get_block() { return block; }
    ship::transaction_trace_v0& get_ttrace() { return ttrace; }
    ship::action_trace_v1& get_atrace() { return atrace; }
    eosio::name get_sender() { return sender; }

    void addtodo(todo::id todo_id, std::string description);
};

} // todo namespace

// Allowing filter member function mapping contract actions
EOSIO_FILTER_ACTIONS(
    todo::todo_filter,
    (todo, addtodo)
)

void process_table(bool present, std::variant<todo::todo_item>&& v) {
    todo::todo_item& rec = std::get<0>(v);
    eosio::print(" processing table: ""\n");
}

void process_kv_table(bool present, eosio::name table_name, eosio::name index_name, eosio::input_stream value) {
    if (table_name == "comment"_n && index_name == "primary.key"_n) {
        process_table(present, eosio::from_bin<std::variant<todo::todo_item>>(value));
    }
}

void process_deltas(ship::get_blocks_result_v0& result0) {
    auto stream = *result0.deltas;
    eosio::unsigned_int num_deltas;
    eosio::from_bin(num_deltas, stream);
    for (uint32_t i = 0; i < num_deltas.value; ++i) {
        ship::table_delta delta;
        eosio::from_bin(delta, stream);
        auto& delta0 = std::get<ship::table_delta_v0>(delta);

        if (delta0.name != "key_value") {
            continue;
        }

        for (auto& row : delta0.rows) {
            ship::key_value kv;
            eosio::from_bin(kv, row.data);
            auto& kv0 = std::get<ship::key_value_v0>(kv);

            if (kv0.key.remaining() >= 17 && kv0.key.pos[0] == 1) {
                eosio::name table_name, index_name;
                memcpy(&table_name.value, kv0.key.pos + 1, sizeof(table_name.value));
                memcpy(&index_name.value, kv0.key.pos + 9, sizeof(index_name.value));
                std::reverse((char*)(&table_name.value), (char*)(&table_name.value + 1));
                std::reverse((char*)(&index_name.value), (char*)(&index_name.value + 1));

                process_kv_table(row.present, table_name, index_name, kv0.value);
            }
        }
    }
}

extern "C" [[eosio::wasm_entry]] void apply(uint64_t, uint64_t, uint64_t) {
    ship::result result;
    eosio::convert_from_bin(result, get_input_data());
    auto& result0 = std::get<ship::get_blocks_result_v0>(result);
    if (!result0.this_block) {
        return;
    }
    
    eosio::print("filter block ", result0.this_block->block_num, "\n");
    if (result0.deltas) {
        process_deltas(result0);
    }

    if (result0.traces) {
        process_traces(result0);
    }
}
