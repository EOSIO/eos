#include "transactions_table.h"

#include <fc/log/logger.hpp>

namespace eosio {

transactions_table::transactions_table(std::shared_ptr<soci::session> session):
        m_session(session)
{

}

void transactions_table::drop()
{
    try {
        *m_session << "drop table transactions";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void transactions_table::create()
{
    *m_session << "CREATE TABLE transactions("
            "id TEXT PRIMARY KEY,"
            "sequence_num NUMERIC,"
            "block_id TEXT,"
            "ref_block_prefix NUMERIC,"
            "status TEXT,"
            "expiration NUMERIC,"
            "pending NUMERIC,"
            "created_at NUMERIC,"
            "type TEXT,"
            "updated_at DATETIME)";
}

} // namespace
