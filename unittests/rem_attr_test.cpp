/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>

#include <boost/test/unit_test.hpp>

#include <contracts.hpp>

#include <functional>
#include <set>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif


using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

struct genesis_account {
   account_name aname;
   uint64_t     initial_balance;
};


struct create_attribute_t {
   name     attr_name;
   int32_t  type;
   int32_t  privacy_type;
};

struct set_attr_t {
   name         issuer;
   name         receiver;
   name         attribute_name;
   std::string  value;

   set_attr_t(name issuer_, name receiver_, name attribute_name_, std::string value_)
      : issuer(issuer_), receiver(receiver_), attribute_name(attribute_name_), value(value_) {}

   virtual bool isValueOk(std::string actualValue) const = 0;
};

template<typename T>
struct set_attr_expected_t : public set_attr_t {
   T expectedValue;

   set_attr_expected_t(name issuer_, name receiver_, name attribute_name_, std::string value_, T expectedValue_)
      : set_attr_t(issuer_, receiver_, attribute_name_, value_), expectedValue(expectedValue_) {}

   virtual bool isValueOk(std::string actualValue) const {
      bytes b(actualValue.size() / 2);
      const auto size = fc::from_hex(actualValue, b.data(), b.size());

      const auto v = fc::raw::unpack<T>(b);
      return v == expectedValue;
   }
};


class attribute_tester : public TESTER {
public:
   void deploy_contract( bool call_init = true ) {
      set_code( config::system_account_name, contracts::rem_system_wasm() );
      set_abi( config::system_account_name, contracts::rem_system_abi().data() );
      if( call_init ) {
         base_tester::push_action(config::system_account_name, N(init),
                                  config::system_account_name,  mutable_variant_object()
                                  ("version", 0)
                                  ("core", CORE_SYM_STR)
            );
      }
   }

    auto delegate_bandwidth( name from, name receiver, asset stake_quantity, uint8_t transfer = 1) {
       auto r = base_tester::push_action(config::system_account_name, N(delegatebw), from, mvo()
                    ("from", from )
                    ("receiver", receiver)
                    ("stake_quantity", stake_quantity)
                    ("transfer", transfer)
                    );
       produce_block();
       return r;
    }

    void create_currency( name contract, name manager, asset maxsupply, const private_key_type* signer = nullptr ) {
        auto act =  mutable_variant_object()
                ("issuer",       manager )
                ("maximum_supply", maxsupply );

        base_tester::push_action(contract, N(create), contract, act );
    }

    auto issue( name contract, name manager, name to, asset amount ) {
       auto r = base_tester::push_action( contract, N(issue), manager, mutable_variant_object()
                ("to",      to )
                ("quantity", amount )
                ("memo", "")
        );
        produce_block();
        return r;
    }

    auto set_privileged( name account ) {
       auto r = base_tester::push_action(config::system_account_name, N(setpriv), config::system_account_name,  mvo()("account", account)("is_priv", 1));
       produce_block();
       return r;
    }


    auto confirm_attr( name owner, name issuer, name attribute_name ) {
        auto r = base_tester::push_action(N(rem.attr), N(confirm), owner, mvo()
            ("owner", owner)
            ("issuer", issuer)
            ("attribute_name", attribute_name)
        );
        produce_block();
        return r;
    }

    auto create_attr( name attr, int32_t type, int32_t ptype ) {
        auto r = base_tester::push_action(N(rem.attr), N(create), N(rem.attr), mvo()
            ("attribute_name", attr)
            ("type", type)
            ("ptype", ptype)
        );
        produce_block();
        return r;
    }

    auto set_attr( name issuer, name receiver, name attribute_name, std::string value ) {
        auto r = base_tester::push_action(N(rem.attr), N(setattr), issuer, mvo()
            ("issuer", issuer)
            ("receiver", receiver)
            ("attribute_name", attribute_name)
            ("value", value)
        );
        produce_block();
        return r;
    }

    auto unset_attr( name issuer, name receiver, name attribute_name ) {
        auto r = base_tester::push_action(N(rem.attr), N(unsetattr), issuer, mvo()
            ("issuer", issuer)
            ("receiver", receiver)
            ("attribute_name", attribute_name)
        );
        produce_block();
        return r;
    }

    auto invalidate_attr( name attribute_name ) {
        auto r = base_tester::push_action(N(rem.attr), N(invalidate), N(rem.attr), mvo()
            ("attribute_name", attribute_name)
        );
        produce_block();
        return r;
    }

    auto remove_attr( name attribute_name ) {
        auto r = base_tester::push_action(N(rem.attr), N(remove), N(rem.attr), mvo()
            ("attribute_name", attribute_name)
        );
        produce_block();
        return r;
    }

    fc::variant get_attribute_info( const account_name& attribute ) {
        vector<char> data = get_row_by_account( N(rem.attr), N(rem.attr), N(attrinfo), attribute );
        if (data.empty()) {
            return fc::variant();
        }
        return abi_ser.binary_to_variant( "attribute_info", data, abi_serializer_max_time );
    }

    fc::variant get_account_attribute( const account_name& account, const account_name& issuer, const account_name& attribute ) {
      //TODO: figure out how to retrieve object by secondary index
//       eosio::chain::uint128_t secondary_index_value = account.value;
//       secondary_index_value <<= 64;
//       secondary_index_value |= issuer.value;

       const auto& db = control->db();
       const auto* t_id = db.find<chain::table_id_object, chain::by_code_scope_table>( boost::make_tuple( N(rem.attr), attribute, N(attributes) ) );
       if ( !t_id ) {
          return fc::variant();
       }

       const auto& idx = db.get_index<chain::key_value_index, chain::by_scope_primary>();

       vector<char> data;
       for (auto it = idx.lower_bound( boost::make_tuple( t_id->id, 0 ) ); it != idx.end() && it->t_id == t_id->id; it++) {
          if (it->value.empty()) {
             continue;
          }
          data.resize( it->value.size() );
          memcpy( data.data(), it->value.data(), data.size() );

          const auto attr_obj = abi_ser.binary_to_variant( "attribute_data", data, abi_serializer_max_time );
          if (attr_obj["receiver"].as_string() == account.to_string() &&
              attr_obj["issuer"].as_string() == issuer.to_string()) {
             return attr_obj["attribute"];
          }
       }
        return fc::variant();
    }

    asset get_balance( const account_name& act ) {
         return get_currency_balance(N(rem.token), symbol(CORE_SYMBOL), act);
    }

    void set_code_abi(const account_name& account, const vector<uint8_t>& wasm, const char* abi, const private_key_type* signer = nullptr) {
       wdump((account));
        set_code(account, wasm, signer);
        set_abi(account, abi, signer);
        if (account == N(rem.attr)) {
           const auto& accnt = control->db().get<account_object,by_name>( account );
           abi_def abi_definition;
           BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi_definition), true);
           abi_ser.set_abi(abi_definition, abi_serializer_max_time);
        }
        produce_blocks();
    }

    abi_serializer abi_ser;
};

BOOST_AUTO_TEST_SUITE(rem_attribute_tests)

BOOST_FIXTURE_TEST_CASE( attribute_test, attribute_tester ) {
    try {
        create_accounts({N(rem.msig), N(rem.token), N(rem.rex), N(rem.ram),
                         N(rem.ramfee), N(rem.stake), N(rem.bpay),
                         N(rem.spay), N(rem.vpay), N(rem.saving), N(rem.attr)});
        // Set code for the following accounts:
        //  - rem (code: rem.bios) (already set by tester constructor)
        //  - rem.msig (code: rem.msig)
        //  - rem.token (code: rem.token)
        // set_code_abi(N(rem.msig), contracts::rem_msig_wasm(), contracts::rem_msig_abi().data());//, &rem_active_pk);
        // set_code_abi(N(rem.token), contracts::rem_token_wasm(), contracts::rem_token_abi().data()); //, &rem_active_pk);

        set_code_abi(N(rem.msig),
                     contracts::rem_msig_wasm(),
                     contracts::rem_msig_abi().data());//, &rem_active_pk);
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
        const auto& rem_msig_acc = get<account_metadata_object, by_name>(N(rem.msig));
        BOOST_TEST(rem_msig_acc.is_privileged() == true);
        const auto& rem_token_acc = get<account_metadata_object, by_name>(N(rem.token));
        BOOST_TEST(rem_token_acc.is_privileged() == true);


        // Create SYS tokens in rem.token, set its manager as rem
        auto max_supply = core_from_string("1000000000.0000");
        auto initial_supply = core_from_string("900000000.0000");
        create_currency(N(rem.token), config::system_account_name, max_supply);
        // Issue the genesis supply of 1 billion SYS tokens to rem.system
        issue(N(rem.token), config::system_account_name, config::system_account_name, initial_supply);

        auto actual = get_balance(config::system_account_name);
        BOOST_REQUIRE_EQUAL(initial_supply, actual);

        // Create genesis accounts

        std::vector<genesis_account> test_genesis( {
            {N(b1),       100'000'000'0000ll},
            {N(whale4),    40'000'000'0000ll},
            {N(whale3),    30'000'000'0000ll},
            {N(whale2),    20'000'000'0000ll},
            {N(proda),      1'000'000'0000ll},
            {N(prodb),      1'000'000'0000ll},
            {N(prodc),      1'000'000'0000ll}
        });
        for( const auto& a : test_genesis ) {
           create_account( a.aname, config::system_account_name );
        }

        deploy_contract();

        // Buy ram and stake cpu and net for each genesis accounts
        for( const auto& a : test_genesis ) {
           auto stake_quantity = a.initial_balance - 1000;

           auto r = delegate_bandwidth(N(rem.stake), a.aname, asset(stake_quantity));
           BOOST_REQUIRE( !r->except_ptr );
        }

        std::vector<create_attribute_t> attributes = {
            { .attr_name=N(crosschain),  .type=3,  .privacy_type=1 },  // type: ChainAccount, privacy type: PublicPointer
            { .attr_name=N(tags),        .type=9,  .privacy_type=4 },  // type: Set,          privacy type: PrivateConfirmedPointer
            { .attr_name=N(creator),     .type=0,  .privacy_type=3 },  // type: Bool,         privacy type: PrivatePointer
            { .attr_name=N(name),        .type=4,  .privacy_type=0 },  // type: UTFString,    privacy type: SelfAssigned
            { .attr_name=N(largeint),    .type=2,  .privacy_type=2 },  // type: LargeInt,     privacy type: PublicConfirmedPointer
        };
        for (const auto& a: attributes) {
            create_attr(a.attr_name, a.type, a.privacy_type);
            const auto attr_info = get_attribute_info(a.attr_name);
            BOOST_TEST( a.attr_name.to_string() == attr_info["attribute_name"].as_string() );
            BOOST_TEST( a.type == attr_info["type"].as_int64() );
            BOOST_TEST( a.privacy_type == attr_info["ptype"].as_int64() );
        }
        BOOST_REQUIRE_THROW(base_tester::push_action(N(rem.attr), N(create), N(b1), mvo()("attribute_name", name(N(fail)))("type", 0)("ptype", 0)),
            missing_auth_exception);
        BOOST_REQUIRE_THROW(create_attr(N(fail), -1, 0), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(create_attr(N(fail), 10, 0), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(create_attr(N(fail), 0, -1), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(create_attr(N(fail), 0, 5), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(create_attr(N(tags), 0, 0), eosio_assert_message_exception);

        set_attr_expected_t<std::pair<fc::sha256, name>> attr1(N(prodb), N(proda), N(crosschain),
            // concatenated "00000000000000000000000000000000000000000000000000000000000000ff" and "rem" in hex
            "00000000000000000000000000000000000000000000000000000000000000ff000000000000A4BA",
            std::pair<chain_id_type, name>("00000000000000000000000000000000000000000000000000000000000000ff", N(rem)));
        set_attr_expected_t<std::pair<fc::sha256, name>> attr2(N(prodb), N(proda), N(crosschain),
            // concatenated "000000000000000000000000000000000000000000000000000000000000ffff" and "eosio" in hex
            "000000000000000000000000000000000000000000000000000000000000ffff0000000000EA3055",
            std::pair<chain_id_type, name>("000000000000000000000000000000000000000000000000000000000000ffff", N(eosio)));
        set_attr_expected_t<std::set<std::pair<std::string, std::string>>> attr3(N(rem.attr), N(proda), N(tags),
            "03013101320133013401350136", // encoded set of pairs
            std::set<std::pair<std::string, std::string>>{ std::pair("1", "2"), std::pair("3", "4"), std::pair("5", "6") });
        set_attr_expected_t<std::set<std::pair<std::string, std::string>>> attr4(N(rem.attr), N(rem.attr), N(tags),
            "02013103323334023536023738", // encoded set of pairs
            std::set<std::pair<std::string, std::string>>{ std::pair("1", "234"), std::pair("56", "78") });
        set_attr_expected_t<bool> attr5(N(rem.attr), N(prodb), N(creator),
            "00", // encoded false
            false);
        set_attr_expected_t<bool> attr6(N(rem.attr), N(rem.attr), N(creator),
            "01", // encoded false
            true);
        set_attr_expected_t<std::string> attr7(N(prodb), N(prodb), N(name),
            "06426c61697a65", // encoded string "Blaize"
            "Blaize");
        set_attr_expected_t<int64_t> attr8(N(proda), N(prodb), N(largeint),
            "0000000000000080", // encoded number -9223372036854775808
            -9223372036854775808);
        set_attr_expected_t<int64_t> attr9(N(prodb), N(prodb), N(largeint),
           "ffffffffffffffff", // encoded number -1
           -1);
        for (auto& attr: std::vector<std::reference_wrapper<set_attr_t>>{ attr1, attr2, attr3, attr4, attr5, attr6, attr7, attr8, attr9 }) {
            auto& attr_ref = attr.get();
            set_attr(attr_ref.issuer, attr_ref.receiver, attr_ref.attribute_name, attr_ref.value);

            const auto attribute_obj = get_account_attribute(attr_ref.receiver, attr_ref.issuer, attr_ref.attribute_name);
            auto data = attribute_obj["data"].as_string();
            if (data.empty()) {
               data = attribute_obj["pending"].as_string();
            }
            BOOST_TEST(attr_ref.isValueOk(data));
        }

        // issuer b1 does not have authorization
        BOOST_REQUIRE_THROW(base_tester::push_action(N(rem.attr), N(setattr), N(proda), mvo()("issuer", "b1")("receiver", "prodb")("attribute_name", "crosschain")("value", "00000000000000000000000000000000000000000000000000000000000000ff000000000000A4BA")),
            missing_auth_exception);
        // non-existing attribute
        BOOST_REQUIRE_THROW(set_attr(N(proda), N(prodb), N(nonexisting), ""), eosio_assert_message_exception);
        // empty value
        BOOST_REQUIRE_THROW(set_attr(N(proda), N(prodb), N(crosschain), ""), eosio_assert_message_exception);
        // Privacy checks:
        // only rem.attr is allowed to set PrivateConfirmedPointer
        BOOST_REQUIRE_THROW(set_attr(N(prodb), N(proda), N(tags), "03013101320133013401350136"), eosio_assert_message_exception);
        // only rem.attr is allowed to set PrivatePointer
        BOOST_REQUIRE_THROW(set_attr(N(proda), N(proda), N(creator), "01"), eosio_assert_message_exception);
        // only proda is allowed to assign SelfAssigned attribute to proda
        BOOST_REQUIRE_THROW(set_attr(N(rem.attr), N(proda), N(name), "06426c61697a65"), eosio_assert_message_exception);

        //check that PublicConfirmedPointer/PrivateConfirmedPointer attributes are not confirmed
        BOOST_TEST(get_account_attribute(    N(proda),     N(prodb),    N(crosschain)) ["data"].as_string()    != "");
        BOOST_TEST(get_account_attribute(    N(proda),     N(prodb),    N(crosschain)) ["pending"].as_string() == "");
        BOOST_TEST(get_account_attribute(    N(proda),  N(rem.attr),          N(tags)) ["data"].as_string()    == "");
        BOOST_TEST(get_account_attribute(    N(proda),  N(rem.attr),          N(tags)) ["pending"].as_string() != ""); //not confirmed
        BOOST_TEST(get_account_attribute( N(rem.attr),  N(rem.attr),          N(tags)) ["data"].as_string()    == "");
        BOOST_TEST(get_account_attribute( N(rem.attr),  N(rem.attr),          N(tags)) ["pending"].as_string() != ""); //not confirmed
        BOOST_TEST(get_account_attribute(    N(prodb),  N(rem.attr),       N(creator)) ["data"].as_string()    != "");
        BOOST_TEST(get_account_attribute(    N(prodb),  N(rem.attr),       N(creator)) ["pending"].as_string() == "");
        BOOST_TEST(get_account_attribute( N(rem.attr),  N(rem.attr),       N(creator)) ["data"].as_string()    != "");
        BOOST_TEST(get_account_attribute( N(rem.attr),  N(rem.attr),       N(creator)) ["pending"].as_string() == "");
        BOOST_TEST(get_account_attribute(    N(prodb),     N(prodb),          N(name)) ["data"].as_string()    != "");
        BOOST_TEST(get_account_attribute(    N(prodb),     N(prodb),          N(name)) ["pending"].as_string() == "");
        BOOST_TEST(get_account_attribute(    N(prodb),     N(proda),      N(largeint)) ["data"].as_string()    == "");
        BOOST_TEST(get_account_attribute(    N(prodb),     N(proda),      N(largeint)) ["pending"].as_string() != ""); //not confirmed
        BOOST_TEST(get_account_attribute(    N(prodb),     N(prodb),      N(largeint)) ["data"].as_string()    == "");
        BOOST_TEST(get_account_attribute(    N(prodb),     N(prodb),      N(largeint)) ["pending"].as_string() != ""); //not confirmed

        // only proda is allowed to confirm his attributes
        BOOST_REQUIRE_THROW(base_tester::push_action(N(rem.attr), N(confirm), N(rem.attr), mvo()("owner", "proda")("issuer", "prodb")("attribute_name", "tags")),
            missing_auth_exception);
        // account prodb does not have this attribute set by prodc
        BOOST_REQUIRE_THROW(confirm_attr(N(prodb), N(prodc), N(largeint)), eosio_assert_message_exception);
        // PublicPointer doesnot need to be confirmed
        BOOST_REQUIRE_THROW(confirm_attr(N(proda), N(prodb), N(crosschain)), eosio_assert_message_exception);
        // account proda does not have this attribute set by rem.attr
        BOOST_REQUIRE_THROW(confirm_attr(N(proda), N(rem.attr), N(largeint)), eosio_assert_message_exception);
        // non-existing attribute
        BOOST_REQUIRE_THROW(confirm_attr(N(proda), N(prodb), N(nonexisting)), eosio_assert_message_exception);

        // Confirm unconfirmed and check
        confirm_attr(    N(proda),  N(rem.attr),      N(tags));
        confirm_attr( N(rem.attr),  N(rem.attr),      N(tags));
        confirm_attr(    N(prodb),     N(proda),  N(largeint));
        confirm_attr(    N(prodb),     N(prodb),  N(largeint));
        BOOST_TEST(get_account_attribute(    N(proda), N(rem.attr),          N(tags)) ["data"].as_string()    != "");
        BOOST_TEST(get_account_attribute(    N(proda), N(rem.attr),          N(tags)) ["pending"].as_string() == "");
        BOOST_TEST(get_account_attribute( N(rem.attr), N(rem.attr),          N(tags)) ["data"].as_string()    != "");
        BOOST_TEST(get_account_attribute( N(rem.attr), N(rem.attr),          N(tags)) ["pending"].as_string() == "");
        BOOST_TEST(get_account_attribute(    N(prodb),    N(proda),      N(largeint)) ["data"].as_string()    != "");
        BOOST_TEST(get_account_attribute(    N(prodb),    N(proda),      N(largeint)) ["pending"].as_string() == "");
        BOOST_TEST(get_account_attribute(    N(prodb),    N(prodb),      N(largeint)) ["data"].as_string()    != "");
        BOOST_TEST(get_account_attribute(    N(prodb),    N(prodb),      N(largeint)) ["pending"].as_string() == "");
        // Reset attribute
        set_attr(N(rem.attr), N(rem.attr), N(tags), "02013103323334023536023739");
        // Check that it hasn`t been confirmed yet
        BOOST_TEST(get_account_attribute( N(rem.attr),  N(rem.attr),          N(tags)) ["pending"].as_string() != ""); //not confirmed
        // Confirm attribute
        confirm_attr(N(rem.attr), N(rem.attr), N(tags));
        // Check that it have been confirmed
        BOOST_TEST(get_account_attribute( N(rem.attr),  N(rem.attr),          N(tags)) ["pending"].as_string() == ""); //confirmed

        BOOST_REQUIRE_THROW(unset_attr(N(proda),      N(proda),      N(nonexisting)),  eosio_assert_message_exception); //nonexisting
        BOOST_REQUIRE_THROW(unset_attr(N(prodc),      N(prodc),      N(crosschain)),   eosio_assert_message_exception); //account does not have attribute
        BOOST_REQUIRE_THROW(unset_attr(N(proda),      N(proda),      N(crosschain)),   eosio_assert_message_exception); //only issuer can unset
        BOOST_REQUIRE_THROW(unset_attr(N(prodc),      N(proda),      N(tags)),         eosio_assert_message_exception); //only issuer or receiver can unset
        BOOST_REQUIRE_THROW(unset_attr(N(prodb),      N(rem.attr),   N(tags)),         eosio_assert_message_exception); //only issuer or receiver can unset
        BOOST_REQUIRE_THROW(unset_attr(N(proda),      N(proda),      N(creator)),      eosio_assert_message_exception); //only contract owner can unset
        BOOST_REQUIRE_THROW(unset_attr(N(rem.attr),   N(proda),      N(name)),         eosio_assert_message_exception); //only receiver can unset
        BOOST_REQUIRE_THROW(unset_attr(N(prodc),      N(prodb),      N(largeint)),     eosio_assert_message_exception); //only issuer or receiver can unset

        BOOST_REQUIRE_THROW(remove_attr(N(crosschain)), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(remove_attr(N(tags)), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(remove_attr(N(creator)), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(remove_attr(N(name)), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(remove_attr(N(largeint)), eosio_assert_message_exception);

        unset_attr(N(prodb),      N(proda),      N(crosschain));
        unset_attr(N(rem.attr),   N(proda),      N(tags));
        unset_attr(N(rem.attr),   N(rem.attr),   N(tags));
        unset_attr(N(rem.attr),   N(prodb),      N(creator));
        unset_attr(N(rem.attr),   N(rem.attr),   N(creator));
        unset_attr(N(prodb),      N(prodb),      N(name));
        unset_attr(N(prodb),      N(prodb),      N(largeint));
        // receiver can unset PublicConfirmedPointer
        base_tester::push_action(N(rem.attr), N(unsetattr), N(prodb), mvo()("issuer", "proda")("receiver", "prodb")("attribute_name", "largeint"));
        produce_block();
        BOOST_TEST(get_account_attribute( N(proda),        N(prodb),  N(crosschain)).is_null());
        BOOST_TEST(get_account_attribute( N(proda),     N(rem.attr),        N(tags)).is_null());
        BOOST_TEST(get_account_attribute( N(rem.attr),  N(rem.attr),        N(tags)).is_null());
        BOOST_TEST(get_account_attribute( N(prodb),     N(rem.attr),     N(creator)).is_null());
        BOOST_TEST(get_account_attribute( N(rem.attr),  N(rem.attr),     N(creator)).is_null());
        BOOST_TEST(get_account_attribute( N(prodb),        N(prodb),        N(name)).is_null());
        BOOST_TEST(get_account_attribute( N(prodb),        N(prodb),    N(largeint)).is_null());
        BOOST_TEST(get_account_attribute( N(prodb),        N(proda),    N(largeint)).is_null());

        BOOST_REQUIRE_THROW(remove_attr(N(crosschain)), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(remove_attr(N(tags)), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(remove_attr(N(creator)), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(remove_attr(N(name)), eosio_assert_message_exception);
        BOOST_REQUIRE_THROW(remove_attr(N(largeint)), eosio_assert_message_exception);

        invalidate_attr(N(crosschain));
        invalidate_attr(N(tags));
        invalidate_attr(N(creator));
        invalidate_attr(N(name));
        invalidate_attr(N(largeint));

        remove_attr(N(crosschain));
        remove_attr(N(tags));
        remove_attr(N(creator));
        remove_attr(N(name));
        remove_attr(N(largeint));

    } FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
