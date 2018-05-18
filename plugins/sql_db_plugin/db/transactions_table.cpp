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

void transactions_table::add(eosio::chain::transaction_metadata_ptr transaction)
{
    const auto transaction_id_str = transaction->trx.id().str();
    const auto expiration = std::chrono::milliseconds{
                    std::chrono::seconds{transaction->trx.expiration.sec_since_epoch()}
                }.count();

    *m_session << "INSERT INTO transactions(id, sequence_num, block_id, ref_block_prefix, status,"
            "expiration, pending, created_at, type, updated_at) VALUES (:id, :se, :bi, :rb, :st, :ex, :pe, :ca, :ty, :ua)",
            soci::use(transaction_id_str),
            soci::use(transaction->trx.ref_block_num), // TODO: proper fields
            soci::use(transaction->trx.ref_block_prefix),
            soci::use(transaction_id_str),
            soci::use(expiration),
            soci::use(1),
            soci::use(expiration),
            soci::use(transaction_id_str),
            soci::use(expiration);
}

} // namespace
