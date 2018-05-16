#ifndef BLOCKS_TABLE_H
#define BLOCKS_TABLE_H

#include <memory>
#include <soci/soci.h>

namespace eosio {

class blocks_table
{
public:
    blocks_table(std::shared_ptr<soci::session> session);

private:
    std::shared_ptr<soci::session> m_session;
};

} // namespace

#endif // BLOCKS_TABLE_H
