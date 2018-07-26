#include "actions_table.h"

namespace eosio {

actions_table::actions_table(std::shared_ptr<soci::session> session):
    m_session(session)
{

}

void actions_table::drop()
{
    try {
        *m_session << "drop table IF EXISTS actions_accounts";
        *m_session << "drop table IF EXISTS stakes";
        *m_session << "drop table IF EXISTS votes";
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
            "id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,"
            "account VARCHAR(12),"
            "transaction_id VARCHAR(64),"
            "seq SMALLINT,"
            "parent INT DEFAULT NULL,"
            "name VARCHAR(12),"
            "created_at DATETIME DEFAULT NOW(),"
            "data JSON, FOREIGN KEY (transaction_id) REFERENCES transactions(id) ON DELETE CASCADE,"
            "FOREIGN KEY (account) REFERENCES accounts(name)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;";

    *m_session << "CREATE INDEX idx_actions_account ON actions (account);";
    *m_session << "CREATE INDEX idx_actions_tx_id ON actions (transaction_id);";
    *m_session << "CREATE INDEX idx_actions_created ON actions (created_at);";

    *m_session << "CREATE TABLE actions_accounts("
            "actor VARCHAR(12),"
            "permission VARCHAR(12),"
            "action_id INT NOT NULL, FOREIGN KEY (action_id) REFERENCES actions(id) ON DELETE CASCADE,"
            "FOREIGN KEY (actor) REFERENCES accounts(name)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;";

    *m_session << "CREATE INDEX idx_actions_actor ON actions_accounts (actor);";
    *m_session << "CREATE INDEX idx_actions_action_id ON actions_accounts (action_id);";

    *m_session << "CREATE TABLE tokens("
            "account VARCHAR(13),"
            "symbol VARCHAR(10),"
            "amount DOUBLE(64,4),"
            "FOREIGN KEY (account) REFERENCES accounts(name)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;"; // TODO: other tokens could have diff format.

    *m_session << "CREATE INDEX idx_tokens_account ON tokens (account);";

    *m_session << "CREATE TABLE votes("
            "account VARCHAR(13) PRIMARY KEY,"
            "votes JSON"
            ", FOREIGN KEY (account) REFERENCES accounts(name), UNIQUE KEY account (account)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;";

    *m_session << "CREATE TABLE stakes("
            "account VARCHAR(13) PRIMARY KEY,"
            "cpu REAL(14,4),"
            "net REAL(14,4),"
            "FOREIGN KEY (account) REFERENCES accounts(name)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;";
}

void actions_table::add(chain::action action, chain::transaction_id_type transaction_id, fc::time_point_sec transaction_time, uint8_t seq)
{

    chain::abi_def abi;
    std::string abi_def_account;
    chain::abi_serializer abis;
    soci::indicator ind;
    const auto transaction_id_str = transaction_id.str();
    const auto expiration = boost::chrono::seconds{transaction_time.sec_since_epoch()}.count();

    *m_session << "SELECT abi FROM accounts WHERE name = :name", soci::into(abi_def_account, ind), soci::use(action.account.to_string());

    if (!abi_def_account.empty()) {
        abi = fc::json::from_string(abi_def_account).as<chain::abi_def>();
    } else if (action.account == chain::config::system_account_name) {
        abi = chain::eosio_contract_abi(abi);
    } else {
        return; // no ABI no party. Should we still store it?
    }

    static const fc::microseconds abi_serializer_max_time(1000000); // 1 second
    abis.set_abi(abi, abi_serializer_max_time);

    auto abi_data = abis.binary_to_variant(abis.get_action_type(action.name), action.data, abi_serializer_max_time);
    string json = fc::json::to_string(abi_data);

    boost::uuids::random_generator gen;
    boost::uuids::uuid id = gen();
    std::string action_id = boost::uuids::to_string(id);

    *m_session << "INSERT INTO actions(account, seq, created_at, name, data, transaction_id) VALUES (:ac, :se, FROM_UNIXTIME(:ca), :na, :da, :ti) ",
            soci::use(action.account.to_string()),
            soci::use(seq),
            soci::use(expiration),
            soci::use(action.name.to_string()),
            soci::use(json),
            soci::use(transaction_id_str);

    for (const auto& auth : action.authorization) {
        *m_session << "INSERT INTO actions_accounts(action_id, actor, permission) VALUES (LAST_INSERT_ID(), :ac, :pe) ",
                soci::use(auth.actor.to_string()),
                soci::use(auth.permission.to_string());
    }
    try {
        parse_actions(action, abi_data);
    } catch(std::exception& e){
        wlog(e.what());
    }
}

void actions_table::parse_actions(chain::action action, fc::variant abi_data)
{
    // TODO: move all  + catch // public keys update // stake / voting
    if (action.name == N(issue)) {

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
            *m_session << "INSERT INTO tokens(account, amount, symbol) VALUES (:ac, :am, :as) ",
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
            *m_session << "INSERT INTO tokens(account, amount, symbol) VALUES (:ac, :am, :as) ",
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

    if (action.name == N(voteproducer)) {
        auto voter = abi_data["voter"].as<chain::name>().to_string();
        string votes = fc::json::to_string(abi_data["producers"]);

        *m_session << "REPLACE INTO votes(account, votes) VALUES (:ac, :vo) ",
                soci::use(voter),
                soci::use(votes);
    }


    if (action.name == N(delegatebw)) {
        auto account = abi_data["receiver"].as<chain::name>().to_string();
        auto cpu = abi_data["stake_cpu_quantity"].as<chain::asset>();
        auto net = abi_data["stake_net_quantity"].as<chain::asset>();

        *m_session << "REPLACE INTO stakes(account, cpu, net) VALUES (:ac, :cp, :ne) ",
                soci::use(account),
                soci::use(cpu.to_real()),
                soci::use(net.to_real());
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
}

} // namespace
