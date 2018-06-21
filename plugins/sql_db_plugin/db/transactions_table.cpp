#include "transactions_table.h"

#include <chrono>
#include <fc/log/logger.hpp>

namespace eosio {

transactions_table::transactions_table(std::shared_ptr<soci::session> session):
    m_session(session)
{

}

void transactions_table::drop()
{
    try {
        *m_session << "DROP TABLE IF EXISTS transactions";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void transactions_table::create()
{
    *m_session << "CREATE TABLE transactions("
            "id VARCHAR(64) PRIMARY KEY,"
            "block_id INT NOT NULL,"
            "ref_block_num INT NOT NULL,"
            "ref_block_prefix INT,"
            "expiration DATETIME DEFAULT NOW(),"
            "pending TINYINT(1),"
            "created_at DATETIME DEFAULT NOW(),"
            "num_actions INT DEFAULT 0,"
            "updated_at DATETIME DEFAULT NOW(), FOREIGN KEY (block_id) REFERENCES blocks(block_number) ON DELETE CASCADE) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;";

    *m_session << "CREATE INDEX transactions_block_id ON transactions (block_id);";

}

void transactions_table::add(uint32_t block_id, chain::transaction transaction)
{
    const auto transaction_id_str = transaction.id().str();
    const auto expiration = std::chrono::seconds{transaction.expiration.sec_since_epoch()}.count();
    *m_session << "INSERT INTO transactions(id, block_id, ref_block_num, ref_block_prefix,"
            "expiration, pending, created_at, updated_at, num_actions) VALUES (:id, :bi, :rbi, :rb, FROM_UNIXTIME(:ex), :pe, FROM_UNIXTIME(:ca), FROM_UNIXTIME(:ua), :na)",
            soci::use(transaction_id_str),
            soci::use(block_id),
            soci::use(transaction.ref_block_num),
            soci::use(transaction.ref_block_prefix),
            soci::use(expiration),
            soci::use(0),
            soci::use(expiration),
            soci::use(expiration),
            soci::use(transaction.total_actions());
}

} // namespace
