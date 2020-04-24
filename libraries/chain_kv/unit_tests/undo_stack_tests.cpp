#include "chain_kv_tests.hpp"
#include <boost/filesystem.hpp>

using chain_kv::bytes;
using chain_kv::to_slice;

BOOST_AUTO_TEST_SUITE(undo_stack_tests)

void undo_tests(bool reload_undo, uint64_t target_segment_size) {
   boost::filesystem::remove_all("test-undo-db");
   chain_kv::database                    db{ "test-undo-db", true };
   std::unique_ptr<chain_kv::undo_stack> undo_stack;

   auto reload = [&] {
      if (!undo_stack || reload_undo)
         undo_stack = std::make_unique<chain_kv::undo_stack>(db, bytes{ 0x10 }, target_segment_size);
   };
   reload();

   KV_REQUIRE_EXCEPTION(undo_stack->undo(), "nothing to undo");
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 0);
   {
      chain_kv::write_session session{ db };
      session.set({ 0x20, 0x00 }, to_slice({}));
      session.set({ 0x20, 0x02 }, to_slice({ 0x50 }));
      session.set({ 0x20, 0x01 }, to_slice({ 0x40 }));
      session.erase({ 0x20, 0x02 });
      session.set({ 0x20, 0x03 }, to_slice({ 0x60 }));
      session.set({ 0x20, 0x01 }, to_slice({ 0x50 }));
      session.write_changes(*undo_stack);
   }
   KV_REQUIRE_EXCEPTION(undo_stack->undo(), "nothing to undo");
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 0);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x10, (char)0x80 }), (kv_values{})); // no undo segments
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x00 }, {} },
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x03 }, { 0x60 } },
                                              } }));

   reload();
   undo_stack->push();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 1);
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 1);
   {
      chain_kv::write_session session{ db };
      session.erase({ 0x20, 0x01 });
      session.set({ 0x20, 0x00 }, to_slice({ 0x70 }));
      session.write_changes(*undo_stack);
   }
   BOOST_REQUIRE_NE(get_all(db, { 0x10, (char)0x80 }), (kv_values{})); // has undo segments
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x00 }, { 0x70 } },
                                                    { { 0x20, 0x03 }, { 0x60 } },
                                              } }));

   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 1);
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 1);
   KV_REQUIRE_EXCEPTION(undo_stack->set_revision(2), "cannot set revision while there is an existing undo stack");
   undo_stack->undo();
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x10, (char)0x80 }), (kv_values{})); // no undo segments
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 0);
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 0);
   undo_stack->set_revision(10);
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 10);
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 10);

   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x00 }, {} },
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x03 }, { 0x60 } },
                                              } }));

   {
      chain_kv::write_session session{ db };
      session.erase({ 0x20, 0x01 });
      session.set({ 0x20, 0x00 }, to_slice({ 0x70 }));
      session.write_changes(*undo_stack);
   }
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x10, (char)0x80 }), (kv_values{})); // no undo segments
   reload();
   undo_stack->push();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 11);
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 11);
   KV_REQUIRE_EXCEPTION(undo_stack->set_revision(12), "cannot set revision while there is an existing undo stack");
   undo_stack->commit(0);
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 11);
   KV_REQUIRE_EXCEPTION(undo_stack->set_revision(12), "cannot set revision while there is an existing undo stack");
   undo_stack->commit(11);
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 11);
   reload();
   KV_REQUIRE_EXCEPTION(undo_stack->set_revision(9), "revision cannot decrease");
   undo_stack->set_revision(12);
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 12);
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 12);

   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x00 }, { 0x70 } },
                                                    { { 0x20, 0x03 }, { 0x60 } },
                                              } }));
} // undo_tests()

void squash_tests(bool reload_undo, uint64_t target_segment_size) {
   boost::filesystem::remove_all("test-squash-db");
   chain_kv::database                    db{ "test-squash-db", true };
   std::unique_ptr<chain_kv::undo_stack> undo_stack;

   auto reload = [&] {
      if (!undo_stack || reload_undo)
         undo_stack = std::make_unique<chain_kv::undo_stack>(db, bytes{ 0x10 }, target_segment_size);
   };
   reload();

   // set 1
   undo_stack->push();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 1);
   {
      chain_kv::write_session session{ db };
      session.set({ 0x20, 0x01 }, to_slice({ 0x50 }));
      session.set({ 0x20, 0x02 }, to_slice({ 0x60 }));
      session.write_changes(*undo_stack);
   }
   reload();
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x60 } },
                                              } }));

   // set 2
   undo_stack->push();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 2);
   {
      chain_kv::write_session session{ db };
      session.erase({ 0x20, 0x01 });
      session.set({ 0x20, 0x02 }, to_slice({ 0x61 }));
      session.set({ 0x20, 0x03 }, to_slice({ 0x70 }));
      session.set({ 0x20, 0x04 }, to_slice({ 0x10 }));
      session.write_changes(*undo_stack);
   }
   reload();
   undo_stack->push();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 3);
   {
      chain_kv::write_session session{ db };
      session.set({ 0x20, 0x01 }, to_slice({ 0x50 }));
      session.set({ 0x20, 0x02 }, to_slice({ 0x62 }));
      session.erase({ 0x20, 0x03 });
      session.set({ 0x20, 0x05 }, to_slice({ 0x05 }));
      session.set({ 0x20, 0x06 }, to_slice({ 0x06 }));
      session.write_changes(*undo_stack);
   }
   reload();
   undo_stack->squash();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 2);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x62 } },
                                                    { { 0x20, 0x04 }, { 0x10 } },
                                                    { { 0x20, 0x05 }, { 0x05 } },
                                                    { { 0x20, 0x06 }, { 0x06 } },
                                              } }));

   // set 3
   undo_stack->push();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 3);
   {
      chain_kv::write_session session{ db };
      session.set({ 0x20, 0x07 }, to_slice({ 0x07 }));
      session.set({ 0x20, 0x08 }, to_slice({ 0x08 }));
      session.write_changes(*undo_stack);
   }
   reload();
   undo_stack->push();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 4);
   {
      chain_kv::write_session session{ db };
      session.set({ 0x20, 0x09 }, to_slice({ 0x09 }));
      session.set({ 0x20, 0x0a }, to_slice({ 0x0a }));
      session.write_changes(*undo_stack);
   }
   reload();
   undo_stack->push();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 5);
   {
      chain_kv::write_session session{ db };
      session.set({ 0x20, 0x0b }, to_slice({ 0x0b }));
      session.set({ 0x20, 0x0c }, to_slice({ 0x0c }));
      session.write_changes(*undo_stack);
   }
   reload();
   undo_stack->squash();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 4);
   undo_stack->squash();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 3);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x62 } },
                                                    { { 0x20, 0x04 }, { 0x10 } },
                                                    { { 0x20, 0x05 }, { 0x05 } },
                                                    { { 0x20, 0x06 }, { 0x06 } },
                                                    { { 0x20, 0x07 }, { 0x07 } },
                                                    { { 0x20, 0x08 }, { 0x08 } },
                                                    { { 0x20, 0x09 }, { 0x09 } },
                                                    { { 0x20, 0x0a }, { 0x0a } },
                                                    { { 0x20, 0x0b }, { 0x0b } },
                                                    { { 0x20, 0x0c }, { 0x0c } },
                                              } }));

   // undo set 3
   undo_stack->undo();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 2);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x62 } },
                                                    { { 0x20, 0x04 }, { 0x10 } },
                                                    { { 0x20, 0x05 }, { 0x05 } },
                                                    { { 0x20, 0x06 }, { 0x06 } },
                                              } }));

   // undo set 2
   undo_stack->undo();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 1);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x60 } },
                                              } }));

   // undo set 1
   undo_stack->undo();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 0);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {} }));

   // TODO: test squash with only 1 undo level
} // squash_tests()

void commit_tests(bool reload_undo, uint64_t target_segment_size) {
   boost::filesystem::remove_all("test-commit-db");
   chain_kv::database                    db{ "test-commit-db", true };
   std::unique_ptr<chain_kv::undo_stack> undo_stack;

   auto reload = [&] {
      if (!undo_stack || reload_undo)
         undo_stack = std::make_unique<chain_kv::undo_stack>(db, bytes{ 0x10 }, target_segment_size);
   };
   reload();

   // revision 1
   undo_stack->push();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 1);
   {
      chain_kv::write_session session{ db };
      session.set({ 0x20, 0x01 }, to_slice({ 0x50 }));
      session.set({ 0x20, 0x02 }, to_slice({ 0x60 }));
      session.write_changes(*undo_stack);
   }
   reload();
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x60 } },
                                              } }));

   // revision 2
   undo_stack->push();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 2);
   {
      chain_kv::write_session session{ db };
      session.erase({ 0x20, 0x01 });
      session.set({ 0x20, 0x02 }, to_slice({ 0x61 }));
      session.set({ 0x20, 0x03 }, to_slice({ 0x70 }));
      session.set({ 0x20, 0x04 }, to_slice({ 0x10 }));
      session.set({ 0x20, 0x01 }, to_slice({ 0x50 }));
      session.set({ 0x20, 0x02 }, to_slice({ 0x62 }));
      session.erase({ 0x20, 0x03 });
      session.set({ 0x20, 0x05 }, to_slice({ 0x05 }));
      session.set({ 0x20, 0x06 }, to_slice({ 0x06 }));
      session.write_changes(*undo_stack);
   }
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 2);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x62 } },
                                                    { { 0x20, 0x04 }, { 0x10 } },
                                                    { { 0x20, 0x05 }, { 0x05 } },
                                                    { { 0x20, 0x06 }, { 0x06 } },
                                              } }));

   // revision 3
   undo_stack->push();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 3);
   {
      chain_kv::write_session session{ db };
      session.set({ 0x20, 0x07 }, to_slice({ 0x07 }));
      session.set({ 0x20, 0x08 }, to_slice({ 0x08 }));
      session.set({ 0x20, 0x09 }, to_slice({ 0x09 }));
      session.set({ 0x20, 0x0a }, to_slice({ 0x0a }));
      session.set({ 0x20, 0x0b }, to_slice({ 0x0b }));
      session.set({ 0x20, 0x0c }, to_slice({ 0x0c }));
      session.write_changes(*undo_stack);
   }
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 3);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x62 } },
                                                    { { 0x20, 0x04 }, { 0x10 } },
                                                    { { 0x20, 0x05 }, { 0x05 } },
                                                    { { 0x20, 0x06 }, { 0x06 } },
                                                    { { 0x20, 0x07 }, { 0x07 } },
                                                    { { 0x20, 0x08 }, { 0x08 } },
                                                    { { 0x20, 0x09 }, { 0x09 } },
                                                    { { 0x20, 0x0a }, { 0x0a } },
                                                    { { 0x20, 0x0b }, { 0x0b } },
                                                    { { 0x20, 0x0c }, { 0x0c } },
                                              } }));

   // commit revision 1
   undo_stack->commit(1);

   // undo revision 3
   undo_stack->undo();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 2);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x62 } },
                                                    { { 0x20, 0x04 }, { 0x10 } },
                                                    { { 0x20, 0x05 }, { 0x05 } },
                                                    { { 0x20, 0x06 }, { 0x06 } },
                                              } }));

   // undo revision 2
   undo_stack->undo();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 1);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x60 } },
                                              } }));

   // Can't undo revision 1
   KV_REQUIRE_EXCEPTION(undo_stack->undo(), "nothing to undo");

   BOOST_REQUIRE_EQUAL(get_all(db, { 0x10, (char)0x80 }), (kv_values{})); // no undo segments

   // revision 2
   undo_stack->push();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 2);
   {
      chain_kv::write_session session{ db };
      session.erase({ 0x20, 0x01 });
      session.set({ 0x20, 0x02 }, to_slice({ 0x61 }));
      session.set({ 0x20, 0x03 }, to_slice({ 0x70 }));
      session.set({ 0x20, 0x04 }, to_slice({ 0x10 }));
      session.set({ 0x20, 0x01 }, to_slice({ 0x50 }));
      session.set({ 0x20, 0x02 }, to_slice({ 0x62 }));
      session.erase({ 0x20, 0x03 });
      session.set({ 0x20, 0x05 }, to_slice({ 0x05 }));
      session.set({ 0x20, 0x06 }, to_slice({ 0x06 }));
      session.write_changes(*undo_stack);
   }
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 2);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x62 } },
                                                    { { 0x20, 0x04 }, { 0x10 } },
                                                    { { 0x20, 0x05 }, { 0x05 } },
                                                    { { 0x20, 0x06 }, { 0x06 } },
                                              } }));

   // revision 3
   undo_stack->push();
   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 3);
   {
      chain_kv::write_session session{ db };
      session.set({ 0x20, 0x07 }, to_slice({ 0x07 }));
      session.set({ 0x20, 0x08 }, to_slice({ 0x08 }));
      session.set({ 0x20, 0x09 }, to_slice({ 0x09 }));
      session.set({ 0x20, 0x0a }, to_slice({ 0x0a }));
      session.set({ 0x20, 0x0b }, to_slice({ 0x0b }));
      session.set({ 0x20, 0x0c }, to_slice({ 0x0c }));
      session.write_changes(*undo_stack);
   }
   reload();

   // commit revision 3
   undo_stack->commit(3);

   // Can't undo
   KV_REQUIRE_EXCEPTION(undo_stack->undo(), "nothing to undo");

   BOOST_REQUIRE_EQUAL(get_all(db, { 0x10, (char)0x80 }), (kv_values{})); // no undo segments

   reload();
   BOOST_REQUIRE_EQUAL(undo_stack->revision(), 3);
   BOOST_REQUIRE_EQUAL(get_all(db, { 0x20 }), (kv_values{ {
                                                    { { 0x20, 0x01 }, { 0x50 } },
                                                    { { 0x20, 0x02 }, { 0x62 } },
                                                    { { 0x20, 0x04 }, { 0x10 } },
                                                    { { 0x20, 0x05 }, { 0x05 } },
                                                    { { 0x20, 0x06 }, { 0x06 } },
                                                    { { 0x20, 0x07 }, { 0x07 } },
                                                    { { 0x20, 0x08 }, { 0x08 } },
                                                    { { 0x20, 0x09 }, { 0x09 } },
                                                    { { 0x20, 0x0a }, { 0x0a } },
                                                    { { 0x20, 0x0b }, { 0x0b } },
                                                    { { 0x20, 0x0c }, { 0x0c } },
                                              } }));
} // commit_tests()

BOOST_AUTO_TEST_CASE(test_undo) {
   undo_tests(false, 0);
   undo_tests(true, 0);
   undo_tests(false, 64 * 1024 * 1024);
   undo_tests(true, 64 * 1024 * 1024);
}

BOOST_AUTO_TEST_CASE(test_squash) {
   squash_tests(false, 0);
   squash_tests(true, 0);
   squash_tests(false, 64 * 1024 * 1024);
   squash_tests(true, 64 * 1024 * 1024);
}

BOOST_AUTO_TEST_CASE(test_commit) {
   commit_tests(false, 0);
   commit_tests(true, 0);
   commit_tests(false, 64 * 1024 * 1024);
   commit_tests(true, 64 * 1024 * 1024);
}

BOOST_AUTO_TEST_SUITE_END();
