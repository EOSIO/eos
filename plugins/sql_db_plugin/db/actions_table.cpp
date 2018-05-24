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
        *m_session << "drop table IF EXISTS actions_accounts";
        *m_session << "drop table IF EXISTS tokens";
        *m_session << "drop table IF EXISTS actions";
    }
    catch(std::exception& e){
        wlog(e.what());
    }
}

void actions_table::create()
{
    *m_session << "CREATE TABLE actions("
            "id VARCHAR(36) PRIMARY KEY,"
            "account VARCHAR(12),"
            "transaction_id VARCHAR(64),"
            "name VARCHAR(12),"
            "data JSON, FOREIGN KEY (transaction_id) REFERENCES transactions(id),"
            "FOREIGN KEY (account) REFERENCES accounts(name))";

    *m_session << "CREATE TABLE actions_accounts("
            "actor VARCHAR(12),"
            "permission VARCHAR(12),"
            "action_id VARCHAR(36), FOREIGN KEY (action_id) REFERENCES actions(id),"
            "FOREIGN KEY (actor) REFERENCES accounts(name))";

    // TODO: move to own class
    *m_session << "CREATE TABLE tokens("
            "account VARCHAR(13),"
            "symbol VARCHAR(10),"
            "amount FLOAT,"
            "staked FLOAT, FOREIGN KEY (account) REFERENCES accounts(name))"; // NOT WORKING VERY GOOD float issue
}

void actions_table::add(chain::action action, chain::transaction_id_type transaction_id)
{

    chain::abi_def abi;
    std::string abi_def_account;
    chain::abi_serializer abis;
    soci::indicator ind;
    const auto transaction_id_str = transaction_id.str();

    *m_session << "SELECT abi FROM accounts WHERE name = :name", soci::into(abi_def_account, ind), soci::use(action.account.to_string());

    if (!abi_def_account.empty()) {
        abi = fc::json::from_string(abi_def_account).as<chain::abi_def>();
    } else if (action.account == chain::config::system_account_name) {
        abi = chain::eosio_contract_abi(abi);
    } else {
        return; // no ABI no party. Should we still store it?
    }

    abis.set_abi(abi);

    auto abi_data = abis.binary_to_variant(abis.get_action_type(action.name), action.data);
    string json = fc::json::to_string(abi_data);

    boost::uuids::random_generator gen;
    boost::uuids::uuid id = gen();
    std::string action_id = boost::uuids::to_string(id);

    *m_session << "INSERT INTO actions(id, account, name, data, transaction_id) VALUES (:id, :ac, :na, :da, :ti) ",
            soci::use(action_id),
            soci::use(action.account.to_string()),
            soci::use(action.name.to_string()),
            soci::use(json),
            soci::use(transaction_id_str);

    for (const auto& auth : action.authorization) {
        *m_session << "INSERT INTO actions_accounts(action_id, actor, permission) VALUES (:id, :ac, :pe) ",
                soci::use(action_id),
                soci::use(auth.actor.to_string()),
                soci::use(auth.permission.to_string());
    }

    // TODO: move all
    if (action.name == N(issue)) {

        auto to_name = abi_data["to"].as<chain::name>().to_string();
        auto asset_quantity = abi_data["quantity"].as<chain::asset>();
        int exist;

        *m_session << "SELECT COUNT(*) FROM tokens WHERE account = :ac AND symbol = :sy",
                soci::into(exist),
                soci::use(to_name),
                soci::use(asset_quantity.get_symbol().name());
        if (exist > 0) {
            *m_session << "UPDATE tokens SET amount = amount + :am WHERE account = :ac AND symbol :sy",
                    soci::use(asset_quantity.to_real()),
                    soci::use(to_name),
                    soci::use(asset_quantity.get_symbol().name());
        } else {
            *m_session << "INSERT INTO tokens(account, amount, staked, symbol) VALUES (:ac, :am, 0, :as) ",
                    soci::use(to_name),
                    soci::use(asset_quantity.to_real()),
                    soci::use(asset_quantity.get_symbol().name());
        }
    }

    if (action.name == N(transfer)) {

        auto from_name = abi_data["from"].as<chain::name>().to_string();
        auto to_name = abi_data["to"].as<chain::name>().to_string();
        auto asset_quantity = abi_data["quantity"].as<chain::asset>();
        int exist;

        *m_session << "SELECT COUNT(*) FROM tokens WHERE account = :ac AND symbol = :sy",
                soci::into(exist),
                soci::use(to_name),
                soci::use(asset_quantity.get_symbol().name());
        if (exist > 0) {
            *m_session << "UPDATE tokens SET amount = amount + :am WHERE account = :ac AND symbol = :sy",
                    soci::use(asset_quantity.to_real()),
                    soci::use(to_name),
                    soci::use(asset_quantity.get_symbol().name());
        } else {
            *m_session << "INSERT INTO tokens(account, amount, staked, symbol) VALUES (:ac, :am, 0, :as) ",
                    soci::use(to_name),
                    soci::use(asset_quantity.to_real()),
                    soci::use(asset_quantity.get_symbol().name());
        }

        *m_session << "UPDATE tokens SET amount = amount - :am WHERE account = :ac AND symbol = :sy",
                soci::use(asset_quantity.to_real()),
                soci::use(from_name),
                soci::use(asset_quantity.get_symbol().name());
    }

    if (action.account != chain::name(chain::config::system_account_name)) {
        return;
    }

    if (action.name == chain::setabi::get_name()) {
        chain::abi_def abi_setabi;
        chain::setabi action_data = action.data_as<chain::setabi>();
        chain::abi_serializer::to_abi(action_data.abi, abi_setabi);
        string abi_string = fc::json::to_string(abi_setabi);

        *m_session << "UPDATE accounts SET abi = :abi, updated_at = NOW() WHERE name = :name",
                soci::use(abi_string),
                soci::use(action_data.account.to_string());

    } else if (action.name == chain::newaccount::get_name()) {
        auto action_data = action.data_as<chain::newaccount>();
        *m_session << "INSERT INTO accounts (name) VALUES (:name)",
                soci::use(action_data.name.to_string());

        for (const auto& key_owner : action_data.owner.keys) {
            string permission_owner = "owner";
            string public_key_owner = static_cast<string>(key_owner.key);
            *m_session << "INSERT INTO accounts_keys(account, public_key, permission) VALUES (:ac, :ke, :pe) ",
                    soci::use(action_data.name.to_string()),
                    soci::use(public_key_owner),
                    soci::use(permission_owner);
        }

        for (const auto& key_active : action_data.active.keys) {
            string permission_active = "active";
            string public_key_active = static_cast<string>(key_active.key);
            *m_session << "INSERT INTO accounts_keys(account, public_key, permission) VALUES (:ac, :ke, :pe) ",
                    soci::use(action_data.name.to_string()),
                    soci::use(public_key_active),
                    soci::use(permission_active);
        }

    }

    // TODO: catch issue (tokens) // public keys update // stake / voting
}

} // namespace
