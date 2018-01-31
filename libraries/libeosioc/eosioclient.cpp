#include <eosio/libeosioc/eosioclient.hpp>

#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>

#include "fc/io/json.hpp"
#include "localize.hpp"

FC_DECLARE_EXCEPTION( explained_exception, 9000000, "explained exception, see error log" );

namespace eosio {
namespace client {

const auto tx_expiration = fc::seconds(30);

eosioclient::eosioclient(peer& peer, wallet& wallet):
    m_wallet(wallet),
    m_peer(peer)
{

}

void eosioclient::create_account(chain::name creator,
                                 chain::name newaccount,
                                 chain::public_key_type owner,
                                 chain::public_key_type active,
                                 bool sign,
                                 uint64_t staked_deposit)
{
    auto owner_auth   = eosio::chain::authority{1, {{owner, 1}}, {}};
    auto active_auth  = eosio::chain::authority{1, {{active, 1}}, {}};
    auto recovery_auth = eosio::chain::authority{1, {}, {{{creator, "active"}, 1}}};

    uint64_t deposit = staked_deposit;

    chain::signed_transaction trx;
    trx.actions.emplace_back( vector<chain::permission_level>{{creator,"active"}},
                              chain::contracts::newaccount{creator, newaccount, owner_auth, active_auth, recovery_auth, deposit});

    std::cout << fc::json::to_pretty_string(push_transaction(trx, sign)) << std::endl;
}

//void Eosioclient::create_producer(const std::string &account_name, const std::string &owner_key, std::vector<std::string> permissions, bool skip_sign)
//{
//    if (permissions.empty()) {
//       permissions.push_back(account_name + "@active");
//    }
//    auto account_permissions = get_account_permissions(permissions);

//    chain::signed_transaction trx;
//    trx.actions.emplace_back(account_permissions,
//                             chain::contracts::setproducer{account_name,
//                                                           chain::public_key_type(owner_key),
//                                                           chain::chain_config{}} );

//    std::cout << fc::json::to_pretty_string(push_transaction(trx, !skip_sign)) << std::endl;
//}

void eosioclient::send_transaction(const std::vector<chain::action>& actions, bool skip_sign) {
    eosio::chain::signed_transaction trx;
    for (const auto& m: actions) {
        trx.actions.emplace_back( m );
    }

    auto info = get_info();
    trx.expiration = info.head_block_time + tx_expiration;
    trx.set_reference_block(info.head_block_id);
    if (!skip_sign) {
        sign_transaction(trx);
    }

    std::cout << fc::json::to_pretty_string(m_peer.push_transaction( trx )) << std::endl;
}

fc::variant eosioclient::push_transaction(eosio::chain::signed_transaction& trx, bool sign )
{
    auto info = get_info();
    trx.expiration = info.head_block_time + tx_expiration;
    trx.set_reference_block(info.head_block_id);

    if (sign) {
        sign_transaction(trx);
    }

    return m_peer.push_transaction( trx );
}

std::vector<uint8_t> eosioclient::assemble_wast( const std::string& wast ) {
   IR::Module module;
   std::vector<WAST::Error> parseErrors;
   WAST::parseModule(wast.c_str(),wast.size(),module,parseErrors);
   if(parseErrors.size())
   {
      // Print any parse errors;
      std::cerr << localized("Error parsing WebAssembly text file:") << std::endl;
      for(auto& error : parseErrors)
      {
         std::cerr << ":" << error.locus.describe() << ": " << error.message.c_str() << std::endl;
         std::cerr << error.locus.sourceLine << std::endl;
         std::cerr << std::setw(error.locus.column(8)) << "^" << std::endl;
      }
      FC_THROW_EXCEPTION( explained_exception, "wast parse error" );
   }

   try
   {
      // Serialize the WebAssembly module.
      Serialization::ArrayOutputStream stream;
      WASM::serialize(stream,module);
      return stream.getBytes();
   }
   catch(Serialization::FatalSerializationException exception)
   {
      std::cerr << localized("Error serializing WebAssembly binary file:") << std::endl;
      std::cerr << exception.message << std::endl;
      FC_THROW_EXCEPTION( explained_exception, "wasm serialize error");
   }
}

void eosioclient::sign_transaction(eosio::chain::signed_transaction& trx)
{
    // TODO better error checking
    const auto& public_keys = m_wallet.public_keys();
    const auto& required_keys = m_peer.get_keys_required(trx, public_keys);
    // TODO determine chain id
    const auto& signed_trx = m_wallet.sign_transaction(trx, required_keys, eosio::chain::chain_id_type{});
    trx = signed_trx.as<eosio::chain::signed_transaction>();
}

eosio::chain_apis::read_only::get_info_results eosioclient::get_info() {
    return m_peer.get_info().as<eosio::chain_apis::read_only::get_info_results>();
}

//std::vector<chain::permission_level> Eosioclient::get_account_permissions(const std::vector<std::string>& permissions)
//{
//   auto fixedPermissions = permissions | boost::adaptors::transformed([](const std::string& p) {
//      std::vector<std::string> pieces;
//      split(pieces, p, boost::algorithm::is_any_of("@"));
//      EOSC_ASSERT(pieces.size() == 2, "Invalid permission: ${p}", ("p", p));
//      return chain::permission_level{ .actor = pieces[0], .permission = pieces[1] };
//   });
//   std::vector<chain::permission_level> accountPermissions;
//   boost::range::copy(fixedPermissions, back_inserter(accountPermissions));
//   return accountPermissions;
//}

void eosioclient::set_account_permission(const std::string& accountStr,
                                         const std::string& permissionStr,
                                         const std::string& authorityJsonOrFile,
                                         const std::string& parentStr,
                                         const std::string& permissionAuth,
                                         bool skip_sign)
{
    chain::name account = chain::name(accountStr);
    chain::name permission = chain::name(permissionStr);
    bool is_delete = boost::iequals(authorityJsonOrFile, "null");

    if (is_delete) {
        send_transaction({create_deleteauth(account, permission, chain::name(permissionAuth))}, skip_sign);
    } else {
        authority auth;
        if (boost::istarts_with(authorityJsonOrFile, "EOS")) {
            auth = authority(public_key_type(authorityJsonOrFile));
        } else {
            fc::variant parsedAuthority;
            if (boost::istarts_with(authorityJsonOrFile, "{")) {
                parsedAuthority = fc::json::from_string(authorityJsonOrFile);
            } else {
                parsedAuthority = fc::json::from_file(authorityJsonOrFile);
            }

            auth = parsedAuthority.as<authority>();
        }

        chain::name parent;
        if (parentStr.size() == 0 && permissionStr != "owner") {
            // see if we can auto-determine the proper parent
            const auto account_result = m_peer.get_account(accountStr);
            const auto& existing_permissions = account_result.get_object()["permissions"].get_array();
            auto permissionPredicate = [&](const auto& perm) {
                return perm.is_object() &&
                        perm.get_object().contains("permission") &&
                        boost::equals(perm.get_object()["permission"].get_string(), permissionStr);
            };

            auto itr = boost::find_if(existing_permissions, permissionPredicate);
            if (itr != existing_permissions.end()) {
                parent = chain::name((*itr).get_object()["parent"].get_string());
            } else {
                // if this is a new permission and there is no parent we default to "active"
                parent = chain::name("active");

            }
        } else {
            parent = chain::name(parentStr);
        }

        send_transaction({create_updateauth(account, permission, parent, auth, chain::name(permissionAuth))}, skip_sign);
    }
}

void eosioclient::set_action_permission(const std::string& accountStr,
                                        const std::string& codeStr,
                                        const std::string& typeStr,
                                        const std::string& requirementStr,
                                        bool skip_sign)
{
    chain::name account = chain::name(accountStr);
    chain::name code = chain::name(codeStr);
    chain::name type = chain::name(typeStr);
    bool is_delete = boost::iequals(requirementStr, "null");

    if (is_delete) {
        send_transaction({create_unlinkauth(account, code, type)}, skip_sign);
    } else {
        chain::name requirement = chain::name(requirementStr);
        send_transaction({create_linkauth(account, code, type, requirement)}, skip_sign);
    }
}

void eosioclient::transfer(const std::string& sender,
                           const std::string& recipient,
                           uint64_t amount,
                           std::string memo,
                           bool skip_sign,
                           bool tx_force_unique)
{
    chain::signed_transaction trx;

    if (tx_force_unique) {
        if (memo.size() == 0) {
            // use the memo to add a nonce
            memo = fc::to_string(generate_nonce_value());
        } else {
            // add a nonce actions
            trx.actions.emplace_back( generate_nonce() );
        }
    }

    trx.actions.emplace_back( std::vector<chain::permission_level>{{sender,"active"}},
                              chain::contracts::transfer{ .from = sender, .to = recipient, .amount = amount, .memo = memo});


    auto info = get_info();
    trx.expiration = info.head_block_time + tx_expiration;
    trx.set_reference_block( info.head_block_id);
    if (!skip_sign) {
        sign_transaction(trx);
    }

    std::cout << fc::json::to_pretty_string( m_peer.push_transaction( trx )) << std::endl;
}


chain::action eosioclient::create_deleteauth(const chain::name& account,
                                             const chain::name& permission,
                                             const chain::name& permissionAuth)
{
    return chain::action { std::vector<chain::permission_level>{{account,permissionAuth}},
        chain::contracts::deleteauth{account, permission}};
}

chain::action eosioclient::create_updateauth(const chain::name& account,
                                             const chain::name& permission,
                                             const chain::name& parent,
                                             const chain::authority& auth,
                                             const chain::name& permissionAuth)
{
    return chain::action { std::vector<chain::permission_level>{{account,permissionAuth}},
        chain::contracts::updateauth{account, permission, parent, auth}};
}

chain::action eosioclient::create_linkauth(const chain::name& account,
                                           const chain::name& code,
                                           const chain::name& type,
                                           const chain::name& requirement)
{
    return chain::action { std::vector<chain::permission_level>{{account,"active"}},
        chain::contracts::linkauth{account, code, type, requirement}};
}

chain::action eosioclient::create_unlinkauth(const chain::name& account,
                                             const chain::name& code,
                                             const chain::name& type)
{
    return chain::action { std::vector<chain::permission_level>{{account,"active"}},
        chain::contracts::unlinkauth{account, code, type}};
}

uint64_t eosioclient::generate_nonce_value() {
   return fc::time_point::now().time_since_epoch().count();
}

chain::action eosioclient::generate_nonce() {
   return chain::action( {}, chain::contracts::nonce{.value = generate_nonce_value()} );
}

} // namespace client
} // namespace eosio
