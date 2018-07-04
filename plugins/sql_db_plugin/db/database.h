#ifndef DATABASE_H
#define DATABASE_H

#include <memory>
#include <mutex>
#include <soci/soci.h>

#include "accounts_table.h"
#include "transactions_table.h"
#include "blocks_table.h"
#include "actions_table.h"

namespace eosio {

class database
{
public:
    database(const std::string& uri);

    void wipe();

private:
    mutable std::mutex m_mux;
    std::shared_ptr<soci::session> m_session;
    std::unique_ptr<accounts_table> m_accounts_table;
    std::unique_ptr<actions_table> m_actions_table;
    std::unique_ptr<blocks_table> m_blocks_table;
    std::unique_ptr<transactions_table> m_transactions_table;
};

} // namespace

#endif // DATABASE_H
