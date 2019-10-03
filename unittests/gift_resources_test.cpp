/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/testing/tester.hpp>

#include "eosio_system_tester.hpp"

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include <boost/test/unit_test.hpp>

#include <contracts.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

namespace
{
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

struct genesis_account
{
   account_name name;
   uint64_t initial_balance;
};

struct create_attribute_t
{
   name attr_name;
   int32_t type;
   int32_t privacy_type;
};

class gift_resources_tester : public TESTER
{
public:
   gift_resources_tester();

   void deploy_contract(bool call_init = true)
   {
      set_code(config::system_account_name, contracts::rem_system_wasm());
      set_abi(config::system_account_name, contracts::rem_system_abi().data());
      if (call_init)
      {
         base_tester::push_action(config::system_account_name, N(init),
                                  config::system_account_name, mutable_variant_object()("version", 0)("core", CORE_SYM_STR));
      }

      const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      rem_sys_abi_ser.set_abi(abi, abi_serializer_max_time);
   }

   auto delegate_bandwidth(name from, name receiver, asset stake_quantity, uint8_t transfer = 1)
   {
      auto r = base_tester::push_action(config::system_account_name, N(delegatebw), from, mvo()("from", from)("receiver", receiver)("stake_quantity", stake_quantity)("transfer", transfer));
      produce_block();
      return r;
   }

   void create_currency(name contract, name manager, asset maxsupply, const private_key_type *signer = nullptr)
   {
      auto act = mutable_variant_object()("issuer", manager)("maximum_supply", maxsupply);

      base_tester::push_action(contract, N(create), contract, act);
   }

   auto issue(name contract, name manager, name to, asset amount)
   {
      auto r = base_tester::push_action(contract, N(issue), manager, mutable_variant_object()("to", to)("quantity", amount)("memo", ""));
      produce_block();
      return r;
   }

   auto set_privileged(name account)
   {
      auto r = base_tester::push_action(config::system_account_name, N(setpriv), config::system_account_name, mvo()("account", account)("is_priv", 1));
      produce_block();
      return r;
   }

   fc::variant get_global_state()
   {
      vector<char> data = get_row_by_account(config::system_account_name, config::system_account_name, N(global), N(global));
      if (data.empty())
      {
         std::cout << "\nData is empty\n"
                   << std::endl;
      }
      return data.empty() ? fc::variant() : rem_sys_abi_ser.binary_to_variant("eosio_global_state", data, abi_serializer_max_time);
   }

   fc::variant get_total_stake(const account_name &act)
   {
      vector<char> data = get_row_by_account(config::system_account_name, act, N(userres), act);
      return data.empty() ? fc::variant() : rem_sys_abi_ser.binary_to_variant("user_resources", data, abi_serializer_max_time);
   }

   transaction_trace_ptr create_account_with_resources(account_name new_acc, account_name creator, asset stake, bool transfer = true)
   {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth = authority(get_public_key(new_acc, "owner"));

      trx.actions.emplace_back(vector<permission_level>{{creator, config::active_name}},
                               newaccount{
                                   .creator = creator,
                                   .name = new_acc,
                                   .owner = owner_auth,
                                   .active = authority(get_public_key(new_acc, "active"))});

      trx.actions.emplace_back(
         get_action(config::system_account_name, N(delegatebw), vector<permission_level>{{creator, config::active_name}},
                     mvo()("from", creator)("receiver", new_acc)("stake_quantity", stake)("transfer", transfer)
         )
      );

      set_transaction_headers(trx);
      trx.sign(get_private_key(creator, "active"), control->get_chain_id());
      return push_transaction(trx);
   }

   auto create_attr(name attr, int32_t type, int32_t ptype)
   {
      auto r = base_tester::push_action(N(rem.attr), N(create), N(rem.attr), mvo()("attribute_name", attr)("type", type)("ptype", ptype));
      produce_block();
      return r;
   }

   auto set_attr(name issuer, name receiver, name attribute_name, std::string value)
   {
      auto r = base_tester::push_action(N(rem.attr), N(setattr), issuer, mvo()("issuer", issuer)("receiver", receiver)("attribute_name", attribute_name)("value", value));
      produce_block();
      return r;
   }

   auto unset_attr(name issuer, name receiver, name attribute_name)
   {
      auto r = base_tester::push_action(N(rem.attr), N(unsetattr), issuer, mvo()("issuer", issuer)("receiver", receiver)("attribute_name", attribute_name));
      produce_block();
      return r;
   }

   fc::variant get_attribute_info(const account_name &attribute)
   {
      vector<char> data = get_row_by_account(N(rem.attr), N(rem.attr), N(attrinfo), attribute);
      if (data.empty())
      {
         return fc::variant();
      }
      return rem_attr_abi_ser.binary_to_variant("attribute_info", data, abi_serializer_max_time);
   }

   fc::variant get_account_attribute(const account_name &issuer, const account_name &account, const account_name &attribute)
   {
      const auto &db = control->db();
      const auto *t_id = db.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(N(rem.attr), attribute, N(attributes)));
      if (!t_id)
      {
         return fc::variant();
      }

      const auto &idx = db.get_index<chain::key_value_index, chain::by_scope_primary>();

      vector<char> data;
      for (auto it = idx.lower_bound(boost::make_tuple(t_id->id, 0)); it != idx.end() && it->t_id == t_id->id; it++)
      {
         if (it->value.empty())
         {
            continue;
         }
         data.resize(it->value.size());
         memcpy(data.data(), it->value.data(), data.size());

         const auto attr_obj = rem_attr_abi_ser.binary_to_variant("attribute_data", data, abi_serializer_max_time);
         if (attr_obj["receiver"].as_string() == account.to_string() &&
             attr_obj["issuer"].as_string() == issuer.to_string())
         {
            return attr_obj["attribute"];
         }
      }

      return fc::variant();
   }

   asset get_balance(const account_name &act)
   {
      return get_currency_balance(N(rem.token), symbol(CORE_SYMBOL), act);
   }

   void set_code_abi(const account_name &account, const vector<uint8_t> &wasm, const char *abi, const private_key_type *signer = nullptr)
   {
      wdump((account));
      set_code(account, wasm, signer);
      set_abi(account, abi, signer);
      if (account == N(rem.attr))
      {
         const auto &accnt = control->db().get<account_object, by_name>(account);
         abi_def abi_definition;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
         rem_attr_abi_ser.set_abi(abi_definition, abi_serializer_max_time);
      }
      produce_blocks();
   }

   void transfer(name from, name to, const asset &amount, name manager = config::system_account_name)
   {
      base_tester::push_action(N(rem.token), N(transfer), manager, mutable_variant_object()("from", from)("to", to)("quantity", amount)("memo", ""));
      produce_blocks();
   }

   void print_usage(account_name account)
   {
      auto rlm = control->get_resource_limits_manager();
      auto ram_usage = rlm.get_account_ram_usage(account);
      int64_t ram_bytes;
      int64_t net_weight;
      int64_t cpu_weight;
      rlm.get_account_limits(account, ram_bytes, net_weight, cpu_weight);
      const auto free_bytes = ram_bytes - ram_usage;
      wdump((account)(ram_usage)(free_bytes)(ram_bytes)(net_weight)(cpu_weight));
   }

   abi_serializer rem_attr_abi_ser;
   abi_serializer rem_sys_abi_ser;
};


gift_resources_tester::gift_resources_tester()
{
   create_accounts({N(rem.msig), N(rem.token), N(rem.ram), N(rem.ramfee), N(rem.stake), N(rem.vpay), N(rem.spay), N(rem.saving), N(rem.attr)});
   set_code_abi(N(rem.msig),
                  contracts::rem_msig_wasm(),
                  contracts::rem_msig_abi().data()); //, &rem_active_pk);
   set_code_abi(N(rem.token),
                  contracts::rem_token_wasm(),
                  contracts::rem_token_abi().data()); //, &rem_active_pk);
   set_code_abi(N(rem.attr),
                  contracts::rem_attr_wasm(),
                  contracts::rem_attr_abi().data()); //, &rem_active_pk);

   // Set privileged for rem.msig and rem.token
   set_privileged(N(rem.msig));
   set_privileged(N(rem.token));

   // Verify rem.msig and rem.token is privileged
   const auto &rem_msig_acc = get<account_metadata_object, by_name>(N(rem.msig));
   BOOST_TEST(rem_msig_acc.is_privileged() == true);
   const auto &rem_token_acc = get<account_metadata_object, by_name>(N(rem.token));
   BOOST_TEST(rem_token_acc.is_privileged() == true);

   // Create SYS tokens in rem.token, set its manager as rem
   auto max_supply = core_from_string("1000000000.0000");
   auto initial_supply = core_from_string("900000000.0000");
   create_currency(N(rem.token), config::system_account_name, max_supply);
   // Issue the genesis supply of 1 billion SYS tokens to rem.system
   issue(N(rem.token), config::system_account_name, config::system_account_name, initial_supply);

   auto actual = get_balance(config::system_account_name);
   BOOST_REQUIRE_EQUAL(initial_supply, actual);

   deploy_contract();
}

BOOST_AUTO_TEST_SUITE(gift_resources_tests)

BOOST_FIXTURE_TEST_CASE(acc_creation_with_attr_set, gift_resources_tester)
{
   try
   {
      const auto min_account_stake = get_global_state()["min_account_stake"].as<uint64_t>();
      BOOST_REQUIRE_EQUAL(min_account_stake, 1000000u);
      print_usage(N(rem));
      print_usage(N(rem.stake));

      const auto acc_gifter_attr_name = N(accgifter);

      // creating without transfer, should throw as `accgifter` attribute is not set for `rem`
      {
         BOOST_REQUIRE(get_account_attribute(N(rem.attr), config::system_account_name, acc_gifter_attr_name).is_null());
         
         BOOST_REQUIRE_EXCEPTION(
               create_account_with_resources(N(testram11111), config::system_account_name, asset{100'0000}, false),
               eosio_assert_message_exception, fc_exception_message_starts_with("assertion failure with message: insufficient minimal account stake")
         );
      }

      // create and set `accgifter` attribute to `rem`
      {
         const auto acc_gifter_attr = create_attribute_t{.attr_name = acc_gifter_attr_name, .type = 0, .privacy_type = 3};
         create_attr(acc_gifter_attr.attr_name, acc_gifter_attr.type, acc_gifter_attr.privacy_type);
         
         const auto attr_info = get_attribute_info(acc_gifter_attr.attr_name);
         BOOST_REQUIRE(acc_gifter_attr.attr_name.to_string() == attr_info["attribute_name"].as_string());
         BOOST_REQUIRE(acc_gifter_attr.type == attr_info["type"].as_int64());
         BOOST_REQUIRE(acc_gifter_attr.privacy_type == attr_info["ptype"].as_int64());

         set_attr(N(rem.attr), config::system_account_name, acc_gifter_attr_name, "01");
         BOOST_REQUIRE(get_account_attribute(N(rem.attr), config::system_account_name, acc_gifter_attr_name)["data"].as_string() == "01");
         BOOST_REQUIRE(get_account_attribute(N(rem.attr), config::system_account_name, acc_gifter_attr_name)["pending"].as_string().empty());
      }

      // now `accgifter` attribute is set for `rem` so it can create acc with gifted resources
      {
         create_account_with_resources(N(testram11111), config::system_account_name, asset{100'0000}, false);

         const auto total_stake = get_total_stake(N(testram11111));
         BOOST_REQUIRE_EQUAL(total_stake["own_stake_amount"].as_uint64(), 0);
         BOOST_REQUIRE_EQUAL(total_stake["free_stake_amount"].as_uint64(), 100'0000);
      }

      // transfer resources to testram11111 so free_stake_amount is half covered
      {
         delegate_bandwidth(N(rem.stake), N(testram11111), asset(50'0000));

         const auto total_stake = get_total_stake(N(testram11111));
         BOOST_REQUIRE_EQUAL(total_stake["own_stake_amount"].as_uint64(), 50'0000);
         BOOST_REQUIRE_EQUAL(total_stake["free_stake_amount"].as_uint64(), 50'0000);
      }

      // set `accgifter` attribute to `testram11111` so it can create account with gifted resources
      {
         // fixes `no balance object found`
         transfer( config::system_account_name, N(testram11111), asset{ 1050'000 } );

         set_attr(N(rem.attr), N(testram11111), acc_gifter_attr_name, "01");
         BOOST_REQUIRE(get_account_attribute(N(rem.attr), N(testram11111), acc_gifter_attr_name)["data"].as_string() == "01");
         BOOST_REQUIRE(get_account_attribute(N(rem.attr), N(testram11111), acc_gifter_attr_name)["pending"].as_string().empty());

         create_account_with_resources(N(testram22222), N(testram11111), asset{100'0000}, false);

         const auto total_stake = get_total_stake(N(testram22222));
         BOOST_REQUIRE_EQUAL(total_stake["own_stake_amount"].as_uint64(), 0);
         BOOST_REQUIRE_EQUAL(total_stake["free_stake_amount"].as_uint64(), 100'0000);
      }

      // unset `accgifter` attribute to `testram11111`
      {
         unset_attr(N(rem.attr), N(testram11111), acc_gifter_attr_name);
         BOOST_REQUIRE(get_account_attribute(N(rem.attr), N(testram11111), acc_gifter_attr_name).is_null());

         BOOST_REQUIRE_EXCEPTION(
            create_account_with_resources(N(testram33333), N(testram11111), asset{100'0000}, false),
            eosio_assert_message_exception, fc_exception_message_starts_with("assertion failure with message: insufficient minimal account stake")
         );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace