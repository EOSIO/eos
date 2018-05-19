#include "accounts_table.h"

#include <fc/log/logger.hpp>

namespace eosio {

accounts_table::accounts_table(std::shared_ptr<soci::session> session):
    m_session(session)
{

}

void accounts_table::drop()
{
    try {
        *m_session << "drop table accounts";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void accounts_table::create()
{
    *m_session << "CREATE TABLE accounts("
            "name TEXT PRIMARY KEY,"
            "eos_balance REAL,"
            "staked_balance REAL,"
            "unstaking_balance REAL,"
            "abi TEXT,"
            "created_at NUMERIC,"
            "updated_at NUMERIC)";
}

void accounts_table::add(string name)
{
    *m_session << "INSERT INTO accounts VALUES (:name, 0, 0, 0, '', strftime('%s','now'), strftime('%s','now'))",
            soci::use(name);
}

bool accounts_table::exist(string name)
{
    int amount;
    *m_session << "SELECT COUNT(*) FROM accounts WHERE name = :name", soci::into(amount), soci::use(name);

    return amount > 0;
}

} // namespace
