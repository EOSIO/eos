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
        *m_session << "DROP TABLE IF EXISTS blocks";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void blocks_table::create()
{
    *m_session << "CREATE TABLE blocks("
            "id VARCHAR(64) PRIMARY KEY,"
            "block_number INT NOT NULL AUTO_INCREMENT,"
            "prev_block_id VARCHAR(64),"
            "irreversible TINYINT(1) DEFAULT 0,"
            "timestamp DATETIME DEFAULT NOW(),"
            "transaction_merkle_root VARCHAR(64),"
            "action_merkle_root VARCHAR(64),"
            "producer VARCHAR(12),"
            "version INT NOT NULL DEFAULT 0,"
            "new_producers JSON DEFAULT NULL,"
            "num_transactions INT DEFAULT 0,"
            "confirmed INT, FOREIGN KEY (producer) REFERENCES accounts(name), UNIQUE KEY block_number (block_number)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;";

    *m_session << "CREATE INDEX idx_blocks_producer ON blocks (producer);";
    *m_session << "CREATE INDEX idx_blocks_number ON blocks (block_number);";

}



void blocks_table::add(chain::signed_block_ptr block)
{
    const auto block_id_str = block->id().str();
    const auto previous_block_id_str = block->previous.str();
    const auto transaction_mroot_str = block->transaction_mroot.str();
    const auto action_mroot_str = block->action_mroot.str();
    const auto timestamp = std::chrono::seconds{block->timestamp.operator fc::time_point().sec_since_epoch()}.count();
    const auto num_transactions = (int)block->transactions.size();


    *m_session << "REPLACE INTO blocks(id, block_number, prev_block_id, timestamp, transaction_merkle_root, action_merkle_root,"
            "producer, version, confirmed, num_transactions) VALUES (:id, :in, :pb, FROM_UNIXTIME(:ti), :tr, :ar, :pa, :ve, :pe, :nt)",
            soci::use(block_id_str),
            soci::use(block->block_num()),
            soci::use(previous_block_id_str),
            soci::use(timestamp),
            soci::use(transaction_mroot_str),
            soci::use(action_mroot_str),
            soci::use(block->producer.to_string()),
            soci::use(block->schedule_version),
            soci::use(block->confirmed),
            soci::use(num_transactions);

    if (block->new_producers) {
        const auto new_producers = fc::json::to_string(block->new_producers->producers);
        *m_session << "UPDATE blocks SET new_producers = :np WHERE id = :id",
                soci::use(new_producers),
                soci::use(block_id_str);
    }
}

} // namespace
