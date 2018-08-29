#include "transactions_table.h"

#include <chrono>
#include <fc/log/logger.hpp>
#include "zdbcpp.h"

using namespace zdbcpp;
namespace eosio {

transactions_table::transactions_table(std::shared_ptr<eosio::connection_pool> pool):
    m_pool(pool)
{

}

void transactions_table::drop()
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    ilog("transactions_table : m_pool->get_connection succeeded");

    try {
        con.execute("DROP TABLE IF EXISTS transactions;");
    }
    catch(std::exception& e){
        wlog(e.what());
    }
    m_pool->release_connection(con);
}

void transactions_table::create()
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    try {
        con.execute("CREATE TABLE transactions("
                "id VARCHAR(64) PRIMARY KEY,"
                "block_id INT NOT NULL,"
                "ref_block_num INT NOT NULL,"
                "ref_block_prefix INT,"
                "expiration DATETIME DEFAULT NOW(),"
                "pending TINYINT(1),"
                "created_at DATETIME DEFAULT NOW(),"
                "num_actions INT DEFAULT 0,"
                "updated_at DATETIME DEFAULT NOW()) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;");

        con.execute("CREATE INDEX transactions_block_id ON transactions (block_id);");
    }
    catch(std::exception& e){
        wlog(e.what());
    }
    m_pool->release_connection(con);

}

void transactions_table::add(uint32_t block_id, chain::transaction transaction)
{
    const auto transaction_id_str = transaction.id().str();
    const auto expiration = std::chrono::seconds{transaction.expiration.sec_since_epoch()}.count();
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    try {
        zdbcpp::PreparedStatement p1 = con.prepareStatement("INSERT INTO transactions(id, block_id, ref_block_num, ref_block_prefix,"
            "expiration, pending, created_at, updated_at, num_actions) VALUES (?, ?, ?, ?, FROM_UNIXTIME(?), ?, FROM_UNIXTIME(?), FROM_UNIXTIME(?), ?)");
        p1.setString(1,transaction_id_str.c_str()),
        p1.setInt(2,block_id),
        p1.setInt(3,transaction.ref_block_num),
        p1.setInt(4,transaction.ref_block_prefix),
        p1.setDouble(5,expiration),
        p1.setInt(6,0),
        p1.setDouble(7,expiration),
        p1.setDouble(8,expiration),
        p1.setInt(9,transaction.total_actions());

        p1.execute();
    }
    catch(std::exception& e){
        wlog(e.what());
    }
    m_pool->release_connection(con);
}

} // namespace
