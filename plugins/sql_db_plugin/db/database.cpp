#include "database.h"

namespace eosio {

database::database(const std::string &uri)
{
    m_session = std::make_shared<soci::session>(uri);
    m_accounts_table = std::make_unique<accounts_table>();
    m_transactions_table = std::make_unique<transactions_table>();
    m_actions_table = std::make_unique<actions_table>();
    m_blocks_table = std::make_unique<blocks_table>(m_session);
}

} // namespace
