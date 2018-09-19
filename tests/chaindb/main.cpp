

#include "multi_index.hpp"
#include "chaindb_control.hpp"
#include <iostream>

struct value_object {
    std::string key;
    uint64_t value;
};

struct hello_object {
    primary_key_t id;
    uint64_t age;
    std::string name;
    std::vector<value_object> values;

    primary_key_t primary_key() const {
        return id;
    }

    const std::string& by_name() const {
        return name;
    }
};

FC_REFLECT(value_object, (key)(value))
FC_REFLECT(hello_object, (id)(age)(name)(values))

using hello_index =
    chaindb::multi_index<
        N(hello), chaindb::primary_key_extractor, hello_object,
        chaindb::indexed_by<
            N(name), chaindb::const_mem_fun<hello_object, const std::string&, &hello_object::by_name>>>;

int main() {
    if (!chaindb_init("mongodb://127.0.0.1:27017")) {
        std::cerr << "Cannot connect to database" << std::endl;
        return 1;
    }

    eosio::chain::abi_def abi;

    abi.structs.emplace_back( eosio::chain::struct_def{
        "value", "",
        {{"name", "string"},
         {"value", "uint64"}}
    });

    abi.structs.emplace_back( eosio::chain::struct_def{
        "hello", "",
        {{"id", "uint64"},
         {"age", "uint64"},
         {"name", "string"},
         {"values", "value[]"}}
    });

    abi.tables.emplace_back( eosio::chain::table_def {
       "hello",
       "", {}, {}, // should be removed from abi
       "hello",
       {{"primary", {"id"}, {"asc"}},
        {"name", {"name"}, {"asc"}}}
    });

    try {
        chaindb_set_abi(N(test), abi);

        hello_index hidx(N(test), N(test));

        auto idx = hidx.get_index<N(name)>();
        auto itr = idx.find("monro");

        if (idx.end() == itr) {
            hidx.emplace(N(test), [&](auto& o) {
                o.id = hidx.available_primary_key();
                o.name = "monro";
                o.age = 10;
                o.values = {{"first", 1}, {"second", 2}};
            });
        }
    } catch (const fc::exception& e) {
        std::cerr << e.to_detail_string() << std::endl;
    }

    return 0;
}