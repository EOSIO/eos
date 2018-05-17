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
            "block_number NUMERIC,"
            "prev_block_id TEXT,"
            "timestamp NUMERIC,"
            "transaction_merkle_root TEXT,"
            "producer_account_id TEXT,"
            "pending NUMERIC,"
            "updated_at NUMERIC)";
}

void blocks_table::add(eosio::chain::block_state_ptr block)
{
    *m_session << "INSERT INTO blocks(id, block_number, prev_block_id, timestamp, transaction_merkle_root, producer_account_id, pending, updated_at) VALUES (:id, :in, :pb, :ti, :tr, :pa, :pe, :ua)",
            soci::use(block->header.id().str()),
            soci::use(block->header.block_num()),
            soci::use(block->header.previous.str()),
            soci::use(block->header.timestamp.slot),
            soci::use(block->header.transaction_mroot.str()),
            soci::use(block->header.producer.to_string()),
            soci::use(1),
            soci::use(block->header.timestamp.slot);
}

} // namespace
