#include "actions_table.h"

#include <fc/log/logger.hpp>

namespace eosio {

actions_table::actions_table(std::shared_ptr<soci::session> session):
    m_session(session)
{

}

void actions_table::drop()
{
    try {
        *m_session << "drop table actions";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void actions_table::create()
{
    *m_session << "create table actions("
            "id TEXT,"
            "index NUMERIC,"
            "transaction_id TEXT,"
            "handler_account_name TEXT,"
            "name TEXT,"
            "data TEXT,"
            "created_at DATETIME)";
}
} // namespace
