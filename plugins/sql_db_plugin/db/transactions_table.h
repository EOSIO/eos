#ifndef TRANSACTIONS_TABLE_H
#define TRANSACTIONS_TABLE_H

#include <memory>
#include <soci/soci.h>

namespace eosio {

class transactions_table
{
public:
    transactions_table(std::shared_ptr<soci::session> session);

    void drop();
    void create();

private:
    std::shared_ptr<soci::session> m_session;
};

} // namespace

#endif // TRANSACTIONS_TABLE_H
