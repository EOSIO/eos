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


BOOST_AUTO_TEST_SUITE(block_state_tests)

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
   auto& sgi = bs->get_security_group_info();
   sgi.version = 1;
   sgi.participants.insert("adam"_n);

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
      BOOST_CHECK_EQUAL(bs->get_security_group_info().version, 1);
      BOOST_TEST(bs->get_security_group_info().participants == sgi.participants);
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
      gpo.pending_security_group = eosio::chain::pending_security_group_t {
         .block_num = 10,
         .version = 1,
         .participants = { "adam"_n }
      };
      
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
         auto pending_security_group = std::visit([](auto& ext) { return ext.pending_security_group; }, row.extension);
         BOOST_CHECK(pending_security_group.has_value());
         if (pending_security_group.has_value()) {
            BOOST_CHECK_EQUAL(pending_security_group->block_num, gpo.pending_security_group->block_num);
            BOOST_CHECK_EQUAL(pending_security_group->version, gpo.pending_security_group->version);
            BOOST_TEST(pending_security_group->participants ==  gpo.pending_security_group->participants);
         }
      }
   }
}

BOOST_AUTO_TEST_SUITE_END()
