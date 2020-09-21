#include "../log_store.hpp"
#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <numeric>
#include <boost/iterator/transform_iterator.hpp>

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
   auto r = nuraft::cs_new<nuraft::log_entry>(term, buf, type);
   buf->put(std::rand());
   return r;
}

namespace nuraft {

std::ostream& operator<<(std::ostream& os, const log_entry& lhs) {
   os << "{ term: " << lhs.get_term() << ", type: " << lhs.get_val_type() << ", buf: ";
   std::ios_base::fmtflags f( os.flags() );
   os << std::hex << std::setfill('0') << std::setw(2);
   const auto& buf = lhs.get_buf();
   std::copy(buf.data_begin(), buf.data_begin() + buf.size(), std::ostream_iterator<int>(os));
   os.flags( f );
   os << " }";
   return os;
}

bool operator == (const log_entry& lhs, const log_entry& rhs) {
   return lhs.get_term() == rhs.get_term() && lhs.get_val_type() == rhs.get_val_type() &&
          lhs.get_buf().size() == rhs.get_buf().size() && memcmp(lhs.get_buf().data_begin(), rhs.get_buf().data_begin(), lhs.get_buf().size()) == 0;   
}

}

BOOST_AUTO_TEST_CASE(test_log_store) {
   new_temp_directory_path tmp_dir;
   blockvault::log_store   test_store(tmp_dir);

   BOOST_CHECK_EQUAL(test_store.start_index(), 1);
   BOOST_CHECK_EQUAL(test_store.next_slot(), 1);
   BOOST_CHECK_EQUAL(test_store.last_entry()->get_term(), 0);


   // test append()
   ulong term = 0;
   int type = 0;
   std::vector<nuraft::ptr<nuraft::log_entry>> entries;
   entries.emplace_back();
   for (int i = 0; i < 10; ++i) {
      // change term and type so that they won't always be the same
      if (i % 2) 
         ++term;
      else {
         type = (++type) % 6;
      }

      auto entry = make_log_entry(term, static_cast<nuraft::log_val_type>(type));
      entries.push_back(entry);
      test_store.append(entry);

      BOOST_CHECK_EQUAL(test_store.start_index(), 1);
      BOOST_CHECK_EQUAL(test_store.next_slot(), i + 2);

      auto ret_entry = test_store.last_entry();
      BOOST_CHECK_EQUAL(*entry, *ret_entry);
   }

   // test entry_at()
   for (int i = 1; i <= 10; ++i) {
      BOOST_CHECK_EQUAL(*entries[i], *test_store.entry_at(i));
   }

   // test term_at
   for (int i = 1; i <= 10; ++i) {
      BOOST_CHECK_EQUAL(entries[i]->get_term(), test_store.term_at(i));
   }

   // test log_entries()
   auto r0 = test_store.log_entries(3, 8);
   for (int idx = 3; idx < 8; ++idx) {
      BOOST_CHECK_EQUAL(*entries[idx], *(r0->at(idx-3)));
   }

   // test log_entries_ext()
   std::vector<uint64_t> total_sizes;
   total_sizes.resize(10);
   auto entry_buf_size = [](const nuraft::ptr<nuraft::log_entry>& e) { return e->get_buf().size(); };
   std::partial_sum(boost::make_transform_iterator(entries.begin()+1, entry_buf_size),
                    boost::make_transform_iterator(entries.end(), entry_buf_size), total_sizes.begin());

   auto r1 = test_store.log_entries_ext(2, 10, total_sizes[5]-total_sizes[0]);
   BOOST_CHECK_EQUAL(r1->size(), 5);

   auto r2 = test_store.log_entries_ext(2, 10, -1);
   BOOST_CHECK_EQUAL(r2->size(), 0);

   auto r3 = test_store.log_entries_ext(2, 10, total_sizes[9]-total_sizes[0]+100);
   BOOST_CHECK_EQUAL(r3->size(), 8);

   // test pack() and apply_pack()

   auto pack = test_store.pack(5, 5);

   new_temp_directory_path tmp_dir2;
   blockvault::log_store   test_store2(tmp_dir2);

   test_store2.apply_pack(5, *pack);
   BOOST_CHECK_EQUAL(test_store2.start_index(), 5);
   BOOST_CHECK_EQUAL(test_store2.next_slot(), 10);

   for (int i = 5; i < 10; ++i) {
      BOOST_CHECK_EQUAL(*entries[i], *test_store2.entry_at(i));
   }


   pack = test_store.pack(8, 3);
   test_store2.apply_pack(8, *pack);
   BOOST_CHECK_EQUAL(test_store2.start_index(), 5);
   BOOST_CHECK_EQUAL(test_store2.next_slot(), 11);

   for (int i = 8; i < 11; ++i) {
      BOOST_CHECK_EQUAL(*entries[i], *test_store2.entry_at(i));
   }

   // test write_at()
   auto entry = make_log_entry(term, static_cast<nuraft::log_val_type>(type));
   test_store.write_at(5, entry);
   BOOST_CHECK_EQUAL(test_store.next_slot(), 6);
   for (int i = 1; i <= 4; ++i) {
      BOOST_CHECK_EQUAL(*entries[i], *test_store.entry_at(i));
   }
   BOOST_CHECK_EQUAL(*entry, *test_store.entry_at(5));

   // test compact()

   test_store.compact(3);
   BOOST_CHECK_EQUAL(test_store.start_index(), 4);
   BOOST_CHECK_EQUAL(test_store.next_slot(), 6);

}
