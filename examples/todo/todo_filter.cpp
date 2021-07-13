#include "./todo_filter.h"

namespace todo {

// Pushing the description to Rodeos
void todo_filter::addtodo(todo::id todo_id, std::string description) {
    eosio::print("new todo description: ", description, "\n");

    const auto [author, time, seq] = todo_id;
    auto content_hash = author.to_string() + std::to_string(time);
    auto& ft = todo::filter_table::instance();
    ft.put(todo::filter_data{
        str_to_hash(content_hash), 
        description
    });

    push_data(description);
}

}
