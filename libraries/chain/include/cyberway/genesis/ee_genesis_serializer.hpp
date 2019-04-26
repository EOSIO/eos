#pragma once
#include <boost/filesystem/fstream.hpp>
#include <cyberway/chaindb/common.hpp>
#include <cyberway/genesis/ee_genesis_container.hpp>
#include <eosio/chain/abi_serializer.hpp>

namespace cyberway { namespace genesis {

using namespace chaindb;
namespace bfs = boost::filesystem;

//const fc::microseconds abi_serializer_max_time = fc::seconds(10);

struct ee_genesis_serializer {

private:
    bfs::ofstream out;
    int _row_count = 0;
    ee_table_header _section;
    abi_serializer serializer;

public:
    void start(const bfs::path& out_file, const fc::sha256& hash, const abi_def& abi, 
            const fc::microseconds abi_serializer_max_time = fc::seconds(10)) {

        out.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        out.open(out_file, std::ios_base::binary);

        ee_genesis_header hdr;
        out.write((char*)(&hdr), sizeof(hdr));

        fc::raw::pack(out, abi);
        serializer.set_abi(abi, abi_serializer_max_time);
    }

    void finalize() {
        finish_section();
    }

    void start_section(account_name code, table_name name, std::string abi_type, uint32_t count) {
        finish_section();

        ee_table_header h{code, name, abi_type, count};
        wlog("Starting section: ${s}", ("s", h));
        fc::raw::pack(out, h);
        _row_count = count;
        _section = h;
    }

    void finish_section() {
        EOS_ASSERT(_row_count == 0, genesis_exception, "Section contains wrong number of rows",
            ("section",_section)("diff",_row_count));
    }

    void insert(const variant& v, const fc::microseconds abi_serializer_max_time = fc::seconds(10)) {
        bytes data = serializer.variant_to_binary(_section.abi_type, v, abi_serializer_max_time);
        fc::raw::pack(out, data);
        _row_count--;
    }
};


}} // cyberway::genesis
