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

void actions_table::add(chain::action action)
{

    chain::abi_def abi;
    std::string abi_def_account;
    chain::abi_serializer abis;

    *m_session << "SELECT abi FROM accounts WHERE name = :name", soci::into(abi_def_account), soci::use(action.account.to_string());
    if (!abi_def_account.empty()) {
        abi = fc::json::from_string(abi_def_account).as<chain::abi_def>();
    } else if (action.account == chain::config::system_account_name) {
        abi = chain::eosio_contract_abi(abi);
    }

    abis.set_abi(abi);

    auto v = abis.binary_to_variant(abis.get_action_type(action.name), action.data);
    string json = fc::json::to_string(v);

    *m_session << "INSERT INTO actions(account, name, data) VALUES (:ac, :na, :da) ",
            soci::use(action.account.to_string()),
            soci::use(action.name.to_string()),
            soci::use(json);

    // TODO: move & catch eosio.token -> transfer
    if (action.account != chain::name(chain::config::system_account_name)) {
        return;
    }

    if (action.name == chain::setabi::get_name()) {
        auto action_data = action.data_as<chain::setabi>();
        chain::abi_serializer::to_abi(action_data.abi, abi);
        string abi_string = fc::json::to_string(abi);

        *m_session << "UPDATE accounts SET abi = :abi WHERE name = :name",
                soci::use(abi_string),
                soci::use(action_data.account.to_string());

    } else if (action.name == chain::newaccount::get_name()) {
        auto action_data = action.data_as<chain::newaccount>();
        *m_session << "INSERT INTO accounts VALUES (:name, 0, 0, 0, '', strftime('%s','now'), strftime('%s','now'))",
                soci::use(action_data.name.to_string());

    }
}

} // namespace
