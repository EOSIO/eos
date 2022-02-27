#pragma once

#include <eosio/eosio.hpp>
#include <macros.h>

static constexpr auto contract_account = "todo"_n;

namespace todo {

struct id {
    eosio::name author = {};
    uint32_t time = 0;
    uint32_t seq = 0;
};
EOSIO_REFLECT(id, author, time, seq)
EOSIO_COMPARE(id);

struct todo_item
{
    enum todo_state {
        PENDING = 0x00,
        DONE = 0x01,
        REMOVED = 0x02
    };

    id key;
    uint64_t state;
    eosio::block_timestamp created_at;
    // Let's put the description in the filter table
    // std::string description;

    id primary_key() const { return key; }
};
EOSIO_REFLECT(todo_item, key, created_at, state);
EOSIO_COMPARE(todo_item);

struct todo_table : eosio::kv_table<todo_item>
{
    KV_NAMED_INDEX("primary.key", primary_key)

    todo_table()
    {
        init(contract_account, "todo"_n, eosio::kv_disk, primary_key);
    }

    static auto& instance()
    {
        static todo_table t;
        return t;
    }
};

/**
 * Main todo contract
 * 
 * @description This contract is the next step of a hello world
 * We can use it as reference how to add filters
 * and use the KV tables
 * 
 * Create a pool of tasks, you can edit them and mark them as done or remove them
 * 
 * @TODO: adding queries and push data to Rabbit Queue
 * 
 */
class [[eosio::contract("todo_contract")]] todo_contract :
public eosio::contract
{
public:
    using contract::contract;

    /**
     * Create or Modify a task
     *
     * @param id value to be checked for equality
     * @param description trask content, this is going to the rodeos table
     */
    [[eosio::action]] void addtodo(id todo_id, std::string description);

    /**
     * Create or Modify a task
     *
     * @param id value to be checked for equality
     * @param state new status as checked or deleted 
     */
    [[eosio::action]] void changestate(id todo_id, uint64_t state);

    /**
     * Create or Modify a task
     *
     * @param id value to be checked for equality
     * @param state new status as checked or deleted 
     */
    [[eosio::action]] std::vector<todo_item> alltodos();

    /**
     * Create or Modify a task
     *
     * @param id value to be checked for equality
     * @param state new status as checked or deleted 
     */
    [[eosio::action]] todo_item gettodo(id todo_id);
    
};
// Set the contract's actions inside the "actions" namespace
// So we can access/test them in an easier way
EOSIO_DECLARE_ACTIONS(
    todo_contract,
    contract_account,
    addtodo,
    changestate,
    alltodos,
    gettodo
)

} // todo namespace
