#include "accounts_table.h"

#include <fc/log/logger.hpp>

namespace eosio {

accounts_table::accounts_table(std::shared_ptr<soci::session> session):
    m_session(session)
{

}

void accounts_table::drop()
{
    try {
        *m_session << "DROP TABLE IF EXISTS accounts_keys";
        *m_session << "DROP TABLE IF EXISTS accounts";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void accounts_table::create()
{
    *m_session << "CREATE TABLE accounts("
            "name VARCHAR(12) PRIMARY KEY,"
            "abi JSON DEFAULT NULL,"
            "created_at DATETIME DEFAULT NOW(),"
            "updated_at DATETIME DEFAULT NOW()) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;";

    *m_session << "CREATE TABLE accounts_keys("
            "account VARCHAR(12),"
            "public_key VARCHAR(53),"
            "permission VARCHAR(12), FOREIGN KEY (account) REFERENCES accounts(name)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;";
}

void accounts_table::add(string name)
{
    *m_session << "INSERT INTO accounts (name) VALUES (:name)",
            soci::use(name);
}

bool accounts_table::exist(string name)
{
    int amount;
    try {
        *m_session << "SELECT COUNT(*) FROM accounts WHERE name = :name", soci::into(amount), soci::use(name);
    }
    catch (std::exception const & e)
    {
        amount = 0;
    }
    return amount > 0;
}

} // namespace
