

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

    template<typename Constructor,typename Allocator>
    hello_object(Constructor&& c, Allocator a) {
        c(*this);
    }
};

FC_REFLECT(value_object, (key)(value))
FC_REFLECT(hello_object, (id)(age)(name)(values))

struct empty_allocator {
};

using hello_index =
    chaindb::multi_index<
        N(hello), chaindb::primary_key_extractor, hello_object, empty_allocator,
        chaindb::indexed_by<
            N(name), chaindb::const_mem_fun<hello_object, const std::string&, &hello_object::by_name>>>;

int main() {
    if (!chaindb_init("mongodb://127.0.0.1:27017")) {
        elog("Cannot connect to database");
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
       "hello",
       {{"primary", true, {"id"}, {"asc"}},
        {"name", false, {"name"}, {"asc"}}}
    });

    try {
        chaindb_set_abi(N(test), abi);

        hello_index hidx(N(test), N(test));

        auto idx = hidx.get_index<N(name)>();
        std::vector<std::string> names = {"monro", "kennedy", "khrushchev", "cuba", "usa", "ussr"};
        std::random_device rd;
        std::mt19937 g(rd());

        std::shuffle(names.begin(), names.end(), g);
        uint64_t age = 10;
        for (auto name: names) {
            auto itr = idx.find(name);
            if (idx.end() == itr) {
                auto nitr = hidx.emplace(N(test), [&](auto& o) {
                    o.id = hidx.available_primary_key();
                    o.name = name;
                    o.age = ++age;
                    o.values = {{"first", age + 1}, {"second", age + 2}};
                });

                ilog("Insert object: ${object}", ("object", *nitr));
            } else {
                ilog("Object is already exists: ${object}", ("object", *itr));

                hidx.modify(*itr, 0, [&](auto& o) {
                    o.age += 1;
                });

                ilog("Change age: ${object}", ("object", *itr));
            }
        }

        ilog("by name:");
        int i = 0;
        for (auto& obj: idx) {
            ilog("${idx}: ${object}", ("idx", ++i)("object", obj));
        }

        ilog("by id:");
        i = 0;
        for (auto& obj: hidx) {
            ilog("${idx}: ${object}", ("idx", ++i)("object", obj));
        }

        {
            std::shuffle(names.begin(), names.end(), g);
            auto name = names.front();

            ilog("remove ${name}", ("name", name));

            auto itr = idx.find(name);
            idx.erase(itr);
        }


        ilog("by name:");
        i = 0;
        for (auto& obj: idx) {
            ilog("${idx}: ${object}", ("idx", ++i)("object", obj));
        }

        {
            ilog("by reverse name:");
            i = 0;
            auto etr = idx.rend();
            for (auto itr = idx.rbegin(); itr != etr; ++itr) {
                ilog("${idx}: ${object}", ("idx", ++i)("object", *itr));
            }
        }

        {
            ilog("by name iterator:");

            i = 0;
            auto itr = idx.begin();
            auto etr = idx.end();

            for (; itr != etr; ++itr) {
                ilog("${idx}: ${object}", ("idx", ++i)("object", *itr));
            }

            i = 0;
            etr = idx.begin();

            ilog("by name iterator to back:");
            for (--itr; itr != etr; --itr) {
                ilog("${idx}: ${object}", ("idx", ++i)("object", *itr));
            }
            ilog("${idx}: ${object}", ("idx", ++i)("object", *itr));
        }

    } catch (const fc::exception& e) {
        elog("An error: ${what}", ("what", e));
    }

    return 0;
}
