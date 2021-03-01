#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/block_state.hpp>


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

}}

FC_REFLECT_DERIVED(  eosio::chain::block_header_state_v0, (eosio::chain::detail::block_header_state_common),
                     (id)
                     (header)
                     (pending_schedule)
                     (activated_protocol_features)
                     (additional_signatures)
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

BOOST_AUTO_TEST_SUITE_END()
