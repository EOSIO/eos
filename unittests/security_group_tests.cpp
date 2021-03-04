#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/block_state.hpp>
#include <eosio/chain/global_property_object.hpp>


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
      eosio::chain::versioned_unpack_stream unpack_strm(in_strm, eosio::chain::block_header_state::minimum_version_with_state_extension-1);
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
      eosio::chain::versioned_unpack_stream unpack_strm(in_strm, eosio::chain::block_header_state::minimum_version_with_state_extension-1);
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
      eosio::chain::versioned_unpack_stream unpack_strm(in_strm, eosio::chain::block_header_state::minimum_version_with_state_extension);
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

   using participants_t = boost::container::flat_set<eosio::chain::account_name>;

   participants_t new_participants({"alice"_n, "bob"_n});
   chain.control->propose_security_group_participants_add(new_participants);

   BOOST_TEST(chain.control->proposed_security_group_participants() == new_participants);
   BOOST_CHECK_EQUAL(chain.control->active_security_group().participants.size() , 0);

   chain.produce_block();

   BOOST_CHECK_EQUAL(chain.control->proposed_security_group_participants().size() , 0);
   BOOST_TEST(chain.control->active_security_group().participants == new_participants);
   BOOST_CHECK(chain.control->in_active_security_group(participants_t({"alice"_n, "bob"_n})));
   BOOST_CHECK(!chain.control->in_active_security_group(participants_t{"bob"_n, "charlie"_n}));

   chain.control->propose_security_group_participants_remove({"alice"_n});
   BOOST_TEST(chain.control->proposed_security_group_participants() == participants_t{"bob"_n});

   chain.produce_block();

   BOOST_CHECK_EQUAL(chain.control->proposed_security_group_participants().size() , 0);
   BOOST_TEST(chain.control->active_security_group().participants == participants_t{"bob"_n});
   BOOST_CHECK(chain.control->in_active_security_group(participants_t{"bob"_n}));

}

BOOST_AUTO_TEST_SUITE_END()
