#include <eosio/tester.hpp>

#include <boot/boot.h>
#include <system/system.h>
#include "../todo_contract.h"
#include "../filter_table.h"

#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

static const eosio::permission_level eosio_active{"eosio"_n, "active"_n};

struct todo_tester
{
    eosio::test_chain chain;
    eosio::test_rodeos rodeos;

    todo::todo_table todo_table;
    todo::filter_table filter_table;

    todo_tester() {
        // First load boot and system wasm
        // they are essentials for testing
        chain.set_code("eosio"_n, "libraries/boot/boot.wasm");
        chain.transact({boot::boot_contract::boot_action{"eosio"_n, eosio_active}.to_action()});
        chain.start_block();

        chain.set_code("eosio"_n, "libraries/system/system.wasm");
        chain.transact({sys::system_contract::init_action{"eosio"_n, eosio_active}.to_action()});

        // Loading the contract wasm
        chain.create_code_account(contract_account, true);
        setreslimit(contract_account, "disk"_n, -1);
        susetcode(contract_account, "todo_contract.wasm");

        // Loading the filter wasm
        rodeos.connect(chain);
        rodeos.add_filter("todo.filter"_n, "todo_filter.wasm");
        rodeos.enable_queries(1024 * 1024, 10, 1000, "");
    }

    eosio::transaction_trace susetcode(eosio::name ac, const char* filename, const char* expected_except = nullptr) {
        auto content_file = eosio::read_whole_file(filename);

        return chain.transact(
            {
                eosio::action{eosio_active, "eosio"_n, "susetcode"_n, std::make_tuple(ac, uint8_t{0}, uint8_t{0}, content_file)}
            },
            expected_except
        );
    }

    eosio::transaction_trace setreslimit(eosio::name account, eosio::name resource, int64_t limit, const char* expected_except = nullptr) {
        return chain.transact(
            {
                eosio::action{eosio_active, "eosio"_n, "setreslimit"_n, std::make_tuple(account, resource, limit)}
            },
            expected_except
        );
    }

    std::vector<todo::todo_item> all_items() {   
        std::vector<todo::todo_item> result;
        for (auto it = todo_table.primary_key.begin(); it != todo_table.primary_key.end(); ++it) {
            result.push_back(it.value());
        }

        return result;
    }

    todo::todo_item get_item(todo::id id) {
        // todo_table& table = todo_table::instance();
        std::optional<todo::todo_item> task = todo_table.primary_key.get(id);
        eosio::check(task.has_value(), "data does not exist");

        return std::move(*task);
    }

    std::vector<todo::filter_data> all_filter_data() {   
        std::vector<todo::filter_data> result;
        todo::filter_table& ft = todo::filter_table::instance();
        for (auto it = filter_table.primary_key.begin(); it != filter_table.primary_key.end(); ++it) {
            result.push_back(it.value());
        }

        return result;
    }
    
};

TEST_CASE("todo contract", "[todo]") {
    todo_tester t;
    eosio::name user_01 = "bob"_n;
    eosio::name user_02 = "sue"_n;
    todo::id id_creator_01{user_01, 20, 20};
    todo::id id_creator_02{user_02, 100, 100};
    todo::id id_creator_03{user_02, 100, 500};
    std::string desc_01 = "a simple description here";
    std::string desc_02 = "this is a simple description";
    std::string desc_02_edited = "this description was edited";
    std::string desc_03 = "another description";
    todo::todo_item task;
    std::vector<todo::todo_item> all_tasks;

    t.chain.create_account(user_01);
    t.chain.create_account(user_02);
    t.chain.start_block();

    t.chain.as(user_01).act<todo::actions::addtodo>(
        id_creator_01,
        desc_01
    );

    // testing task/todo item creation
    task = t.get_item(id_creator_01);
    REQUIRE(task.key == id_creator_01);
    REQUIRE(task.state == todo::todo_item::todo_state::PENDING);

    t.chain.as(user_01).act<todo::actions::changestate>(
        id_creator_01,
        todo::todo_item::todo_state::DONE
    );

    // Marked as done
    task = t.get_item(id_creator_01);
    REQUIRE(task.state == todo::todo_item::todo_state::DONE);

    t.chain.as(user_02).act<todo::actions::addtodo>(
        id_creator_02,
        desc_02
    );

    t.chain.as(user_02).act<todo::actions::addtodo>(
        id_creator_02,
        desc_02_edited
    );

    // Testing the number of records in the table
    all_tasks = t.chain.as(user_02).act<todo::actions::alltodos>();
    REQUIRE(all_tasks.size() == 2);

    // checking non-existent task
    // expect(
    //     t.chain.as("bob"_n).act<todo::actions::gettodo>(id_creator_03),
    //     "data does not exist"
    // );

    t.chain.as(user_01).act<todo::actions::addtodo>(
        id_creator_03,
        desc_03
    );

    todo::todo_item item_found = t.chain.as(user_01).act<todo::actions::gettodo>(
        id_creator_03
    );

    // getting one task
    REQUIRE(item_found.key == id_creator_03);

    all_tasks = t.all_items();
    // test how many entries are registered in the table
    REQUIRE(all_tasks.size() == 3);
    // inspect the current state of the table
    REQUIRE(
        all_tasks == 
        std::vector<todo::todo_item>{
            todo::todo_item{
                id_creator_01,
                todo::todo_item::todo_state::DONE,
                eosio::block_timestamp()
            },
            todo::todo_item{
                id_creator_02,
                todo::todo_item::todo_state::PENDING,
                eosio::block_timestamp()
            },
            todo::todo_item{
                id_creator_03,
                todo::todo_item::todo_state::PENDING,
                eosio::block_timestamp()
            }
        }
    );

    // Syncronizing Rodeos...
    // very important if we wanna start testing data from Rodeos
    t.chain.finish_block();
    t.rodeos.sync_blocks();

    // Testing data in Rodeos
    // Now we are able to test data pushed to Rodeos
    REQUIRE(t.rodeos.get_num_pushed_data() == 4);
    REQUIRE(t.rodeos.get_pushed_data_str(0) == desc_01);
    REQUIRE(t.rodeos.get_pushed_data_str(1) == desc_02);
    REQUIRE(t.rodeos.get_pushed_data_str(2) == desc_02_edited);
    REQUIRE(t.rodeos.get_pushed_data_str(3) == desc_03);

    REQUIRE(
        t.rodeos.as().act<todo::actions::gettodo>(id_creator_01) == 
        todo::todo_item{
            id_creator_01,
            todo::todo_item::todo_state::DONE,
            eosio::block_timestamp()
        }
    );

    REQUIRE(
        t.rodeos.as().act<todo::actions::gettodo>(id_creator_02) == 
        todo::todo_item{
            id_creator_02,
            todo::todo_item::todo_state::PENDING,
            eosio::block_timestamp()
        }
    );

    REQUIRE(
        t.rodeos.as().act<todo::actions::gettodo>(id_creator_03) == 
        todo::todo_item{
            id_creator_03,
            todo::todo_item::todo_state::PENDING,
            eosio::block_timestamp()
        }
    );
}