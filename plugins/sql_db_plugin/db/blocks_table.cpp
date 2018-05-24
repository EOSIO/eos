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
        *m_session << "DROP TABLE blocks";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void blocks_table::create()
{
    *m_session << "CREATE TABLE blocks("
            "id VARCHAR(64) PRIMARY KEY,"
            "block_number INT,"
            "prev_block_id VARCHAR(64),"
            "timestamp DATETIME DEFAULT NOW(),"
            "transaction_merkle_root VARCHAR(64),"
            "producer VARCHAR(18),"
            "confirmed INT)";
}

void blocks_table::add(chain::signed_block_ptr block)
{
    const auto block_id_str = block->id().str();
    const auto previous_block_id_str = block->previous.str();
    const auto transaction_mroot_str = block->transaction_mroot.str();
    const auto timestamp = std::chrono::seconds{block->timestamp.operator fc::time_point().sec_since_epoch()}.count();

    *m_session << "INSERT INTO blocks(id, block_number, prev_block_id, timestamp, transaction_merkle_root,"
                  "producer, confirmed) VALUES (:id, :in, :pb, FROM_UNIXTIME(:ti), :tr, :pa, :pe)",
            soci::use(block_id_str),
            soci::use(block->block_num()),
            soci::use(previous_block_id_str),
            soci::use(timestamp),
            soci::use(transaction_mroot_str),
            soci::use(block->producer.to_string()),
            soci::use(block->confirmed);
}

} // namespace
