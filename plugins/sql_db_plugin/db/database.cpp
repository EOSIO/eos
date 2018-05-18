#include "database.h"

namespace eosio {

database::database(const std::string &uri)
{
    m_session = std::make_shared<soci::session>(uri);
    m_accounts_table = std::make_unique<accounts_table>(m_session);
    m_transactions_table = std::make_unique<transactions_table>(m_session);
    m_actions_table = std::make_unique<actions_table>(m_session);
    m_blocks_table = std::make_unique<blocks_table>(m_session);
}

void database::consume(const std::vector<chain::block_state_ptr> &blocks)
{
    for (const chain::block_state_ptr& block : blocks)
    {
        add(block);
        for (const chain::transaction_metadata_ptr& transaction : block->trxs) {
            add(transaction);
            for (const chain::action& action : transaction->trx.actions) {
                add(action);
            }
        }
    }
}

void database::wipe()
{
    std::unique_lock<std::mutex> lock(m_mux);

    m_actions_table->drop();
    m_transactions_table->drop();
    m_blocks_table->drop();
    m_accounts_table->drop();

    m_accounts_table->create();
    m_blocks_table->create();
    m_transactions_table->create();
    m_actions_table->create();

    m_accounts_table->insert(eosio::chain::name(chain::config::system_account_name).to_string());
}

void database::add(chain::block_state_ptr block)
{
    m_blocks_table->add(block);
}

void database::add(chain::transaction_metadata_ptr transaction)
{
    m_transactions_table->add(transaction);
}

void database::add(chain::action action)
{
    m_actions_table->add(action);
}

} // namespace
