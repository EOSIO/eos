#include "accounts_table.h"
#include <eosio/chain/eosio_contract.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>

#include <fc/io/json.hpp>
#include <fc/utf8.hpp>
#include <fc/variant.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/chrono.hpp>
#include <fc/log/logger.hpp>

#include "zdbcpp.h"

using namespace zdbcpp;
namespace eosio {

using chain::account_name;
using chain::action_name;
using chain::block_id_type;
using chain::permission_name;
using chain::transaction;
using chain::signed_transaction;
using chain::signed_block;
using chain::transaction_id_type;
using chain::packed_transaction;

accounts_table::accounts_table(std::shared_ptr<connection_pool> pool):
    m_pool(pool)
{

}

void accounts_table::drop()
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    ilog("accounts_table : m_pool->get_connection succeeded");

    try {
        con.execute("DROP TABLE IF EXISTS accounts_control;");
        con.execute("DROP TABLE IF EXISTS accounts_keys;");
        con.execute("DROP TABLE IF EXISTS accounts;");
    }
    catch(std::exception& e){
        wlog(e.what());
    } 

    m_pool->release_connection(con);
}

void accounts_table::create()
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    try {
        con.execute("CREATE TABLE accounts("
                    "name VARCHAR(12) PRIMARY KEY,"
                    "abi JSON DEFAULT NULL,"
                    "created_at DATETIME DEFAULT NOW(),"
                    "updated_at DATETIME DEFAULT NOW()) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;");

        con.execute("CREATE TABLE accounts_keys("
                "account VARCHAR(12),"
                "public_key VARCHAR(53),"
                "permission VARCHAR(12)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;");
                
        con.execute("CREATE TABLE accounts_control("
                "controlled_account VARCHAR(12) PRIMARY KEY, "
                "controlled_permission VARCHAR(10) PRIMARY KEY, "
                "controlling_account VARCHAR(12),"
                "created_at DATETIME DEFAULT NOW()) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;");

        con.execute("CREATE INDEX idx_controlling_account ON accounts_control (controlling_account);");
    }
    catch(std::exception& e){
        wlog(e.what());
    }

    m_pool->release_connection(con);
    
}

void accounts_table::add(string account_name)
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    try {
        zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO accounts (name) VALUES (?)");
        pre.setString(1,account_name.c_str());

        pre.execute();
    }
    catch(std::exception& e) {
        wlog(e.what());
    }    
    
}

void accounts_table::add_account_control( const vector<chain::permission_level_weight>& controlling_accounts,
                                        const account_name& name, const permission_name& permission,
                                        const std::chrono::milliseconds& now)
{
    if(controlling_accounts.empty()) return;

    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    for( const auto& controlling_account : controlling_accounts ) {
        try {
            zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO accounts_control (controlled_account,controlled_permission,controlling_account,created_at) VALUES (?,?,?,?)");
            pre.setString(1,name.to_string().c_str());
            pre.setString(2,permission.to_string().c_str());
            pre.setString(3,controlling_account.permission.actor.to_string().c_str());
            pre.setDouble(4,now.count());

            pre.execute();
        }
        catch(std::exception& e) {
            wlog(e.what());
        }
    }
    
}

void accounts_table::remove_account_control( const account_name& name, const permission_name& permission )
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    try {
        zdbcpp::PreparedStatement pre = con.prepareStatement("DELETE FROM accounts_control WHERE controlled_account = ? AND controlled_permission = ? ");
        pre.setString(1,name.to_string().c_str());
        pre.setString(2,permission.to_string().c_str());
        
        pre.execute();
    }
    catch(std::exception& e) {
        wlog(e.what());
    }
    
}

void accounts_table::add_pub_keys(const vector<chain::key_weight>& keys, const account_name& name, const permission_name& permission)
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);

    for (const auto& key_weight : keys) {
        zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO accounts_keys(account, public_key, permission) VALUES (?,?,?) ");
        pre.setString(1,name.to_string().c_str()),
        pre.setString(2,key_weight.key.operator string().c_str());
        pre.setString(3,permission.to_string().c_str());

        pre.execute();
    }
}

void accounts_table::remove_pub_keys(const account_name& name, const permission_name& permission)
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);

    zdbcpp::PreparedStatement pre = con.prepareStatement("DELETE FROM accounts_keys WHERE account = ? AND permission = ?  ");
    pre.setString(1,name.to_string().c_str()),
    pre.setString(2,permission.to_string().c_str());

    pre.execute();
}

void accounts_table::update_account(chain::action action)
{
    try {
        if (action.name == chain::setabi::get_name()) {
            zdbcpp::Connection con = m_pool->get_connection();
            assert(con);

            chain::abi_def abi_setabi;
            chain::setabi action_data = action.data_as<chain::setabi>();
            chain::abi_serializer::to_abi(action_data.abi, abi_setabi);
            string abi_string = fc::json::to_string(abi_setabi);

            zdbcpp::PreparedStatement pre = con.prepareStatement("UPDATE accounts SET abi = ?, updated_at = NOW() WHERE name = ?");
            pre.setString(1,abi_string.c_str());
            pre.setString(2,action_data.account.to_string().c_str());
                    
            pre.execute();
        } else if (action.name == chain::newaccount::get_name()) {
            std::chrono::milliseconds now = std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()} );
            auto newacc = action.data_as<chain::newaccount>();

            add(newacc.name.to_string());

            add_pub_keys(newacc.owner.keys, newacc.name, chain::config::owner_name);            
            add_account_control(newacc.owner.accounts, newacc.name, chain::config::owner_name, now);
            add_pub_keys(newacc.active.keys, newacc.name, chain::config::active_name);            
            add_account_control(newacc.active.accounts, newacc.name, chain::config::active_name, now);
        } else if( action.name == chain::updateauth::get_name() ) {
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::microseconds{fc::time_point::now().time_since_epoch().count()} );
            const auto update = action.data_as<chain::updateauth>();
            remove_pub_keys(update.account, update.permission);
            remove_account_control(update.account, update.permission);
            add_pub_keys(update.auth.keys, update.account, update.permission);
            add_account_control(update.auth.accounts, update.account, update.permission, now);

        } else if( action.name == chain::deleteauth::get_name() ) {
            const auto del = action.data_as<chain::deleteauth>();
            remove_pub_keys( del.account, del.permission );
            remove_account_control(del.account, del.permission);
        }
        
    }
    catch(std::exception& e) {
        wlog(e.what());
    }    
}

bool accounts_table::exist(string name)
{
    int cnt;

    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    try {
        zdbcpp::ResultSet rset = con.executeQuery("SELECT COUNT(*) as cnt FROM accounts WHERE name = ?",name);
        assert(rset);
        rset.next();    
        cnt = rset.getIntByName("cnt");
    }
    catch(std::exception& e) {
        wlog(e.what());
        cnt = 0;
    }

    m_pool->release_connection(con);

    return cnt > 0;

    
}

} // namespace
