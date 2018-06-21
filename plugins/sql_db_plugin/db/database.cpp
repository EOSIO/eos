#include "database.h"

namespace eosio
{

database::database(const std::string &uri, uint32_t block_num_start)
{
    m_session = std::make_shared<soci::session>(uri);
    m_accounts_table = std::make_unique<accounts_table>(m_session);
    m_blocks_table = std::make_unique<blocks_table>(m_session);
    m_transactions_table = std::make_unique<transactions_table>(m_session);
    m_actions_table = std::make_unique<actions_table>(m_session);
    m_block_num_start = block_num_start;
    system_account = chain::name(chain::config::system_account_name).to_string();
}

void
database::consume(const std::vector<chain::block_state_ptr> &blocks)
{
    try {
        for (const auto &block : blocks) {
            if (m_block_num_start > 0 && block->block_num < m_block_num_start) {
                continue;
            }


            m_blocks_table->add(block->block);
            for (const auto &transaction : block->trxs) {
                m_transactions_table->add(block->block_num, transaction->trx);
                uint8_t seq = 0;
                for (const auto &action : transaction->trx.actions) {
                    try {
                        m_actions_table->add(action, transaction->trx.id(), transaction->trx.expiration, seq);
                        seq++;
                    } catch (const fc::assert_exception &ex) { // malformed actions
                        wlog("${e}", ("e", ex.what()));
                        continue;
                    }
                }
            }

        }
    } catch (const std::exception &ex) {
        elog("${e}", ("e", ex.what())); // prevent crash
    }
}

void
database::wipe()
{
    *m_session << "SET foreign_key_checks = 0;";

    m_actions_table->drop();
    m_transactions_table->drop();
    m_blocks_table->drop();
    m_accounts_table->drop();

    *m_session << "SET foreign_key_checks = 1;";

    m_accounts_table->create();
    m_blocks_table->create();
    m_transactions_table->create();
    m_actions_table->create();

    m_accounts_table->add(system_account);
}

bool
database::is_started()
{
    return m_accounts_table->exist(system_account);
}

} // namespace
