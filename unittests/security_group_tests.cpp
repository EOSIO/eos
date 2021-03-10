#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <contracts.hpp>


namespace eosio { namespace chain {

// block_header_state_v0 should has the same layout with block_header_state except
// the lack of the state_extension data member
struct block_header_state_v0 : public detail::block_header_state_common {
   block_id_type                        id;
   signed_block_header                  header;
   detail::schedule_info                pending_schedule;
   protocol_feature_activation_set_ptr  activated_protocol_features;
   vector<signature_type>               additional_signatures;

   /// this data is redundant with the data stored in header, but it acts as a cache that avoids
   /// duplication of work
   flat_multimap<uint16_t, block_header_extension> header_exts;
};

struct snapshot_global_property_object_v5 {
   std::optional<block_num_type> proposed_schedule_block_num;
   producer_authority_schedule   proposed_schedule;
   chain_config                  configuration;
   chain_id_type                 chain_id;
   kv_database_config            kv_configuration;
   wasm_config                   wasm_configuration;

   snapshot_global_property_object_v5(const global_property_object& value)
       : proposed_schedule_block_num(value.proposed_schedule_block_num)
       , proposed_schedule(producer_authority_schedule::from_shared(value.proposed_schedule))
       , configuration(value.configuration)
       , chain_id(value.chain_id)
       , kv_configuration(value.kv_configuration)
       , wasm_configuration(value.wasm_configuration) {}
};

bool operator == (const security_group_info_t& lhs, const security_group_info_t& rhs) {
   return lhs.version == rhs.version && std::equal(lhs.participants.begin(), lhs.participants.end(),
                                                   rhs.participants.begin(), rhs.participants.end());
}
}}

FC_REFLECT_DERIVED(  eosio::chain::block_header_state_v0, (eosio::chain::detail::block_header_state_common),
                     (id)
                     (header)
                     (pending_schedule)
                     (activated_protocol_features)
                     (additional_signatures)
)

FC_REFLECT(eosio::chain::snapshot_global_property_object_v5,
            (proposed_schedule_block_num)(proposed_schedule)(configuration)(chain_id)(kv_configuration)(wasm_configuration)
          )


BOOST_AUTO_TEST_SUITE(security_group_tests)

using participants_t = boost::container::flat_set<eosio::chain::account_name>;

BOOST_AUTO_TEST_CASE(test_unpack_legacy_block_state) {
   eosio::testing::tester main;

   using namespace eosio::chain::literals;

   // First we create a valid block with valid transaction
   main.create_account("newacc"_n);
   auto b = main.produce_block();

   auto bs = main.control->head_block_state();

   // pack block_header_state as the legacy format
   fc::datastream<std::vector<char>> out_strm;
   fc::raw::pack(out_strm, reinterpret_cast<eosio::chain::block_header_state_v0&>(*bs));

   BOOST_CHECK_NE(out_strm.storage().size(), 0);

   // make sure we can unpack block_header_state 
   {
      fc::datastream<const char*> in_strm(out_strm.storage().data(), out_strm.storage().size());
      eosio::chain::versioned_unpack_stream unpack_strm(in_strm, eosio::chain::block_header_state::minimum_snapshot_version_with_state_extension-1);
      eosio::chain::block_header_state tmp;
      BOOST_CHECK_NO_THROW(fc::raw::unpack(unpack_strm, tmp));
      BOOST_CHECK_EQUAL(bs->id, tmp.id);
      BOOST_CHECK_EQUAL(in_strm.remaining(), 0);
   }

   // manual pack legacy block_state
   fc::raw::pack(out_strm, bs->block);
   fc::raw::pack(out_strm, false);

   // make sure we can unpack block_state 
   {
      fc::datastream<const char*> in_strm(out_strm.storage().data(), out_strm.storage().size());
      eosio::chain::versioned_unpack_stream unpack_strm(in_strm, eosio::chain::block_header_state::minimum_snapshot_version_with_state_extension-1);
      eosio::chain::block_state tmp;
      BOOST_CHECK_NO_THROW(fc::raw::unpack(unpack_strm, tmp));
      BOOST_CHECK_EQUAL(bs->id, tmp.id);
      BOOST_CHECK_EQUAL(bs->block->previous, tmp.block->previous);
      BOOST_CHECK_EQUAL(in_strm.remaining(), 0);
   }
}

BOOST_AUTO_TEST_CASE(test_unpack_new_block_state) {
   eosio::testing::tester main;

   using namespace eosio::chain::literals;

   // First we create a valid block with valid transaction
   main.create_account("newacc"_n);
   auto b = main.produce_block();

   auto bs = main.control->head_block_state();
   bs->set_security_group_info( { .version =1, .participants={"adam"_n }} );
   

   // pack block_header_state as the legacy format
   fc::datastream<std::vector<char>> out_strm;
   fc::raw::pack(out_strm, *bs);

   BOOST_CHECK_NE(out_strm.storage().size(), 0);


   // make sure we can unpack block_state 
   {
      fc::datastream<const char*> in_strm(out_strm.storage().data(), out_strm.storage().size());
      eosio::chain::versioned_unpack_stream unpack_strm(in_strm, eosio::chain::block_header_state::minimum_snapshot_version_with_state_extension);
      eosio::chain::block_state tmp;
      BOOST_CHECK_NO_THROW(fc::raw::unpack(unpack_strm, tmp));
      BOOST_CHECK_EQUAL(bs->id, tmp.id);
      BOOST_CHECK_EQUAL(bs->block->previous, tmp.block->previous);
      BOOST_CHECK_EQUAL(bs->get_security_group_info().version, tmp.get_security_group_info().version);
      BOOST_TEST(bs->get_security_group_info().participants == tmp.get_security_group_info().participants);
      BOOST_CHECK_EQUAL(in_strm.remaining(), 0);
   }
}

BOOST_AUTO_TEST_CASE(test_snapshot_global_property_object) {
   eosio::testing::tester main;

   using namespace eosio::chain::literals;

   // First we create a valid block with valid transaction
   main.create_account("newacc"_n);
   auto b = main.produce_block();

   auto gpo = main.control->get_global_properties();

   using row_traits = eosio::chain::detail::snapshot_row_traits<eosio::chain::global_property_object>;

   {
      // pack snapshot_global_property_object as the legacy format
      fc::datastream<std::vector<char>> out_strm;
      fc::raw::pack(out_strm, eosio::chain::snapshot_global_property_object_v5(gpo));

      // make sure we can unpack snapshot_global_property_object
      {
         fc::datastream<const char*>           in_strm(out_strm.storage().data(), out_strm.storage().size());
         uint32_t                              version = 5;
         eosio::chain::versioned_unpack_stream unpack_strm(in_strm, version);

         eosio::chain::snapshot_global_property_object row;
         BOOST_CHECK_NO_THROW(fc::raw::unpack(unpack_strm, row));
         BOOST_CHECK_EQUAL(in_strm.remaining(), 0);
         BOOST_CHECK_EQUAL(row.chain_id, gpo.chain_id);
      }
   }

   {
      // pack snapshot_global_property_object as the new format
      gpo.proposed_security_group_block_num = 10;
      gpo.proposed_security_group_participants = {"adam"_n};

      fc::datastream<std::vector<char>> out_strm;
      fc::raw::pack(out_strm, row_traits::to_snapshot_row(gpo, main.control->db()));

      // make sure we can unpack snapshot_global_property_object
      {
         fc::datastream<const char*>           in_strm(out_strm.storage().data(), out_strm.storage().size());
         eosio::chain::versioned_unpack_stream unpack_strm(in_strm, eosio::chain::snapshot_global_property_object::minimum_version_with_extension);

         eosio::chain::snapshot_global_property_object row;
         BOOST_CHECK_NO_THROW(fc::raw::unpack(unpack_strm, row));
         BOOST_CHECK_EQUAL(in_strm.remaining(), 0);

         BOOST_CHECK_EQUAL(row.chain_id, gpo.chain_id);
         std::visit(
             [&gpo](const auto& ext) {
                BOOST_CHECK_EQUAL(ext.proposed_security_group_block_num, gpo.proposed_security_group_block_num);
                BOOST_TEST(ext.proposed_security_group_participants == gpo.proposed_security_group_participants );
             },
             row.extension);
      }
   }
}

BOOST_AUTO_TEST_CASE(test_participants_change) {
   eosio::testing::tester chain;
   using namespace eosio::chain::literals;

   chain.create_accounts( {"alice"_n,"bob"_n,"charlie"_n} );
   chain.produce_block();
   
   {
      const auto& cur_security_group = chain.control->active_security_group();
      BOOST_REQUIRE_EQUAL(cur_security_group.version, 0);
      BOOST_REQUIRE_EQUAL(cur_security_group.participants.size(), 0);
   }

   participants_t new_participants({"alice"_n, "bob"_n});
   chain.control->add_security_group_participants(new_participants);

   BOOST_TEST(chain.control->proposed_security_group_participants() == new_participants);
   BOOST_CHECK_EQUAL(chain.control->active_security_group().participants.size() , 0);

   chain.produce_block();

   BOOST_CHECK_EQUAL(chain.control->proposed_security_group_participants().size() , 0);
   BOOST_TEST(chain.control->active_security_group().participants == new_participants);
   BOOST_CHECK(chain.control->in_active_security_group(participants_t({"alice"_n, "bob"_n})));
   BOOST_CHECK(!chain.control->in_active_security_group(participants_t{"bob"_n, "charlie"_n}));

   chain.control->remove_security_group_participants({"alice"_n});
   BOOST_TEST(chain.control->proposed_security_group_participants() == participants_t{"bob"_n});

   chain.produce_block();

   BOOST_CHECK_EQUAL(chain.control->proposed_security_group_participants().size() , 0);
   BOOST_TEST(chain.control->active_security_group().participants == participants_t{"bob"_n});
   BOOST_CHECK(chain.control->in_active_security_group(participants_t{"bob"_n}));

}

void push_blocks( eosio::testing::tester& from, eosio::testing::tester& to ) {
   while( to.control->fork_db_pending_head_block_num()
            < from.control->fork_db_pending_head_block_num() )
   {
      auto fb = from.control->fetch_block_by_number( to.control->fork_db_pending_head_block_num()+1 );
      to.push_block( fb );
   }
}

// The webassembly in text format to add security group participants
static const char add_security_group_participants_wast[] =  R"=====(
(module
 (func $action_data_size (import "env" "action_data_size") (result i32))
 (func $read_action_data (import "env" "read_action_data") (param i32 i32) (result i32))
 (func $add_security_group_participants (import "env" "add_security_group_participants") (param i32 i32)(result i64))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
   (local $bytes_remaining i32)
   (set_local $bytes_remaining (call $action_data_size))
   (drop (call $read_action_data (i32.const 0) (get_local $bytes_remaining)))
   (drop (call $add_security_group_participants (i32.const 0) (get_local $bytes_remaining)))
 )
)
)=====";

// The webassembly in text format to remove security group participants
static const char remove_security_group_participants_wast[] =  R"=====(
(module
 (func $action_data_size (import "env" "action_data_size") (result i32))
 (func $read_action_data (import "env" "read_action_data") (param i32 i32) (result i32))
 (func $remove_security_group_participants (import "env" "remove_security_group_participants") (param i32 i32)(result i64))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
   (local $bytes_remaining i32)
   (set_local $bytes_remaining (call $action_data_size))
   (drop (call $read_action_data (i32.const 0) (get_local $bytes_remaining)))
   (drop (call $remove_security_group_participants (i32.const 0) (get_local $bytes_remaining)))
 )
)
)=====";

// The webassembly in text format to assert the given participants are all in active security group
static const char assert_in_security_group_wast[] =  R"=====(
(module
 (func $action_data_size (import "env" "action_data_size") (result i32))
 (func $read_action_data (import "env" "read_action_data") (param i32 i32) (result i32))
 (func $in_active_security_group (import "env" "in_active_security_group") (param i32 i32)(result i32))
 (func $eosio_assert (import "env" "eosio_assert") (param i32 i32))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
   (local $bytes_remaining i32)
   (local $in_group i32)
   (set_local $bytes_remaining (call $action_data_size))
   (drop (call $read_action_data (i32.const 0) (get_local $bytes_remaining)))
   (set_local $in_group (call $in_active_security_group (i32.const 0) (get_local $bytes_remaining)))
   (call $eosio_assert (get_local $in_group)(i32.const 512))
 )
 (data (i32.const 512) "in_active_security_group should return true")
)
)=====";

// The webassembly in text format to assert the given participants are exactly the entire active security group
static const char assert_get_security_group_wast[] =  R"=====(
(module
 (func $action_data_size (import "env" "action_data_size") (result i32))
 (func $read_action_data (import "env" "read_action_data") (param i32 i32) (result i32))
 (func $get_active_security_group (import "env" "get_active_security_group") (param i32 i32)(result i32))
 (func $eosio_assert (import "env" "eosio_assert") (param i32 i32))
 (func $memcmp (import "env" "memcmp") (param i32 i32 i32) (result i32))
 (memory 1)
 (func (export "apply") (param i64 i64 i64)
   (local $bytes_remaining i32)
   (local $required_bytes i32)
   (set_local $bytes_remaining (call $action_data_size))
   (drop (call $read_action_data (i32.const 0) (get_local $bytes_remaining)))
   (set_local $required_bytes (call $get_active_security_group (i32.const 0) (i32.const 0)))
   (call $eosio_assert (i32.eq (i32.const 13) (get_local $required_bytes))(i32.const 512))
   (drop (call $get_active_security_group (i32.const 256) (get_local $required_bytes)))
   (call $eosio_assert (i32.eq (call $memcmp (i32.const 0) (i32.const 256) (get_local $required_bytes))(i32.const 0)) (i32.const 768)) 
 )
 (data (i32.const 512) "get_active_security_group should return the right size")
 (data (i32.const 768) "get_active_security_group output buffer must match the input")
)
)=====";

std::vector<char> participants_payload( participants_t names ) {
   fc::datastream<std::vector<char>> ds;
   fc::raw::pack(ds, names);
   return ds.storage();
}

BOOST_AUTO_TEST_CASE(test_security_group_intrinsic) {

   eosio::testing::tester chain1;
   using namespace eosio::chain::literals;

   chain1.create_accounts( {"alice"_n,"bob"_n,"charlie"_n} );
   chain1.produce_blocks(3);

   chain1.create_accounts({ "addmember"_n, "rmmember"_n, "ingroup"_n, "getgroup"_n });
   chain1.produce_block();

   chain1.set_code( "addmember"_n, add_security_group_participants_wast );
   chain1.set_code( "rmmember"_n, remove_security_group_participants_wast );
   chain1.set_code( "ingroup"_n, assert_in_security_group_wast );
   chain1.set_code( "getgroup"_n, assert_get_security_group_wast );
   chain1.produce_block();
   
   chain1.push_action( "eosio"_n, "setpriv"_n, "eosio"_n, fc::mutable_variant_object()("account", "addmember"_n)("is_priv", 1));
   chain1.push_action( "eosio"_n, "setpriv"_n, "eosio"_n, fc::mutable_variant_object()("account", "rmmember"_n)("is_priv", 1));
   chain1.produce_blocks(24);

   chain1.set_producers( {"alice"_n,"bob"_n} );
   chain1.produce_blocks(3); // Starts new blocks which promotes the proposed schedule to pending
   BOOST_REQUIRE_EQUAL( chain1.control->active_producers().version, 1u );

   BOOST_TEST_REQUIRE(chain1.push_action(  eosio::chain::action({}, "addmember"_n, {}, participants_payload({"alice"_n})), "addmember"_n.to_uint64_t() ) == "");
   chain1.produce_block();
   BOOST_TEST_REQUIRE(chain1.push_action(  eosio::chain::action({}, "addmember"_n, {}, participants_payload({"bob"_n})), "addmember"_n.to_uint64_t() ) == "");
   chain1.produce_blocks(10+11);
   BOOST_CHECK_EQUAL(chain1.control->proposed_security_group_participants().size() , 2);
   BOOST_CHECK_EQUAL(chain1.control->active_security_group().participants.size(), 0);
   chain1.produce_blocks(1);
   BOOST_CHECK_EQUAL(chain1.control->proposed_security_group_participants().size() , 0);
   BOOST_CHECK(chain1.control->in_active_security_group(participants_t({"alice"_n, "bob"_n})));

   BOOST_TEST_REQUIRE(chain1.push_action(  eosio::chain::action({}, "rmmember"_n, {}, participants_payload({"alice"_n})), "rmmember"_n.to_uint64_t() ) == "");
   BOOST_CHECK_EQUAL(chain1.control->proposed_security_group_participants().size() , 1);
   chain1.produce_blocks(11+12);
   BOOST_CHECK(!chain1.control->in_active_security_group(participants_t({"alice"_n})));

   BOOST_TEST_REQUIRE(chain1.push_action(  eosio::chain::action({}, "ingroup"_n, {}, participants_payload({"bob"_n})), "ingroup"_n.to_uint64_t() ) == "");

   eosio::chain::security_group_info_t grp{
      .version = 2,
      .participants = {"bob"_n}
   };

   fc::datastream<std::vector<char>> strm;
   fc::raw::pack(strm, grp);

   BOOST_TEST_REQUIRE(chain1.push_action(eosio::chain::action({}, "getgroup"_n, {}, strm.storage()), "getgroup"_n.to_uint64_t()) == "");
   BOOST_TEST_REQUIRE(chain1.push_action(  eosio::chain::action({}, "addmember"_n, {}, participants_payload({"charlie"_n})), "addmember"_n.to_uint64_t() ) == "");
   chain1.produce_blocks(11); 
   BOOST_TEST(chain1.control->proposed_security_group_participants() == participants_t({"bob"_n, "charlie"_n}));
   chain1.control->abort_block();

   /// Test snapshot recovery

   std::stringstream snapshot_strm;
   auto writer = std::make_shared<eosio::chain::ostream_snapshot_writer>(snapshot_strm);
   chain1.control->write_snapshot(writer);
   writer->finalize();

   auto               cfg = chain1.get_config();
   fc::temp_directory tmp_dir;
   cfg.blog.log_dir = tmp_dir.path() / "blocks";
   cfg.state_dir    = tmp_dir.path() / "state";

   auto                   reader = std::make_shared<eosio::chain::istream_snapshot_reader>(snapshot_strm);
   eosio::testing::tester chain2([&cfg, &reader](eosio::testing::tester& self) { self.init(cfg, reader); });
   {
      const auto& active_security_group = chain2.control->active_security_group();
      BOOST_CHECK_EQUAL(2, active_security_group.version);
      BOOST_TEST(active_security_group.participants == participants_t{"bob"_n});
      BOOST_TEST(chain2.control->proposed_security_group_participants() == participants_t({"bob"_n, "charlie"_n}));
   }
}


BOOST_AUTO_TEST_CASE(test_security_group_contract) {
   eosio::testing::tester chain;
   using namespace eosio::chain::literals;

   chain.create_accounts({"secgrptest"_n,"alice"_n,"bob"_n,"charlie"_n});
   chain.produce_block();
   chain.set_code("secgrptest"_n, eosio::testing::contracts::security_group_test_wasm());
   chain.set_abi("secgrptest"_n, eosio::testing::contracts::security_group_test_abi().data());
   chain.produce_block();
   chain.push_action( "eosio"_n, "setpriv"_n, "eosio"_n, fc::mutable_variant_object()("account", "secgrptest"_n)("is_priv", 1));
   chain.produce_block();

   chain.push_action("secgrptest"_n, "add"_n, "secgrptest"_n, fc::mutable_variant_object()
         ( "nm", "alice" )
      );
   chain.push_action("secgrptest"_n, "add"_n, "secgrptest"_n, fc::mutable_variant_object()
         ( "nm", "bob" )
      );
   chain.produce_block();
   BOOST_CHECK(chain.control->in_active_security_group(participants_t({"alice"_n, "bob"_n})));

   chain.push_action("secgrptest"_n, "remove"_n, "secgrptest"_n, fc::mutable_variant_object()
         ( "nm", "alice" )
      );
   chain.produce_block();
   BOOST_CHECK(chain.control->in_active_security_group(participants_t({"bob"_n})));

   {
      auto result =
          chain.push_action("secgrptest"_n, "ingroup"_n, "secgrptest"_n, fc::mutable_variant_object()("nm", "alice"));

      BOOST_CHECK_EQUAL(false, fc::raw::unpack<bool>(result->action_traces[0].return_value));
   }

   {
      auto result = chain.push_action("secgrptest"_n, "activegroup"_n, "secgrptest"_n, fc::mutable_variant_object());
      auto participants = fc::raw::unpack<participants_t>(result->action_traces[0].return_value);
      BOOST_TEST(participants == participants_t({"bob"_n}));
   }
}

BOOST_AUTO_TEST_SUITE_END()
