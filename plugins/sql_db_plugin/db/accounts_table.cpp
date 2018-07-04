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
    *m_session << "create table accounts("
                  "name TEXT,"
                  "eos_balance REAL,"
                  "staked_balance REAL,"
                  "unstaking_balance REAL,"
                  "abi TEXT,"
                  "created_at DATETIME,"
                  "updated_at DATETIME)";
}

} // namespace
