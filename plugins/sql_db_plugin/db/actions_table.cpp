#include "actions_table.h"

#include <fc/log/logger.hpp>

namespace eosio {

actions_table::actions_table(std::shared_ptr<soci::session> session):
    m_session(session)
{

}

void actions_table::drop()
{
    try {
        *m_session << "drop table actions";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void actions_table::create()
{
    *m_session << "create table actions("
            "account TEXT,"
            "name TEXT,"
            "data TEXT)";
}

void actions_table::add(eosio::chain::action action){

    // TODO: we may do different stuff depending of the action and account (ex: sync balance, create account)
    const auto data = std::string(action.data.begin(),action.data.end());

    *m_session << "INSERT INTO actions(account, name, data) VALUES (:ac, :na, :da) ",
            soci::use(action.account.to_string()),
            soci::use(action.name.to_string()),
            soci::use(data);
}

} // namespace
