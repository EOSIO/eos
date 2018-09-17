

#include "multi_index.hpp"
#include <iostream>


struct hello_object {
    primary_key_t id;
    uint64_t age;
    std::string name;

    primary_key_t primary_key() const {
        return id;
    }

    const std::string& by_name() const {
        return name;
    }
};

FC_REFLECT(hello_object, (id)(age)(name))

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

    try {
        hello_index hidx(N(test), N(test));

        auto idx = hidx.get_index<N(name)>();
        auto itr = idx.find("monro");

        if (idx.end() == itr) {
            hidx.emplace(N(test), [&](auto& o) {
                o.id = hidx.available_primary_key();
                o.name = "monro";
                o.age = 10;
            });
        }
    } catch (const fc::exception& e) {
        std::cerr << e.to_detail_string() << std::endl;
    }

    return 0;
}