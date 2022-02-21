#include "chain_kv_tests.hpp"
#include <boost/filesystem.hpp>

using chain_kv::bytes;
using chain_kv::to_slice;

BOOST_AUTO_TEST_SUITE(view_tests)

void view_test(bool reload_session) {
   boost::filesystem::remove_all("view-test-session-db");
   chain_kv::database                       db{ "view-test-session-db", true };
   chain_kv::undo_stack                     undo_stack{ db, { 0x10 } };
   std::unique_ptr<chain_kv::write_session> session;
   std::unique_ptr<chain_kv::view>          view;

   auto reload = [&] {
      if (session && reload_session) {
         session->write_changes(undo_stack);
         view    = nullptr;
         session = nullptr;
      }
      if (!session)
         session = std::make_unique<chain_kv::write_session>(db);
      if (!view)
         view = std::make_unique<chain_kv::view>(*session, bytes{ 0x70 });
   };
   reload();

   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x1234), (kv_values{}));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x1234), get_matching2(*view, 0x1234));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x5678), (kv_values{}));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x5678), get_matching2(*view, 0x5678));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x9abc), (kv_values{}));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x9abc), get_matching2(*view, 0x9abc));

   view->set(0x1234, to_slice({ 0x30, 0x40 }), to_slice({ 0x50, 0x60 }));
   view->set(0x5678, to_slice({ 0x30, 0x71 }), to_slice({ 0x59, 0x69 }));
   view->set(0x5678, to_slice({ 0x30, 0x00 }), to_slice({ 0x59, 0x69 }));
   view->set(0x5678, to_slice({ 0x30, 0x42 }), to_slice({ 0x55, 0x66 }));
   view->set(0x5678, to_slice({ 0x30, 0x41 }), to_slice({ 0x51, 0x61 }));
   view->set(0x9abc, to_slice({ 0x30, 0x42 }), to_slice({ 0x52, 0x62 }));
   reload();

   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x1234), (kv_values{ {
                                                          { { 0x30, 0x40 }, { 0x50, 0x60 } },
                                                    } }));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x1234), get_matching2(*view, 0x1234));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x5678), (kv_values{ {
                                                          { { 0x30, 0x00 }, { 0x59, 0x69 } },
                                                          { { 0x30, 0x41 }, { 0x51, 0x61 } },
                                                          { { 0x30, 0x42 }, { 0x55, 0x66 } },
                                                          { { 0x30, 0x71 }, { 0x59, 0x69 } },
                                                    } }));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x5678), get_matching2(*view, 0x5678));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x9abc), (kv_values{ {
                                                          { { 0x30, 0x42 }, { 0x52, 0x62 } },
                                                    } }));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x9abc), get_matching2(*view, 0x9abc));

   view->erase(0x5678, to_slice({ 0x30, 0x00 }));
   view->erase(0x5678, to_slice({ 0x30, 0x71 }));
   view->erase(0x5678, to_slice({ 0x30, 0x42 }));
   reload();

   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x1234), (kv_values{ {
                                                          { { 0x30, 0x40 }, { 0x50, 0x60 } },
                                                    } }));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x1234), get_matching2(*view, 0x1234));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x5678), (kv_values{ {
                                                          { { 0x30, 0x41 }, { 0x51, 0x61 } },
                                                    } }));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x5678), get_matching2(*view, 0x5678));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x9abc), (kv_values{ {
                                                          { { 0x30, 0x42 }, { 0x52, 0x62 } },
                                                    } }));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0x9abc), get_matching2(*view, 0x9abc));

   {
      chain_kv::view::iterator it{ *view, 0x5678, {} };
      it.lower_bound({ 0x30, 0x22 });
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { 0x30, 0x41 }, { 0x51, 0x61 } },
                                      } }));
      view->set(0x5678, to_slice({ 0x30, 0x22 }), to_slice({ 0x55, 0x66 }));
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { 0x30, 0x41 }, { 0x51, 0x61 } },
                                      } }));
      it.lower_bound({ 0x30, 0x22 });
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { 0x30, 0x22 }, { 0x55, 0x66 } },
                                      } }));
      view->set(0x5678, to_slice({ 0x30, 0x22 }), to_slice({ 0x00, 0x11 }));
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { 0x30, 0x22 }, { 0x00, 0x11 } },
                                      } }));
      view->erase(0x5678, to_slice({ 0x30, 0x22 }));
      KV_REQUIRE_EXCEPTION(it.get_kv(), "kv iterator is at an erased value");
      KV_REQUIRE_EXCEPTION(--it, "kv iterator is at an erased value");
      KV_REQUIRE_EXCEPTION(++it, "kv iterator is at an erased value");
      view->set(0x5678, to_slice({ 0x30, 0x22 }), to_slice({}));
      KV_REQUIRE_EXCEPTION(it.get_kv(), "kv iterator is at an erased value");
      it.lower_bound({ 0x30, 0x22 });
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { 0x30, 0x22 }, {} },
                                      } }));
      view->set(0x5678, to_slice({ 0x30, 0x22 }), to_slice({ 0x00 }));
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { 0x30, 0x22 }, { 0x00 } },
                                      } }));
      view->erase(0x5678, to_slice({ 0x30, 0x22 }));
      KV_REQUIRE_EXCEPTION(it.get_kv(), "kv iterator is at an erased value");
   }
   reload();

   {
      chain_kv::view::iterator it{ *view, 0xbeefbeef, {} };
      view->set(0xbeefbeef, to_slice({ (char)0x80 }), to_slice({ (char)0xff }));
      view->set(0xbeefbeef, to_slice({ (char)0x90 }), to_slice({ (char)0xfe }));
      view->set(0xbeefbeef, to_slice({ (char)0xa0 }), to_slice({ (char)0xfd }));
      view->set(0xbeefbeef, to_slice({ (char)0xb0 }), to_slice({ (char)0xfc }));
      it.lower_bound({});
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { (char)0x80 }, { (char)0xff } },
                                      } }));
      it.lower_bound({ (char)0x80 });
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { (char)0x80 }, { (char)0xff } },
                                      } }));
      it.lower_bound({ (char)0x81 });
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { (char)0x90 }, { (char)0xfe } },
                                      } }));
      it.lower_bound({ (char)0x90 });
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { (char)0x90 }, { (char)0xfe } },
                                      } }));
      --it;
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { (char)0x80 }, { (char)0xff } },
                                      } }));
      ++it;
      ++it;
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { (char)0xa0 }, { (char)0xfd } },
                                      } }));
      view->erase(0xbeefbeef, to_slice({ (char)0x90 }));
      --it;
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { (char)0x80 }, { (char)0xff } },
                                      } }));
      view->erase(0xbeefbeef, to_slice({ (char)0xa0 }));
      ++it;
      BOOST_REQUIRE_EQUAL(get_it(it), (kv_values{ {
                                            { { (char)0xb0 }, { (char)0xfc } },
                                      } }));
   }
   reload();

   BOOST_REQUIRE_EQUAL(get_matching(*view, 0xbeefbeef), (kv_values{ {
                                                              { { (char)0x80 }, { (char)0xff } },
                                                              { { (char)0xb0 }, { (char)0xfc } },
                                                        } }));

   view->set(0xf00df00d, to_slice({ 0x10, 0x20, 0x00 }), to_slice({ (char)0x70 }));
   view->set(0xf00df00d, to_slice({ 0x10, 0x20, 0x01 }), to_slice({ (char)0x71 }));
   view->set(0xf00df00d, to_slice({ 0x10, 0x20, 0x02 }), to_slice({ (char)0x72 }));

   view->set(0xf00df00d, to_slice({ 0x10, 0x30, 0x00 }), to_slice({ (char)0x70 }));
   view->set(0xf00df00d, to_slice({ 0x10, 0x30, 0x01 }), to_slice({ (char)0x71 }));
   view->set(0xf00df00d, to_slice({ 0x10, 0x30, 0x02 }), to_slice({ (char)0x72 }));

   view->set(0xf00df00d, to_slice({ 0x20, 0x00 }), to_slice({ (char)0x70 }));
   view->set(0xf00df00d, to_slice({ 0x20, 0x01 }), to_slice({ (char)0x71 }));
   view->set(0xf00df00d, to_slice({ 0x20, 0x02 }), to_slice({ (char)0x72 }));
   reload();

   BOOST_REQUIRE_EQUAL(get_matching(*view, 0xf00df00d, { 0x10, 0x20 }), (kv_values{ {
                                                                              { { 0x10, 0x20, 0x00 }, { 0x70 } },
                                                                              { { 0x10, 0x20, 0x01 }, { 0x71 } },
                                                                              { { 0x10, 0x20, 0x02 }, { 0x72 } },
                                                                        } }));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0xf00df00d, { 0x10, 0x20 }),
                       get_matching2(*view, 0xf00df00d, { 0x10, 0x20 }));

   BOOST_REQUIRE_EQUAL(get_matching(*view, 0xf00df00d, { 0x10, 0x30 }), (kv_values{ {
                                                                              { { 0x10, 0x30, 0x00 }, { 0x70 } },
                                                                              { { 0x10, 0x30, 0x01 }, { 0x71 } },
                                                                              { { 0x10, 0x30, 0x02 }, { 0x72 } },
                                                                        } }));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0xf00df00d, { 0x10, 0x30 }),
                       get_matching2(*view, 0xf00df00d, { 0x10, 0x30 }));

   BOOST_REQUIRE_EQUAL(get_matching(*view, 0xf00df00d, { 0x10 }), (kv_values{ {
                                                                        { { 0x10, 0x20, 0x00 }, { 0x70 } },
                                                                        { { 0x10, 0x20, 0x01 }, { 0x71 } },
                                                                        { { 0x10, 0x20, 0x02 }, { 0x72 } },
                                                                        { { 0x10, 0x30, 0x00 }, { 0x70 } },
                                                                        { { 0x10, 0x30, 0x01 }, { 0x71 } },
                                                                        { { 0x10, 0x30, 0x02 }, { 0x72 } },
                                                                  } }));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0xf00df00d, { 0x10 }), get_matching2(*view, 0xf00df00d, { 0x10 }));

   BOOST_REQUIRE_EQUAL(get_matching(*view, 0xf00df00d, { 0x20 }), (kv_values{ {
                                                                        { { 0x20, 0x00 }, { 0x70 } },
                                                                        { { 0x20, 0x01 }, { 0x71 } },
                                                                        { { 0x20, 0x02 }, { 0x72 } },
                                                                  } }));
   BOOST_REQUIRE_EQUAL(get_matching(*view, 0xf00df00d, { 0x20 }), get_matching2(*view, 0xf00df00d, { 0x20 }));
} // view_test()

BOOST_AUTO_TEST_CASE(test_view) {
   view_test(false);
   view_test(true);
}

BOOST_AUTO_TEST_SUITE_END();
