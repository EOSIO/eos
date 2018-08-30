#include "blocks_table.h"

#include <fc/log/logger.hpp>
#include "zdbcpp.h"

using namespace zdbcpp;
namespace eosio {

blocks_table::blocks_table(std::shared_ptr<eosio::connection_pool> pool):
        m_pool(pool)
{

}

void blocks_table::drop()
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    ilog("blocks_table : m_pool->get_connection succeeded");
    try {
        con.execute("DROP TABLE IF EXISTS blocks;");
    }
    catch(std::exception& e){
        wlog(e.what());
    }
    m_pool->release_connection(con);
}

void blocks_table::create()
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    try {
        con.execute("CREATE TABLE blocks("
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
            "confirmed INT, UNIQUE KEY block_number (block_number)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;");

        con.execute("CREATE INDEX idx_blocks_producer ON blocks (producer);");
        con.execute("CREATE INDEX idx_blocks_number ON blocks (block_number);");
    }
    catch(std::exception& e){
        wlog(e.what());
    }
    m_pool->release_connection(con);

}



void blocks_table::add(chain::signed_block_ptr block)
{
    const auto block_id_str = block->id().str();
    const auto previous_block_id_str = block->previous.str();
    const auto transaction_mroot_str = block->transaction_mroot.str();
    const auto action_mroot_str = block->action_mroot.str();
    const auto timestamp = std::chrono::seconds{block->timestamp.operator fc::time_point().sec_since_epoch()}.count();
    const auto num_transactions = (int)block->transactions.size();
    int exist;

    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);

    try {
        ResultSet rset = con.executeQuery("SELECT COUNT(*) as cnt FROM blocks WHERE id = ? ", block_id_str.c_str());
        assert(rset);
        rset.next();

        exist = rset.getIntByName("cnt");
        if (exist > 0) {
            zdbcpp::PreparedStatement pre = con.prepareStatement("UPDATE blocks SET irreversible = '1' WHERE id = ?");
            pre.setString(1,block_id_str.c_str());
            pre.execute();
        } else {
            PreparedStatement pre = con.prepareStatement("INSERT INTO blocks(id, block_number, prev_block_id, timestamp, transaction_merkle_root, action_merkle_root,"
                        "producer, version, confirmed, num_transactions) VALUES (?, ?, ?, FROM_UNIXTIME(?), ?, ?, ?, ?, ?, ?) ");
            pre.setString(1,block_id_str.c_str());
            pre.setInt(2,block->block_num());
            pre.setString(3,previous_block_id_str.c_str());
            pre.setDouble(4,timestamp);
            pre.setString(5,transaction_mroot_str.c_str());
            pre.setString(6,action_mroot_str.c_str());
            pre.setString(7,block->producer.to_string().c_str());
            pre.setInt(8,block->schedule_version);
            pre.setInt(9,block->confirmed);
            pre.setInt(10,num_transactions);

            pre.execute();

            if (block->new_producers) {
                const auto new_producers = fc::json::to_string(block->new_producers->producers);
                PreparedStatement pre = con.prepareStatement("UPDATE blocks SET new_producers = ? WHERE id = ? ");
                pre.setString(1,new_producers.c_str());
                pre.setString(2,block_id_str.c_str());

                pre.execute();
            }
        }
    }
    catch(std::exception& e){
        wlog(e.what());
    }
    m_pool->release_connection(con);
    
    
}

void blocks_table::process_irreversible(chain::signed_block_ptr block)
{
    const auto block_id_str = block->id().str();
    int exist;

    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);

    try {
        ResultSet rset = con.executeQuery("SELECT COUNT(*) as cnt FROM blocks WHERE id = ? ", block_id_str.c_str());
        assert(rset);
        rset.next();

        exist = rset.getIntByName("cnt");

        if (exist > 0) {               
            zdbcpp::PreparedStatement pre = con.prepareStatement("UPDATE blocks SET irreversible = '1' WHERE id = ?");
            pre.setString(1,block_id_str.c_str());
            pre.execute();
        }else{
            add(block);
        }
    }
    catch(std::exception& e){
        wlog(e.what());
    }
    m_pool->release_connection(con);
}

} // namespace
