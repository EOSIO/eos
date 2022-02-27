#include "./todo_contract.h"

namespace todo {

void todo_contract::addtodo(id todo_id, std::string description)
{
    todo_table& table = todo_table::instance();

    std::optional<todo_item> task_exist = table.primary_key.get(todo_id);
    if(!(task_exist.has_value())) {
        todo_item task{todo_id, todo_item::todo_state::PENDING, eosio::block_timestamp()};
        table.put(task);
    }
}

void todo_contract::changestate(id todo_id, uint64_t state)
{    
    todo_table& table = todo_table::instance();
    std::optional<todo_item> task = table.primary_key.get(todo_id);
    eosio::check(task.has_value(), "data does not exist");
    task->state = state;

    table.put(*task);
}

std::vector<todo::todo_item> todo_contract::alltodos() {   
    todo_table tt = todo_table::instance();
    std::vector<todo::todo_item> result;
    for (auto it = tt.primary_key.begin(); it != tt.primary_key.end(); ++it) {
        result.push_back(it.value());
    }

    return result;
}

todo_item todo_contract::gettodo(id todo_id) {
    todo_table& table = todo_table::instance();
    std::optional<todo_item> task = table.primary_key.get(todo_id);
    eosio::check(task.has_value(), "data does not exist");

    return std::move(*task);
}

}

EOSIO_ACTION_DISPATCHER(todo::actions);
