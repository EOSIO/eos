#ifndef ACCOUNTS_TABLE_H
#define ACCOUNTS_TABLE_H

#include <memory>
#include <soci/soci.h>

namespace eosio {

class accounts_table
{
public:
    accounts_table(std::shared_ptr<soci::session> session);

    void drop();
    void create();
    void insert(std::string name);

private:
    std::shared_ptr<soci::session> m_session;
};

} // namespace

#endif // ACCOUNTS_TABLE_H
