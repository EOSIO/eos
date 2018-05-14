#include "accounts_table.h"

namespace eosio {

accounts_table::accounts_table(std::shared_ptr<soci::session> session):
    m_session(session)
{

}

void accounts_table::drop()
{
    try {
        *m_session << "drop table accounts";
    }
    catch(...){

    }
}

void accounts_table::create()
{
    *m_session << "create table accounts(name TEXT)";
}

} // namespace
