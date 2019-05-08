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

    void start_section(account_name code, table_name name, std::string abi_type) {
        finish_section();

        ee_table_header h{code, name, abi_type, 0};
        wlog("Starting section: ${s}", ("s", h));
        _section_offset = out.tellp();
        fc::raw::pack(out, h);
        _section = h;
    }

    void finish_section(fc::optional<uint32_t> count = fc::optional<uint32_t>()) {
        if (_section_offset == 0) {
            // No section yet
            return;
        }

        if (count.valid()) {
            EOS_ASSERT(*count == _section.count, ee_genesis_exception, "Section contains wrong number of rows",
                ("section",_section)("expected",*count)("diff", _section.count - *count));
        }

        auto pos = out.tellp();
        out.seekp(_section_offset);

        fc::raw::pack(out, _section);

        out.seekp(pos);
        wlog("Finished section: ${s}", ("s", _section));

        _section_offset = 0;
    }

    void insert(const variant& v, const fc::microseconds abi_serializer_max_time = fc::seconds(10)) {
        bytes data = serializer.variant_to_binary(_section.abi_type, v, abi_serializer_max_time);
        out.write(data.data(), data.size());
        _section.count++;
    }

    template<typename T, typename Lambda>
    void emplace(Lambda&& constructor) {
        T obj(constructor, 0);
        fc::raw::pack(out, obj);
        _section.count++;
    }
};


}} // cyberway::genesis
