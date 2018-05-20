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
        *m_session << "DROP TABLE transactions";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void transactions_table::create()
{
    *m_session << "CREATE TABLE transactions("
            "id TEXT PRIMARY KEY,"
            "block_id TEXT,"
            "ref_block_prefix NUMERIC,"
            "expiration NUMERIC,"
            "pending NUMERIC,"
            "created_at NUMERIC,"
            "updated_at NUMERIC)";
}

void transactions_table::add(chain::transaction transaction)
{
    const auto transaction_id_str = transaction.id().str();
    const auto expiration = std::chrono::milliseconds{
                    std::chrono::seconds{transaction.expiration.sec_since_epoch()}
                }.count();

    *m_session << "INSERT INTO transactions(id, block_id, ref_block_prefix,"
            "expiration, pending, created_at, updated_at) VALUES (:id, :bi, :rb, :ex, :pe, :ca, :ua)",
            soci::use(transaction_id_str),
            soci::use(transaction.ref_block_num),
            soci::use(transaction.ref_block_prefix),
            soci::use(expiration),
            soci::use(0),
            soci::use(expiration),
            soci::use(expiration);
}

} // namespace
