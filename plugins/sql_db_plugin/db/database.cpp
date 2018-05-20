#include "database.h"

namespace eosio {

database::database(const std::string &uri)
{
    m_session = std::make_shared<soci::session>(uri);
    m_accounts_table = std::make_unique<accounts_table>(m_session);
    m_transactions_table = std::make_unique<transactions_table>(m_session);
    m_actions_table = std::make_unique<actions_table>(m_session);
    m_blocks_table = std::make_unique<blocks_table>(m_session);

    system_account = chain::name(chain::config::system_account_name).to_string();
}

void database::consume(const std::vector<chain::block_state_ptr> &blocks)
{
    for (const auto& block : blocks)
    {
        m_blocks_table->add(block->block);
        for (const auto& transaction : block->trxs) {
            m_transactions_table->add(transaction->trx);
            for (const auto& action : transaction->trx.actions) {
                m_actions_table->add(action, transaction->trx.id());
            }
        }
    }
}

void database::wipe()
{
    m_actions_table->drop();
    m_transactions_table->drop();
    m_blocks_table->drop();
    m_accounts_table->drop();

    m_accounts_table->create();
    m_blocks_table->create();
    m_transactions_table->create();
    m_actions_table->create();

    m_accounts_table->add(system_account);
}

bool database::is_started()
{
    return m_accounts_table->exist(system_account);
}

} // namespace
