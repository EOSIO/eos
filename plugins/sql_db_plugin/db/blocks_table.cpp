#include "blocks_table.h"

#include <fc/log/logger.hpp>

namespace eosio {

blocks_table::blocks_table(std::shared_ptr<soci::session> session):
        m_session(session)
{

}

void blocks_table::drop()
{
    try {
        *m_session << "drop table blocks";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void blocks_table::create()
{
    *m_session << "CREATE TABLE blocks("
            "id TEXT PRIMARY KEY,"
            "index NUMERIC,"
            "prev_block_id TEXT,"
            "timestamp NUMERIC,"
            "transaction_merkle_root TEXT,"
            "producer_account_id TEXT,"
            "pending NUMERIC,"
            "updated_at NUMERIC)";
}

} // namespace
