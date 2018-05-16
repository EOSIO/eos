#include "database.h"

namespace eosio {

database::database(const std::string &uri)
{
    m_session = std::make_shared<soci::session>(uri);
    m_accounts_table = std::make_unique<accounts_table>(m_session);
    m_transactions_table = std::make_unique<transactions_table>();
    m_actions_table = std::make_unique<actions_table>();
    m_blocks_table = std::make_unique<blocks_table>(m_session);
}

void database::wipe()
{
    std::unique_lock<std::mutex> lock(m_mux);

    m_accounts_table->drop();

    m_accounts_table->create();
}

} // namespace
