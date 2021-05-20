#include "chain_kv_tests.hpp"
#include <boost/filesystem.hpp>

using chain_kv::bytes;
using chain_kv::to_slice;

BOOST_AUTO_TEST_SUITE(write_session_tests)

void write_session_test(bool reload_session) {
   boost::filesystem::remove_all("test-write-session-db");
   chain_kv::database                       db{ "test-write-session-db", true };
   chain_kv::undo_stack                     undo_stack{ db, { 0x10 } };
   std::unique_ptr<chain_kv::write_session> session;

   auto reload = [&] {
      if (session && reload_session) {
         session->write_changes(undo_stack);
         session = nullptr;
      }
      if (!session)
         session = std::make_unique<chain_kv::write_session>(db);
   };
   reload();

   std::vector<bytes> keys = {
      { 0x20 },
      { 0x20, 0x00 },
      { 0x30 },
   };

   BOOST_REQUIRE_EQUAL(get_values(*session, keys), (kv_values{}));
   session->erase({ 0x20 });
   reload();
   BOOST_REQUIRE_EQUAL(get_values(*session, keys), (kv_values{}));
   session->set({ 0x20, 0x00 }, to_slice({ 0x01 }));
   session->set({ 0x30 }, to_slice({ 0x02 }));
   reload();
   session->set({ 0x20 }, to_slice({ 0x03 }));
   reload();
   BOOST_REQUIRE_EQUAL(get_values(*session, keys), (kv_values{ {
                                                         { { 0x20 }, { 0x03 } },
                                                         { { 0x20, 0x00 }, { 0x01 } },
                                                         { { 0x30 }, { 0x02 } },
                                                   } }));
   session->erase({ 0x20 });
   reload();
   BOOST_REQUIRE_EQUAL(get_values(*session, keys), (kv_values{ {
                                                         { { 0x20, 0x00 }, { 0x01 } },
                                                         { { 0x30 }, { 0x02 } },
                                                   } }));
   session->set({ 0x20, 0x00 }, to_slice({ 0x11 }));
   reload();
   BOOST_REQUIRE_EQUAL(get_values(*session, keys), (kv_values{ {
                                                         { { 0x20, 0x00 }, { 0x11 } },
                                                         { { 0x30 }, { 0x02 } },
                                                   } }));
   session->set({ 0x20 }, to_slice({ 0x23 }));
   reload();
   BOOST_REQUIRE_EQUAL(get_values(*session, keys), (kv_values{ {
                                                         { { 0x20 }, { 0x23 } },
                                                         { { 0x20, 0x00 }, { 0x11 } },
                                                         { { 0x30 }, { 0x02 } },
                                                   } }));
} // write_session_test()

BOOST_AUTO_TEST_CASE(test_write_session) {
   write_session_test(false);
   write_session_test(true);
}

BOOST_AUTO_TEST_SUITE_END();
