#include "../log_store.hpp"
#include <boost/iterator/transform_iterator.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <numeric>

namespace fs = std::filesystem;

class new_temp_directory_path : public fs::path {
 public:
   new_temp_directory_path()
       : std::filesystem::path(std::tmpnam(nullptr)) {
      fs::create_directory(*this);
   }
   ~new_temp_directory_path() { fs::remove_all(*this); }

   new_temp_directory_path(const new_temp_directory_path&) = delete;
   new_temp_directory_path& operator=(const new_temp_directory_path&) = delete;
};

nuraft::ptr<nuraft::log_entry> make_log_entry(ulong term, nuraft::log_val_type type) {
   auto buf = nuraft::buffer::alloc(sizeof(int32_t) + std::rand() % 32);
   auto r   = nuraft::cs_new<nuraft::log_entry>(term, buf, type);
   buf->put(std::rand());
   return r;
}

namespace nuraft {

std::ostream& operator<<(std::ostream& os, const log_entry& lhs) {
   os << "{ term: " << lhs.get_term() << ", type: " << lhs.get_val_type() << ", buf: ";
   std::ios_base::fmtflags f(os.flags());
   os << std::hex << std::setfill('0') << std::setw(2);
   const auto& buf = lhs.get_buf();
   std::copy(buf.data_begin(), buf.data_begin() + buf.size(), std::ostream_iterator<int>(os));
   os.flags(f);
   os << " }";
   return os;
}

bool operator==(const log_entry& lhs, const log_entry& rhs) {
   return lhs.get_term() == rhs.get_term() && lhs.get_val_type() == rhs.get_val_type() &&
          lhs.get_buf().size() == rhs.get_buf().size() &&
          memcmp(lhs.get_buf().data_begin(), rhs.get_buf().data_begin(), lhs.get_buf().size()) == 0;
}

} // namespace nuraft

BOOST_AUTO_TEST_CASE(test_log_store) {
   new_temp_directory_path tmp_dir;
   blockvault::log_store   test_store(tmp_dir, 20);

   BOOST_CHECK_EQUAL(test_store.start_index(), 0);
   BOOST_CHECK_EQUAL(test_store.next_slot(), 1);
   BOOST_CHECK_EQUAL(test_store.last_entry()->get_term(), 0);

   // test append()
   ulong                                       term = 0;
   int                                         type = 0;
   std::vector<nuraft::ptr<nuraft::log_entry>> entries;
   entries.emplace_back();
   for (int i = 1; i < 150; ++i) {
      // change term and type so that they won't always be the same
      if (i % 2)
         ++term;
      else {
         type = (++type) % 6;
      }

      auto entry = make_log_entry(term, static_cast<nuraft::log_val_type>(type));

      entries.push_back(entry);
      test_store.append(entry);

      BOOST_CHECK_EQUAL(test_store.start_index(), 0);
      BOOST_CHECK_EQUAL(test_store.next_slot(), i + 1);

      auto ret_entry = test_store.last_entry();

      BOOST_REQUIRE_EQUAL(*entry, *ret_entry);
   }

   // test entry_at()
   for (int i = 1; i < 150; ++i) {
      BOOST_CHECK_EQUAL(*entries[i], *test_store.entry_at(i));
   }

   // test term_at
   for (int i = 1; i < 150; ++i) {
      BOOST_CHECK_EQUAL(entries[i]->get_term(), test_store.term_at(i));
   }

   // test log_entries() in the same segment
   auto r0 = test_store.log_entries(60, 70);
   BOOST_CHECK_EQUAL(70 - 60, r0->size());
   for (int idx = 60; idx < 70; ++idx) {
      BOOST_CHECK_EQUAL(*entries[idx], *(r0->at(idx - 60)));
   }

   // test log_entries() across segments
   auto r00 = test_store.log_entries(90, 110);
   BOOST_CHECK_EQUAL(110 - 90, r00->size());
   for (int idx = 90; idx < 110; ++idx) {
      BOOST_CHECK_EQUAL(*entries[idx], *(r00->at(idx - 90)));
   }

   // test log_entries_ext()
   std::vector<uint64_t> total_sizes;
   total_sizes.resize(150 + 1);
   auto entry_buf_size = [](const nuraft::ptr<nuraft::log_entry>& e) { return e->get_buf().size(); };
   std::partial_sum(boost::make_transform_iterator(entries.begin() + 1, entry_buf_size),
                    boost::make_transform_iterator(entries.end(), entry_buf_size), total_sizes.begin() + 2);

   auto r1 = test_store.log_entries_ext(90, 110, total_sizes[95] - total_sizes[90]);
   BOOST_CHECK_EQUAL(r1->size(), 5);

   auto r11 = test_store.log_entries_ext(90, 110, total_sizes[105] - total_sizes[90]);
   BOOST_CHECK_EQUAL(r11->size(), 15);

   auto r2 = test_store.log_entries_ext(90, 110, -1);
   BOOST_CHECK_EQUAL(r2->size(), 0);

   auto r3 = test_store.log_entries_ext(90, 110, total_sizes[98] - total_sizes[90] + 1);
   BOOST_CHECK_EQUAL(r3->size(), 9);

   // test pack() and apply_pack()

   auto pack = test_store.pack(110, 40);

   new_temp_directory_path tmp_dir2;
   {
      blockvault::log_store test_store2(tmp_dir2, 10);

      test_store2.apply_pack(110, *pack);
      BOOST_CHECK_EQUAL(test_store2.start_index(), 110);
      BOOST_CHECK_EQUAL(test_store2.next_slot(), 150);

      for (int i = 110; i < 150; ++i) {
         BOOST_CHECK_EQUAL(*entries[i], *test_store2.entry_at(i));
      }

      pack = test_store.pack(130, 10);
      test_store2.apply_pack(130, *pack);

      BOOST_CHECK_EQUAL(test_store2.start_index(), 110);
      BOOST_CHECK_EQUAL(test_store2.next_slot(), 140);

      for (int i = 110; i < 140; ++i) {
         BOOST_CHECK_EQUAL(*entries[i], *test_store2.entry_at(i));
      }
   }

   // test restart
   {
      blockvault::log_store test_store2(tmp_dir2, 10);
      BOOST_CHECK_EQUAL(test_store2.start_index(), 110);
      BOOST_CHECK_EQUAL(test_store2.next_slot(), 140);
   }

   // test write_at()
   auto entry = make_log_entry(term, static_cast<nuraft::log_val_type>(type));
   test_store.write_at(135, entry);
   BOOST_CHECK_EQUAL(test_store.next_slot(), 136);
   for (int i = 130; i <= 134; ++i) {
      BOOST_CHECK_EQUAL(*entries[i], *test_store.entry_at(i));
   }
   BOOST_CHECK_EQUAL(*entry, *test_store.entry_at(135));

   // test for_each_app_entry

   std::set<ulong> app_entry_indices;

   test_store.for_each_app_entry(90, 120, [&entries, &app_entry_indices](ulong index, std::string_view payload) {
      const auto& entry = entries[index];
      BOOST_CHECK_EQUAL(entry->get_val_type(), nuraft::app_log);
      BOOST_CHECK_EQUAL(entry->get_buf().size(), payload.size());
      BOOST_CHECK_EQUAL(memcmp(entry->get_buf().data_begin(), payload.data(), payload.size()), 0);
      app_entry_indices.emplace(index);
   });

   for (int i = 90; i < 120; ++i) {
      // make sure all app_log entries are received from for_each_app_entry
      const auto& entry = entries[i];
      if (entry->get_val_type() == nuraft::app_log) {
         BOOST_CHECK_EQUAL(app_entry_indices.count(i), 1);
      }
   }

   // test compact() with index not in the segment boundary

   test_store.compact(90);
   BOOST_CHECK_EQUAL(test_store.start_index(), 80);
   BOOST_CHECK_EQUAL(test_store.next_slot(), 136);

   test_store.compact(99);
   BOOST_CHECK_EQUAL(test_store.start_index(), 100);
   BOOST_CHECK_EQUAL(test_store.next_slot(), 136);

   // test for_each_app_entry when the start_index is out of range
   app_entry_indices.clear();

   test_store.for_each_app_entry(90, 120, [&entries, &app_entry_indices](ulong index, std::string_view payload) {
      const auto& entry = entries[index];
      BOOST_CHECK_EQUAL(entry->get_val_type(), nuraft::app_log);
      BOOST_CHECK_EQUAL(entry->get_buf().size(), payload.size());
      BOOST_CHECK_EQUAL(memcmp(entry->get_buf().data_begin(), payload.data(), payload.size()), 0);
      app_entry_indices.emplace(index);
   });

   for (int i = 100; i < 120; ++i) {
      // make sure all app_log entries are received from for_each_app_entry
      const auto& entry = entries[i];
      if (entry->get_val_type() == nuraft::app_log) {
         BOOST_CHECK_EQUAL(app_entry_indices.count(i), 1);
      }
   }

   // test compact() with index in the segment boundary

   test_store.compact(120);
   BOOST_CHECK_EQUAL(test_store.start_index(), 120);
   BOOST_CHECK_EQUAL(test_store.next_slot(), 136);

}
