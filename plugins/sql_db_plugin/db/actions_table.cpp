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

void actions_table::add(chain::action action){

    // TODO: move
    if (action.name.to_string() == "setabi") {
        auto setabi = action.data_as<chain::setabi>();
        auto abi = fc::json::to_string(setabi.abi);
        auto name = setabi.account.to_string();
        *m_session << "UPDATE accounts SET abi = :abi WHERE name = :name", soci::use(abi), soci::use(name);
    }
    /*
    chain::abi_def abi;
    std::string abi_def_account;
    chain::abi_serializer abis;

    *m_session << "SELECT abi FROM accounts WHERE name = :name", soci::into(abi_def_account), soci::use(action.account.to_string());
    abi = fc::json::from_string(abi_def_account).as<chain::abi_def>();

    if (action.account == chain::config::system_account_name) {
        abi = chain::eosio_contract_abi(abi);
    }

    abis.set_abi(abi);

    auto v = abis.binary_to_variant(abis.get_action_type(action.name), action.data);
    auto json = fc::json::to_string(v); */
    std::string json = "";

    *m_session << "INSERT INTO actions(account, name, data) VALUES (:ac, :na, :da) ",
            soci::use(action.account.to_string()),
            soci::use(action.name.to_string()),
            soci::use(json);
}

} // namespace
