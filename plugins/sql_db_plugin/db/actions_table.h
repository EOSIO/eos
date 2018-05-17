#ifndef ACTIONS_TABLE_H
#define ACTIONS_TABLE_H

#include <memory>
#include <soci/soci.h>

namespace eosio {

class actions_table
{
public:
    actions_table(std::shared_ptr<soci::session> session);

    void drop();
    void create();

private:
    std::shared_ptr<soci::session> m_session;
};

} // namespace

#endif // ACTIONS_TABLE_H
