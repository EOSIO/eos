#ifndef DATABASE_H
#define DATABASE_H

#include "consumer_core.h"

#include <memory>
#include <mutex>
#include <soci/soci.h>
#include <eosio/chain/config.hpp>
#include <eosio/chain/name.hpp>
#include <eosio/chain/block_state.hpp>

#include "accounts_table.h"
#include "transactions_table.h"
#include "blocks_table.h"
#include "actions_table.h"

namespace eosio {

class database : public consumer_core<chain::block_state_ptr>
{
public:
    database(const std::string& uri);

    void consume(const std::vector<chain::block_state_ptr>& blocks) override;

    void wipe();
    void add(chain::block_state_ptr block);
    void add(chain::transaction_metadata_ptr transaction);
    void add(chain::action action);

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
