#include "actions_table.h"
#include "zdbcpp.h"

#include <eosio/chain/eosio_contract.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/types.hpp>

#include <fc/io/json.hpp>
#include <fc/utf8.hpp>
#include <fc/variant.hpp>

#include <iostream>

using namespace zdbcpp;
namespace eosio {

actions_table::actions_table(std::shared_ptr<connection_pool> pool):
    m_pool(pool)
{

}

void actions_table::drop()
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    ilog("actions_table : m_pool->get_connection succeeded");

    try {
        con.execute("drop table IF EXISTS actions_accounts;");
        ilog("drop table IF EXISTS actions_accounts;");
        con.execute("drop table IF EXISTS stakes;");
        ilog("drop table IF EXISTS stakes;");
        con.execute("drop table IF EXISTS votes;");
        ilog("drop table IF EXISTS votes;");
        con.execute("drop table IF EXISTS tokens;");
        ilog("drop table IF EXISTS tokens;");
        con.execute("drop table IF EXISTS actions;");
        ilog("drop table IF EXISTS actions;");
    }
    catch(std::exception& e){
        wlog(e.what());
    }

    m_pool->release_connection(con);
    ilog("actions_table : m_pool->release_connection succeeded");
}

void actions_table::create()
{
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);
    try {
        con.execute("CREATE TABLE actions("
                "id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,"
                "account VARCHAR(12),"
                "transaction_id VARCHAR(64),"
                "seq SMALLINT,"
                "parent INT DEFAULT NULL,"
                "name VARCHAR(12),"
                "created_at DATETIME DEFAULT NOW(),"
                "hex_data LONGTEXT, "
                "data JSON "
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;");

        con.execute("CREATE INDEX idx_actions_account ON actions (account);");
        con.execute("CREATE INDEX idx_actions_tx_id ON actions (transaction_id);");
        con.execute("CREATE INDEX idx_actions_created ON actions (created_at);");

        con.execute("CREATE TABLE actions_accounts("
                "actor VARCHAR(12),"
                "permission VARCHAR(12),"
                "action_id INT NOT NULL "
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;");

        con.execute("CREATE INDEX idx_actions_actor ON actions_accounts (actor);");
        con.execute("CREATE INDEX idx_actions_action_id ON actions_accounts (action_id);");

        con.execute("CREATE TABLE tokens("
                "account VARCHAR(13),"
                "symbol VARCHAR(10),"
                "amount DOUBLE(64,4)"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;"); // TODO: other tokens could have diff format.

        con.execute("CREATE INDEX idx_tokens_account ON tokens (account);");

        con.execute("CREATE TABLE votes("
                "account VARCHAR(13) PRIMARY KEY,"
                "votes JSON"
                ", UNIQUE KEY account (account)) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;");

        con.execute("CREATE TABLE stakes("
                "account VARCHAR(13) PRIMARY KEY,"
                "cpu REAL(14,4),"
                "net REAL(14,4) "
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE utf8mb4_general_ci;");
    }
    catch(std::exception& e){
        wlog(e.what());
    }

    m_pool->release_connection(con);
}

void actions_table::add(chain::action action, chain::transaction_id_type transaction_id, fc::time_point_sec transaction_time, int32_t seq)
{
    chain::abi_def abi;
    std::string abi_def_account;
    chain::abi_serializer abis;
    
    const auto transaction_id_str = transaction_id.str();
    const auto expiration = boost::chrono::seconds{transaction_time.sec_since_epoch()}.count();
    string m_account_name = action.account.to_string();
    int max_field_size = 6500000;

    try {
        if(action.account == chain::config::system_account_name) {
            if(action.name == chain::setabi::get_name()) {
                auto setabi = action.data_as<chain::setabi>();
                try {
                    const chain::abi_def& abi_def = fc::raw::unpack<chain::abi_def>( setabi.abi );
                    const string json_str = fc::json::to_string( abi_def );

                    zdbcpp::Connection con = m_pool->get_connection();
                    assert(con);

                    zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO actions(account, seq, created_at, name, data, transaction_id) VALUES (?, ?, FROM_UNIXTIME(?), ?, ?, ?) ");
            
                    pre.setString(1,m_account_name.c_str());
                    pre.setInt(2,seq);
                    pre.setDouble(3,expiration);
                    pre.setString(4,action.name.to_string().c_str());
                    pre.setString(5,json_str.c_str());
                    pre.setString(6,transaction_id_str.c_str());

                    pre.execute();
                    
                    for (const auto& auth : action.authorization) {
                        string m_actor_name = auth.actor.to_string();
                        zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO actions_accounts(action_id, actor, permission) VALUES (LAST_INSERT_ID(), ?, ?) ");
                        pre.setString(1,m_actor_name.c_str());
                        pre.setString(2,auth.permission.to_string().c_str());
                        pre.execute();
                    }
                    return;
                } catch( fc::exception& e ) {
                    ilog( "Unable to convert action abi_def to json for ${n}", ("n", setabi.account.to_string()));
                }
            }
        }

        zdbcpp::Connection con = m_pool->get_connection();
        assert(con);
    
        zdbcpp::ResultSet rset = con.executeQuery("SELECT REPLACE(abi,'\"','\\\"') as `abi` FROM accounts WHERE `name` = ?; ", m_account_name.c_str());
        assert(rset);

        if(rset.next()){
            const char* cp = rset.getStringByName("abi");
            if(cp) {
                abi_def_account = std::string(cp);
                abi_def_account.append("\0");
            }
            
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
            
            // boost::uuids::random_generator gen;
            // boost::uuids::uuid id = gen();
            // std::string action_id = boost::uuids::to_string(id);

            zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO actions(account, seq, created_at, name, data, transaction_id) VALUES (?, ?, FROM_UNIXTIME(?), ?, ?, ?) ");
            
            pre.setString(1,m_account_name.c_str());
            pre.setInt(2,seq);
            pre.setDouble(3,expiration);
            pre.setString(4,action.name.to_string().c_str());
            pre.setString(5,json.c_str());
            pre.setString(6,transaction_id_str.c_str());

            pre.execute();
            
            for (const auto& auth : action.authorization) {
                string m_actor_name = auth.actor.to_string();
                zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO actions_accounts(action_id, actor, permission) VALUES (LAST_INSERT_ID(), ?, ?) ");
                pre.setString(1,m_actor_name.c_str());
                pre.setString(2,auth.permission.to_string().c_str());
                pre.execute();
            }

            parse_actions(action, abi_data);

            return;
        }       
    } catch( std::exception& e ) {
        ilog( "Unable to convert action.data to ABI: ${s}::${n}, std what: ${e}",
            ("s", action.account)( "n", action.name )( "e", e.what()));
    } catch (fc::exception& e) {
        if (action.name != "onblock") { // eosio::onblock not in original eosio.system abi
            ilog( "Unable to convert action.data to ABI: ${s}::${n}, fc exception: ${e}",
                ("s", action.account)( "n", action.name )( "e", e.to_detail_string()));
        }
    } catch( ... ) {
        ilog( "Unable to convert action.data to ABI: ${s}::${n}, unknown exception",
            ("s", action.account)( "n", action.name ));
    }

    string m_hex_data = fc::variant( action.data ).as_string();
    
    zdbcpp::Connection con = m_pool->get_connection();
    assert(con);

    zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO actions(account, seq, created_at, name, hex_data, transaction_id) VALUES (?, ?, FROM_UNIXTIME(?), ?, ?, ?) ");

    pre.setString(1,m_account_name.c_str());
    pre.setInt(2,seq);
    pre.setDouble(3,expiration);
    pre.setString(4,action.name.to_string().c_str());
    pre.setString(5,m_hex_data.c_str());
    pre.setString(6,transaction_id_str.c_str());

    pre.execute();
    for (const auto& auth : action.authorization) {
        string m_actor_name = auth.actor.to_string();
        zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO actions_accounts(action_id, actor, permission) VALUES (LAST_INSERT_ID(), ?, ?) ");
        pre.setString(1,m_actor_name.c_str());
        pre.setString(2,auth.permission.to_string().c_str());
        pre.execute();
    }

    m_pool->release_connection(con);
}

void actions_table::parse_actions(chain::action action, fc::variant abi_data)
{
    try {
        // TODO: move all  + catch // public keys update // stake / voting
        if (action.name == N(issue)) {
            auto to_name = abi_data["to"].as<chain::name>().to_string();
            auto asset_quantity = abi_data["quantity"].as<chain::asset>();
            int exist;

            zdbcpp::Connection con = m_pool->get_connection();
            assert(con);

            zdbcpp::ResultSet rset = con.executeQuery("SELECT COUNT(*) as cnt FROM tokens WHERE account = ? AND symbol = ?",to_name,asset_quantity.get_symbol().name());
            assert(rset);
            rset.next();
            exist = rset.getIntByName("cnt");
                    
            if (exist > 0) {
                zdbcpp::PreparedStatement pre = con.prepareStatement("UPDATE tokens SET amount = amount + ? WHERE account = ? AND symbol = ?");
                pre.setDouble(1,asset_quantity.to_real());
                pre.setString(2,to_name.c_str());
                pre.setString(3,asset_quantity.get_symbol().name().c_str());
                
                pre.execute();
            } else {
                zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO tokens(account, amount, symbol) VALUES (?, ?, ?)");
                pre.setString(1,to_name.c_str());
                pre.setDouble(2,asset_quantity.to_real());
                pre.setString(3,asset_quantity.get_symbol().name().c_str());
                
                pre.execute();
            }
        }

        if (action.name == N(transfer)) {
            auto from_name = abi_data["from"].as<chain::name>().to_string();
            auto to_name = abi_data["to"].as<chain::name>().to_string();
            auto asset_quantity = abi_data["quantity"].as<chain::asset>();
            int exist;

            zdbcpp::Connection con = m_pool->get_connection();
            assert(con);

            zdbcpp::ResultSet rset = con.executeQuery("SELECT COUNT(*) as cnt FROM tokens WHERE account = ? AND symbol = ?",to_name,asset_quantity.get_symbol().name());
            assert(rset);
            rset.next();
            exist = rset.getIntByName("cnt");

            if (exist > 0) {
                zdbcpp::PreparedStatement pre = con.prepareStatement("UPDATE tokens SET amount = amount + ? WHERE account = ? AND symbol = ?");
                pre.setDouble(1,asset_quantity.to_real());
                pre.setString(2,to_name.c_str());
                pre.setString(3,asset_quantity.get_symbol().name().c_str());

                pre.execute();
            } else {
                zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO tokens(account, amount, symbol) VALUES (?, ?, ?)");
                pre.setString(1,to_name.c_str());
                pre.setDouble(2,asset_quantity.to_real());
                pre.setString(3,asset_quantity.get_symbol().name().c_str());
                
                pre.execute();
            }

            zdbcpp::PreparedStatement pre = con.prepareStatement("UPDATE tokens SET amount = amount + ? WHERE account = ? AND symbol = ?");
            pre.setDouble(1,asset_quantity.to_real());
            pre.setString(2,to_name.c_str());
            pre.setString(3,asset_quantity.get_symbol().name().c_str());
            pre.execute();
        }

        if (action.account != chain::name(chain::config::system_account_name)) {
            return;
        }

        if (action.name == N(voteproducer)) {
            auto voter = abi_data["voter"].as<chain::name>().to_string();
            string votes = fc::json::to_string(abi_data["producers"]);

            zdbcpp::Connection con = m_pool->get_connection();
            assert(con);

            zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO votes(account, votes) VALUES (?, ?) ON DUPLICATE KEY UPDATE votes = ?; ");
            pre.setString(1,voter.c_str());
            pre.setString(2,votes.c_str());
            pre.setString(3,votes.c_str());

            pre.execute();
        }


        if (action.name == N(delegatebw)) {
            auto account = abi_data["receiver"].as<chain::name>().to_string();
            auto cpu = abi_data["stake_cpu_quantity"].as<chain::asset>();
            auto net = abi_data["stake_net_quantity"].as<chain::asset>();

            zdbcpp::Connection con = m_pool->get_connection();
            assert(con);

            zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO stakes(account, cpu, net) VALUES (?, ?, ?) ON DUPLICATE KEY UPDATE cpu = ?, net = ? ;");
            pre.setString(1,account.c_str());
            pre.setDouble(2,cpu.to_real());
            pre.setDouble(3,net.to_real());
            pre.setDouble(4,cpu.to_real());
            pre.setDouble(5,net.to_real());
            
            pre.execute();
        }

        // if (action.name == chain::setabi::get_name()) {
        //     zdbcpp::Connection con = m_pool->get_connection();
        //     assert(con);

        //     chain::abi_def abi_setabi;
        //     chain::setabi action_data = action.data_as<chain::setabi>();
        //     chain::abi_serializer::to_abi(action_data.abi, abi_setabi);
        //     string abi_string = fc::json::to_string(abi_setabi);

        //     zdbcpp::PreparedStatement pre = con.prepareStatement("UPDATE accounts SET abi = ?, updated_at = NOW() WHERE name = ?");
        //     pre.setString(1,abi_string.c_str());
        //     pre.setString(2,action_data.account.to_string().c_str());
                    
        //     pre.execute();
        // } else if (action.name == chain::newaccount::get_name()) {
        //     zdbcpp::Connection con = m_pool->get_connection();
        //     assert(con);

        //     auto action_data = action.data_as<chain::newaccount>();
        //     zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO accounts (name) VALUES (?)");
        //     pre.setString(1,action_data.name.to_string().c_str());

        //     pre.execute();

        //     for (const auto& key_owner : action_data.owner.keys) {
        //         string permission_owner = "owner";
        //         string public_key_owner = static_cast<string>(key_owner.key);
        //         zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO accounts_keys(account, public_key, permission) VALUES (?,?,?) ");
        //         pre.setString(1,action_data.name.to_string().c_str()),
        //         pre.setString(2,public_key_owner.c_str());
        //         pre.setString(3,permission_owner.c_str());

        //         pre.execute();
        //     }

        //     for (const auto& key_active : action_data.active.keys) {
        //         string permission_active = "active";
        //         string public_key_active = static_cast<string>(key_active.key);

        //         zdbcpp::PreparedStatement pre = con.prepareStatement("INSERT INTO accounts_keys(account, public_key, permission) VALUES (?,?,?) ");
        //         pre.setString(1,action_data.name.to_string().c_str());
        //         pre.setString(2,public_key_active.c_str());
        //         pre.setString(3,permission_active.c_str());

        //         pre.execute();
        //     }

        // }
        
    } catch(std::exception& e){
        wlog(e.what());
    }

    // m_pool->release_connection(con);
}

} // namespace
