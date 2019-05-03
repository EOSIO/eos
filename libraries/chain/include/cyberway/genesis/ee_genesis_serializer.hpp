#pragma once
#include <boost/filesystem/fstream.hpp>
#include <cyberway/chaindb/common.hpp>
#include <cyberway/genesis/ee_genesis_container.hpp>
#include <eosio/chain/abi_serializer.hpp>

namespace cyberway { namespace genesis {

using namespace chaindb;
namespace bfs = boost::filesystem;

FC_DECLARE_EXCEPTION(ee_genesis_exception, 10000000, "event engine genesis create exception");

//const fc::microseconds abi_serializer_max_time = fc::seconds(10);

struct ee_genesis_serializer {

private:
    bfs::ofstream out;
    uint32_t _count = 0;
    ee_table_header _section;
    uint64_t _section_offset = 0;
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
        _section_offset = out.tellp();
        fc::raw::pack(out, h);
        _section = h;
        _count = 0;
    }

    void finish_section(uint32_t add_count = 0) {
        if (add_count != 0) {
            _section.count += add_count;

            auto pos = out.tellp();
            out.seekp(_section_offset);
            fc::raw::pack(out, _section);
            out.seekp(pos);

            wlog("Finished with count: ${count}", ("count", _section.count));
        }

        EOS_ASSERT(_count == _section.count, ee_genesis_exception, "Section contains wrong number of rows",
            ("section",_section)("diff", _section.count - _count));
    }

    void insert(const variant& v, const fc::microseconds abi_serializer_max_time = fc::seconds(10)) {
        bytes data = serializer.variant_to_binary(_section.abi_type, v, abi_serializer_max_time);
        fc::raw::pack(out, data);
        _count++;
    }
};


}} // cyberway::genesis
