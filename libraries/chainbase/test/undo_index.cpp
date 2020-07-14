#include <chainbase/undo_index.hpp>

#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>


namespace {
int exception_counter = 0;
int throw_at = -1;
struct test_exception_base {};
template<typename E>
struct test_exception : E, test_exception_base {
  template<typename... A>
  test_exception(A&&... a) : E{static_cast<E&&>(a)...} {}
};
template<typename E, typename... A>
void throw_point(A&&... a) {
   if(throw_at != -1 && exception_counter++ >= throw_at) {
     throw test_exception<E>{static_cast<A&&>(a)...};
   }
}
template<typename F>
void test_exceptions(F&& f) {
   for(throw_at = 0; ; ++throw_at) {
      exception_counter = 0;
      try {
         f();
         break;
      } catch(test_exception_base&) {}
   }
   throw_at = -1;
   exception_counter = 0;
}

struct throwing_copy {
   throwing_copy() { throw_point<std::bad_alloc>(); }
   throwing_copy(const throwing_copy&) { throw_point<std::bad_alloc>(); }
   throwing_copy(throwing_copy&&) noexcept = default;
   throwing_copy& operator=(const throwing_copy&) { throw_point<std::bad_alloc>(); return *this; }
   throwing_copy& operator=(throwing_copy&&) noexcept = default;
};

template<typename T>
struct test_allocator : std::allocator<T> {
   test_allocator() = default;
   template<typename U>
   test_allocator(const test_allocator<U>&) {}
   template<typename U>
   struct rebind { using other = test_allocator<U>; };
   T* allocate(std::size_t count) {
      throw_point<std::bad_alloc>();
      return std::allocator<T>::allocate(count);
   }
};

template<typename F>
struct scope_fail {
   scope_fail(F&& f) : _f{static_cast<F&&>(f)}, _exception_count{std::uncaught_exceptions()} {}
   ~scope_fail() {
      if(_exception_count != std::uncaught_exceptions()) _f();
   }
   F _f;
   int _exception_count;
};

struct basic_element_t {
   template<typename C, typename A>
   basic_element_t(C&& c, const std::allocator<A>&) { c(*this); }
   uint64_t id;
   throwing_copy dummy;
};

// TODO: Replace with boost::multi_index::key once we bump our minimum Boost version to at least 1.69
template<typename T>
struct key_impl;
template<typename C, typename T>
struct key_impl<T C::*> { template<auto F> using fn = boost::multi_index::member<C, T, F>; };

template<auto Fn>
using key = typename key_impl<decltype(Fn)>::template fn<Fn>;

}

BOOST_AUTO_TEST_SUITE(undo_index_tests)

#define EXCEPTION_TEST_CASE(name)                               \
   void name##impl();                                         \
   BOOST_AUTO_TEST_CASE(name) { test_exceptions(&name##impl); } \
   void name##impl ()

EXCEPTION_TEST_CASE(test_simple) {
   chainbase::undo_index<basic_element_t, test_allocator<basic_element_t>, boost::multi_index::ordered_unique<key<&basic_element_t::id>>> i0;
   i0.emplace([](basic_element_t& elem) {});
   const basic_element_t* element = i0.find(0);
   BOOST_TEST((element != nullptr && element->id == 0));
   const basic_element_t* e1 = i0.find(1);
   BOOST_TEST(e1 == nullptr);
   i0.emplace([](basic_element_t& elem) {});
   const basic_element_t* e2 = i0.find(1);
   BOOST_TEST((e2 != nullptr && e2->id == 1));

   i0.modify(*element, [](basic_element_t& elem) {});
   i0.remove(*element);
   element = i0.find(0);
   BOOST_TEST(element == nullptr);
}

struct test_element_t {
   template<typename C, typename A>
   test_element_t(C&& c, const std::allocator<A>&) { c(*this); }
   uint64_t id;
   int secondary;
   throwing_copy dummy;
};

// If an exception is thrown while an undo session is active, undo will restore the state.
template<typename C>
auto capture_state(const C& index) {
   std::vector<std::pair<test_element_t, const test_element_t*>> vec;
   for(const auto& elem : index) {
     vec.emplace_back(elem, &elem);
   }
   return scope_fail{[vec = std::move(vec), &index]{
      BOOST_TEST(index.size() == vec.size());
      for(const auto& [elem, ptr] : vec) {
         auto * actual0 = index.find(elem.id);
         BOOST_TEST(actual0 == ptr); // reference stability is guaranteed
         if (actual0 != nullptr) {
            BOOST_TEST(actual0->id == elem.id);
            BOOST_TEST(actual0->secondary == elem.secondary);
         }
         auto actual1iter = index.template get<1>().find(elem.secondary);
         BOOST_TEST((actual1iter != index.template get<1>().end() && &*actual1iter == actual0));
      }
   }};
}

EXCEPTION_TEST_CASE(test_insert_undo) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary> > > i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session = i0.start_undo_session(true);
   i0.emplace([](test_element_t& elem) { elem.secondary = 12; });
   BOOST_TEST(i0.find(1)->secondary == 12);
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
   BOOST_TEST(i0.find(1) == nullptr);
}

EXCEPTION_TEST_CASE(test_insert_squash) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary> > > i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session0 = i0.start_undo_session(true);
   auto session1 = i0.start_undo_session(true);
   i0.emplace([](test_element_t& elem) { elem.secondary = 12; });
   BOOST_TEST(i0.find(1)->secondary == 12);
   session1.squash();
   BOOST_TEST(i0.find(1)->secondary == 12);
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
   BOOST_TEST(i0.find(1) == nullptr);
}

EXCEPTION_TEST_CASE(test_insert_push) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary> > > i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session = i0.start_undo_session(true);
   i0.emplace([](test_element_t& elem) { elem.secondary = 12; });
   BOOST_TEST(i0.find(1)->secondary == 12);
   session.push();
   i0.commit(i0.revision());
   }
   BOOST_TEST(!i0.has_undo_session());
   BOOST_TEST(i0.find(0)->secondary == 42);
   BOOST_TEST(i0.find(1)->secondary == 12);
}

EXCEPTION_TEST_CASE(test_modify_undo) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session = i0.start_undo_session(true);
   i0.modify(*i0.find(0), [](test_element_t& elem) { elem.secondary = 18; });
   BOOST_TEST(i0.find(0)->secondary == 18);
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
}

EXCEPTION_TEST_CASE(test_modify_squash) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session0 = i0.start_undo_session(true);
   auto session1 = i0.start_undo_session(true);
   i0.modify(*i0.find(0), [](test_element_t& elem) { elem.secondary = 18; });
   BOOST_TEST(i0.find(0)->secondary == 18);
   session1.squash();
   BOOST_TEST(i0.find(0)->secondary == 18);
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
}

EXCEPTION_TEST_CASE(test_modify_push) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session = i0.start_undo_session(true);
   i0.modify(*i0.find(0), [](test_element_t& elem) { elem.secondary = 18; });
   BOOST_TEST(i0.find(0)->secondary == 18);
   session.push();
   i0.commit(i0.revision());
   }
   BOOST_TEST(!i0.has_undo_session());
   BOOST_TEST(i0.find(0)->secondary == 18);
}

EXCEPTION_TEST_CASE(test_remove_undo) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session = i0.start_undo_session(true);
   i0.remove(*i0.find(0));
   BOOST_TEST(i0.find(0) == nullptr);
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
}

EXCEPTION_TEST_CASE(test_remove_squash) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session0 = i0.start_undo_session(true);
   auto session1 = i0.start_undo_session(true);
   i0.remove(*i0.find(0));
   BOOST_TEST(i0.find(0) == nullptr);
   session1.squash();
   BOOST_TEST(i0.find(0) == nullptr);
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
}

EXCEPTION_TEST_CASE(test_remove_push) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session = i0.start_undo_session(true);
   i0.remove(*i0.find(0));
   BOOST_TEST(i0.find(0) == nullptr);
   session.push();
   i0.commit(i0.revision());
   }
   BOOST_TEST(!i0.has_undo_session());
   BOOST_TEST(i0.find(0) == nullptr);
}

EXCEPTION_TEST_CASE(test_insert_modify) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   i0.emplace([](test_element_t& elem) { elem.secondary = 12; });
   BOOST_TEST(i0.find(1)->secondary == 12);
   i0.modify(*i0.find(1), [](test_element_t& elem) { elem.secondary = 24; });
   BOOST_TEST(i0.find(1)->secondary == 24);
}

EXCEPTION_TEST_CASE(test_insert_modify_undo) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session = i0.start_undo_session(true);
   i0.emplace([](test_element_t& elem) { elem.secondary = 12; });
   BOOST_TEST(i0.find(1)->secondary == 12);
   i0.modify(*i0.find(1), [](test_element_t& elem) { elem.secondary = 24; });
   BOOST_TEST(i0.find(1)->secondary == 24);
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
   BOOST_TEST(i0.find(1) == nullptr);
}


EXCEPTION_TEST_CASE(test_insert_modify_squash) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session1 = i0.start_undo_session(true);
   i0.emplace([](test_element_t& elem) { elem.secondary = 12; });
   BOOST_TEST(i0.find(1)->secondary == 12);
   auto session2 = i0.start_undo_session(true);
   i0.modify(*i0.find(1), [](test_element_t& elem) { elem.secondary = 24; });
   BOOST_TEST(i0.find(1)->secondary == 24);
   session2.squash();
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
   BOOST_TEST(i0.find(1) == nullptr);
}

EXCEPTION_TEST_CASE(test_insert_remove_undo) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session = i0.start_undo_session(true);
   i0.emplace([](test_element_t& elem) { elem.secondary = 12; });
   BOOST_TEST(i0.find(1)->secondary == 12);
   i0.remove(*i0.find(1));
   BOOST_TEST(i0.find(1) == nullptr);
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
   BOOST_TEST(i0.find(1) == nullptr);
}

EXCEPTION_TEST_CASE(test_insert_remove_squash) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session1 = i0.start_undo_session(true);
   i0.emplace([](test_element_t& elem) { elem.secondary = 12; });
   BOOST_TEST(i0.find(1)->secondary == 12);
   auto session2 = i0.start_undo_session(true);
   i0.remove(*i0.find(1));
   BOOST_TEST(i0.find(1) == nullptr);
   session2.squash();
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
   BOOST_TEST(i0.find(1) == nullptr);
}

EXCEPTION_TEST_CASE(test_modify_modify_undo) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session = i0.start_undo_session(true);
   i0.modify(*i0.find(0), [](test_element_t& elem) { elem.secondary = 18; });
   BOOST_TEST(i0.find(0)->secondary == 18);
   i0.modify(*i0.find(0), [](test_element_t& elem) { elem.secondary = 24; });
   BOOST_TEST(i0.find(0)->secondary == 24);
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
}

EXCEPTION_TEST_CASE(test_modify_modify_squash) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session1 = i0.start_undo_session(true);
   i0.modify(*i0.find(0), [](test_element_t& elem) { elem.secondary = 18; });
   BOOST_TEST(i0.find(0)->secondary == 18);
   auto session2 = i0.start_undo_session(true);
   i0.modify(*i0.find(0), [](test_element_t& elem) { elem.secondary = 24; });
   BOOST_TEST(i0.find(0)->secondary == 24);
   session2.squash();
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
}

EXCEPTION_TEST_CASE(test_modify_remove_undo) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session = i0.start_undo_session(true);
   i0.modify(*i0.find(0), [](test_element_t& elem) { elem.secondary = 18; });
   BOOST_TEST(i0.find(0)->secondary == 18);
   i0.remove(*i0.find(0));
   BOOST_TEST(i0.find(0) == nullptr);
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
}

EXCEPTION_TEST_CASE(test_modify_remove_squash) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   auto undo_checker = capture_state(i0);
   auto session1 = i0.start_undo_session(true);
   i0.modify(*i0.find(0), [](test_element_t& elem) { elem.secondary = 18; });
   BOOST_TEST(i0.find(0)->secondary == 18);
   auto session2 = i0.start_undo_session(true);
   i0.remove(*i0.find(0));
   BOOST_TEST(i0.find(0) == nullptr);
   session2.squash();
   }
   BOOST_TEST(i0.find(0)->secondary == 42);
}

EXCEPTION_TEST_CASE(test_squash_one) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   {
   i0.modify(*i0.find(0), [](test_element_t& elem) { elem.secondary = 18; });
   BOOST_TEST(i0.find(0)->secondary == 18);
   auto session2 = i0.start_undo_session(true);
   i0.remove(*i0.find(0));
   BOOST_TEST(i0.find(0) == nullptr);
   session2.squash();
   }
}

EXCEPTION_TEST_CASE(test_insert_non_unique) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.find(0)->secondary == 42);
   BOOST_CHECK_THROW(i0.emplace([](test_element_t& elem) { elem.secondary = 42; }),  std::exception);
   BOOST_TEST(i0.find(0)->secondary == 42);
}

struct conflict_element_t {
   template<typename C, typename A>
   conflict_element_t(C&& c, const test_allocator<A>&) { c(*this); }
   uint64_t id;
   int x0;
   int x1;
   int x2;
   throwing_copy dummy;
};

EXCEPTION_TEST_CASE(test_modify_conflict) {
   chainbase::undo_index<conflict_element_t, test_allocator<conflict_element_t>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::x0>>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::x1>>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::x2>>> i0;
   // insert 3 elements
   i0.emplace([](conflict_element_t& elem) { elem.x0 = 0; elem.x1 = 10; elem.x2 = 10; });
   i0.emplace([](conflict_element_t& elem) { elem.x0 = 11; elem.x1 = 1; elem.x2 = 11; });
   i0.emplace([](conflict_element_t& elem) { elem.x0 = 12; elem.x1 = 12; elem.x2 = 2; });
   {
   auto session = i0.start_undo_session(true);
   // set them to a different value
   i0.modify(*i0.find(0), [](conflict_element_t& elem) { elem.x0 = 10; elem.x1 = 10; elem.x2 = 10; });
   i0.modify(*i0.find(1), [](conflict_element_t& elem) { elem.x0 = 11; elem.x1 = 11; elem.x2 = 11; });
   i0.modify(*i0.find(2), [](conflict_element_t& elem) { elem.x0 = 12; elem.x1 = 12; elem.x2 = 12; });
   // create a circular conflict with the original values
   i0.modify(*i0.find(0), [](conflict_element_t& elem) { elem.x0 = 10; elem.x1 = 1; elem.x2 = 10; });
   i0.modify(*i0.find(1), [](conflict_element_t& elem) { elem.x0 = 11; elem.x1 = 11; elem.x2 = 2; });
   i0.modify(*i0.find(2), [](conflict_element_t& elem) { elem.x0 = 0; elem.x1 = 12; elem.x2 = 12; });
   }
   BOOST_TEST(i0.find(0)->x0 == 0);
   BOOST_TEST(i0.find(1)->x1 == 1);
   BOOST_TEST(i0.find(2)->x2 == 2);
   // Check lookup in the other indices
   BOOST_TEST(i0.get<1>().find(0)->x0 == 0);
   BOOST_TEST(i0.get<1>().find(11)->x0 == 11);
   BOOST_TEST(i0.get<1>().find(12)->x0 == 12);
   BOOST_TEST(i0.get<2>().find(10)->x1 == 10);
   BOOST_TEST(i0.get<2>().find(1)->x1 == 1);
   BOOST_TEST(i0.get<2>().find(12)->x1 == 12);
   BOOST_TEST(i0.get<3>().find(10)->x2 == 10);
   BOOST_TEST(i0.get<3>().find(11)->x2 == 11);
   BOOST_TEST(i0.get<3>().find(2)->x2 == 2);
}

BOOST_DATA_TEST_CASE(test_insert_fail, boost::unit_test::data::make({true, false}), use_undo) {
   chainbase::undo_index<conflict_element_t, test_allocator<conflict_element_t>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::x0>>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::x1>>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::x2>>> i0;
   // insert 3 elements
   i0.emplace([](conflict_element_t& elem) { elem.x0 = 10; elem.x1 = 10; elem.x2 = 10; });
   i0.emplace([](conflict_element_t& elem) { elem.x0 = 11; elem.x1 = 11; elem.x2 = 11; });
   i0.emplace([](conflict_element_t& elem) { elem.x0 = 12; elem.x1 = 12; elem.x2 = 12; });
   {
   auto session = i0.start_undo_session(true);
   // Insert a value with a duplicate
   BOOST_CHECK_THROW(i0.emplace([](conflict_element_t& elem) { elem.x0 = 81; elem.x1 = 11; elem.x2 = 91; }), std::logic_error);
   }
   BOOST_TEST(i0.find(0)->x0 == 10);
   BOOST_TEST(i0.find(1)->x1 == 11);
   BOOST_TEST(i0.find(2)->x2 == 12);
   // Check lookup in the other indices
   BOOST_TEST(i0.get<1>().find(10)->x0 == 10);
   BOOST_TEST(i0.get<1>().find(11)->x0 == 11);
   BOOST_TEST(i0.get<1>().find(12)->x0 == 12);
   BOOST_TEST(i0.get<2>().find(10)->x1 == 10);
   BOOST_TEST(i0.get<2>().find(11)->x1 == 11);
   BOOST_TEST(i0.get<2>().find(12)->x1 == 12);
   BOOST_TEST(i0.get<3>().find(10)->x2 == 10);
   BOOST_TEST(i0.get<3>().find(11)->x2 == 11);
   BOOST_TEST(i0.get<3>().find(12)->x2 == 12);
}

EXCEPTION_TEST_CASE(test_modify_fail) {
   chainbase::undo_index<conflict_element_t, test_allocator<conflict_element_t>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::x0>>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::x1>>,
                         boost::multi_index::ordered_unique<key<&conflict_element_t::x2>>> i0;
   // insert 3 elements
   i0.emplace([](conflict_element_t& elem) { elem.x0 = 10; elem.x1 = 10; elem.x2 = 10; });
   i0.emplace([](conflict_element_t& elem) { elem.x0 = 11; elem.x1 = 11; elem.x2 = 11; });
   i0.emplace([](conflict_element_t& elem) { elem.x0 = 12; elem.x1 = 12; elem.x2 = 12; });
   {
   auto session = i0.start_undo_session(true);
   // Insert a value with a duplicate
   i0.emplace([](conflict_element_t& elem) { elem.x0 = 71; elem.x1 = 81; elem.x2 = 91; });
   BOOST_CHECK_THROW(i0.modify(i0.get(3), [](conflict_element_t& elem) { elem.x0 = 71; elem.x1 = 10; elem.x2 = 91; }), std::logic_error);
   }
   BOOST_TEST(i0.get<0>().size() == 3);
   BOOST_TEST(i0.get<1>().size() == 3);
   BOOST_TEST(i0.get<2>().size() == 3);
   BOOST_TEST(i0.get<3>().size() == 3);
   BOOST_TEST(i0.find(0)->x0 == 10);
   BOOST_TEST(i0.find(1)->x1 == 11);
   BOOST_TEST(i0.find(2)->x2 == 12);
   // Check lookup in the other indices
   BOOST_TEST(i0.get<1>().find(10)->x0 == 10);
   BOOST_TEST(i0.get<1>().find(11)->x0 == 11);
   BOOST_TEST(i0.get<1>().find(12)->x0 == 12);
   BOOST_TEST(i0.get<2>().find(10)->x1 == 10);
   BOOST_TEST(i0.get<2>().find(11)->x1 == 11);
   BOOST_TEST(i0.get<2>().find(12)->x1 == 12);
   BOOST_TEST(i0.get<3>().find(10)->x2 == 10);
   BOOST_TEST(i0.get<3>().find(11)->x2 == 11);
   BOOST_TEST(i0.get<3>().find(12)->x2 == 12);
}

struct by_secondary {};

BOOST_AUTO_TEST_CASE(test_project) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<boost::multi_index::tag<by_secondary>, key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 42; });
   BOOST_TEST(i0.project<by_secondary>(i0.begin()) == i0.get<by_secondary>().begin());
   BOOST_TEST(i0.project<by_secondary>(i0.end()) == i0.get<by_secondary>().end());
   BOOST_TEST(i0.project<1>(i0.begin()) == i0.get<by_secondary>().begin());
   BOOST_TEST(i0.project<1>(i0.end()) == i0.get<by_secondary>().end());
}


EXCEPTION_TEST_CASE(test_remove_tracking_session) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 20; });
   auto session = i0.start_undo_session(true);
   auto tracker = i0.track_removed();
   i0.emplace([](test_element_t& elem) { elem.secondary = 21; });
   const test_element_t& elem0 = *i0.find(0);
   const test_element_t& elem1 = *i0.find(1);
   BOOST_CHECK(!tracker.is_removed(elem0));
   BOOST_CHECK(!tracker.is_removed(elem1));
   tracker.remove(elem0);
   tracker.remove(elem1);
   BOOST_CHECK(tracker.is_removed(elem0));
   BOOST_CHECK(tracker.is_removed(elem1));
}


EXCEPTION_TEST_CASE(test_remove_tracking_no_session) {
   chainbase::undo_index<test_element_t, test_allocator<test_element_t>,
                         boost::multi_index::ordered_unique<key<&test_element_t::id>>,
                         boost::multi_index::ordered_unique<key<&test_element_t::secondary>>> i0;
   i0.emplace([](test_element_t& elem) { elem.secondary = 20; });
   auto tracker = i0.track_removed();
   i0.emplace([](test_element_t& elem) { elem.secondary = 21; });
   const test_element_t& elem0 = *i0.find(0);
   const test_element_t& elem1 = *i0.find(1);
   BOOST_CHECK(!tracker.is_removed(elem0));
   BOOST_CHECK(!tracker.is_removed(elem1));
   tracker.remove(elem0);
   tracker.remove(elem1);
   BOOST_CHECK(tracker.is_removed(elem0));
   BOOST_CHECK(tracker.is_removed(elem1));
}

BOOST_AUTO_TEST_SUITE_END()
