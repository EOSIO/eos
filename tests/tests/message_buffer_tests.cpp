/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/net_plugin/message_buffer.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>


namespace eosio {
using namespace std;

size_t mb_size(boost::asio::mutable_buffer& mb) {
#if BOOST_VERSION >= 106600
   return mb.size();
#else
   return boost::asio::detail::buffer_size_helper(mb);
#endif
}

void* mb_data(boost::asio::mutable_buffer& mb) {
#if BOOST_VERSION >= 106600
   return mb.data();
#else
   return boost::asio::detail::buffer_cast_helper(mb);
#endif
}

BOOST_AUTO_TEST_SUITE(message_buffer_tests)

constexpr auto     def_buffer_size_mb = 4;
constexpr auto     def_buffer_size = 1024*1024*def_buffer_size_mb;

/// Test default construction and buffer sequence generation
BOOST_AUTO_TEST_CASE(message_buffer_construction)
{
  try {
    eosio::message_buffer<def_buffer_size> mb;
    BOOST_CHECK_EQUAL(mb.total_bytes(), def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_write(), def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_read(), 0);
    BOOST_CHECK_EQUAL(mb.read_ptr(), mb.write_ptr());

    auto mbs = mb.get_buffer_sequence_for_boost_async_read();
    auto mbsi = mbs.begin();
    BOOST_CHECK_EQUAL(mb_size(*mbsi), def_buffer_size);
    BOOST_CHECK_EQUAL(mb_data(*mbsi), mb.write_ptr());
    mbsi++;
    BOOST_CHECK(mbsi == mbs.end());
  }
  FC_LOG_AND_RETHROW()
}

/// Test buffer growth and shrinking
BOOST_AUTO_TEST_CASE(message_buffer_growth)
{
  try {
    eosio::message_buffer<def_buffer_size> mb;
    mb.add_buffer_to_chain();
    BOOST_CHECK_EQUAL(mb.total_bytes(), 2 * def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_write(), 2 * def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_read(), 0);
    BOOST_CHECK_EQUAL(mb.read_ptr(), mb.write_ptr());

    {
      auto mbs = mb.get_buffer_sequence_for_boost_async_read();
      auto mbsi = mbs.begin();
      BOOST_CHECK_EQUAL(mb_size(*mbsi), def_buffer_size);
      BOOST_CHECK_EQUAL(mb_data(*mbsi), mb.write_ptr());
      mbsi++;
      BOOST_CHECK(mbsi != mbs.end());
      BOOST_CHECK_EQUAL(mb_size(*mbsi), def_buffer_size);
      BOOST_CHECK_NE(mb_data(*mbsi), nullptr);
      mbsi++;
      BOOST_CHECK(mbsi == mbs.end());
    }

    mb.advance_write_ptr(100);
    BOOST_CHECK_EQUAL(mb.total_bytes(), 2 * def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_write(), 2 * def_buffer_size - 100);
    BOOST_CHECK_EQUAL(mb.bytes_to_read(), 100);
    BOOST_CHECK_NE(mb.read_ptr(), nullptr);
    BOOST_CHECK_NE(mb.write_ptr(), nullptr);
    BOOST_CHECK_EQUAL((mb.read_ptr() + 100), mb.write_ptr());

    {
      auto mbs = mb.get_buffer_sequence_for_boost_async_read();
      auto mbsi = mbs.begin();
      BOOST_CHECK_EQUAL(mb_size(*mbsi), def_buffer_size - 100);
      BOOST_CHECK_EQUAL(mb_data(*mbsi), mb.write_ptr());
      mbsi++;
      BOOST_CHECK(mbsi != mbs.end());
      BOOST_CHECK_EQUAL(mb_size(*mbsi), def_buffer_size);
      BOOST_CHECK_NE(mb_data(*mbsi), nullptr);
      mbsi++;
      BOOST_CHECK(mbsi == mbs.end());
    }

    mb.advance_read_ptr(50);
    BOOST_CHECK_EQUAL(mb.total_bytes(), 2 * def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_write(), 2 * def_buffer_size - 100);
    BOOST_CHECK_EQUAL(mb.bytes_to_read(), 50);

    mb.advance_write_ptr(def_buffer_size);
    BOOST_CHECK_EQUAL(mb.total_bytes(), 2 * def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_write(), def_buffer_size - 100);
    BOOST_CHECK_EQUAL(mb.bytes_to_read(), 50 + def_buffer_size);

    // Moving read_ptr into second block should reset second block to first
    mb.advance_read_ptr(def_buffer_size);
    BOOST_CHECK_EQUAL(mb.total_bytes(), def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_write(), def_buffer_size - 100);
    BOOST_CHECK_EQUAL(mb.bytes_to_read(), 50);

    // Moving read_ptr to write_ptr should shrink chain and reset ptrs
    mb.advance_read_ptr(50);
    BOOST_CHECK_EQUAL(mb.total_bytes(), def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_write(), def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_read(), 0);

    mb.add_buffer_to_chain();
    BOOST_CHECK_EQUAL(mb.total_bytes(), 2 * def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_write(), 2 * def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_read(), 0);

    mb.advance_write_ptr(50);
    BOOST_CHECK_EQUAL(mb.total_bytes(), 2 * def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_write(), 2 * def_buffer_size - 50);
    BOOST_CHECK_EQUAL(mb.bytes_to_read(), 50);

    // Moving read_ptr to write_ptr should shrink chain and reset ptrs
    mb.advance_read_ptr(50);
    BOOST_CHECK_EQUAL(mb.total_bytes(), def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_write(), def_buffer_size);
    BOOST_CHECK_EQUAL(mb.bytes_to_read(), 0);
  }
  FC_LOG_AND_RETHROW()
}

/// Test peek and read across multiple buffers
BOOST_AUTO_TEST_CASE(message_buffer_peek_read)
{
  try {
    {
      const uint32_t small = 32;
      eosio::message_buffer<small> mb;
      BOOST_CHECK_EQUAL(mb.total_bytes(), small);
      BOOST_CHECK_EQUAL(mb.bytes_to_write(), small);
      BOOST_CHECK_EQUAL(mb.bytes_to_read(), 0);
      BOOST_CHECK_EQUAL(mb.read_ptr(), mb.write_ptr());
      BOOST_CHECK_EQUAL(mb.read_index().first, 0);
      BOOST_CHECK_EQUAL(mb.read_index().second, 0);
      BOOST_CHECK_EQUAL(mb.write_index().first, 0);
      BOOST_CHECK_EQUAL(mb.write_index().second, 0);

      mb.add_space(100 - small);
      BOOST_CHECK_EQUAL(mb.total_bytes(), 4 * small);
      BOOST_CHECK_EQUAL(mb.bytes_to_write(), 4 * small);
      BOOST_CHECK_EQUAL(mb.bytes_to_read(), 0);
      BOOST_CHECK_EQUAL(mb.read_ptr(), mb.write_ptr());

      char* write_ptr = mb.write_ptr();
      for (char ind = 0; ind < 100; ) {
        *write_ptr = ind;
        ind++;
        if (ind % small == 0) {
          mb.advance_write_ptr(small);
          write_ptr = mb.write_ptr();
        } else {
          write_ptr++;
        }
      }
      mb.advance_write_ptr(100 % small);

      BOOST_CHECK_EQUAL(mb.total_bytes(), 4 * small);
      BOOST_CHECK_EQUAL(mb.bytes_to_write(), 4 * small - 100);
      BOOST_CHECK_EQUAL(mb.bytes_to_read(), 100);
      BOOST_CHECK_NE((void*) mb.read_ptr(), (void*) mb.write_ptr());
      BOOST_CHECK_EQUAL(mb.read_index().first, 0);
      BOOST_CHECK_EQUAL(mb.read_index().second, 0);
      BOOST_CHECK_EQUAL(mb.write_index().first, 3);
      BOOST_CHECK_EQUAL(mb.write_index().second, 4);

      char buffer[100];
      auto index = mb.read_index();
      mb.peek(buffer, 50, index);
      mb.peek(buffer+50, 50, index);
      for (int i=0; i < 100; i++) {
        BOOST_CHECK_EQUAL(i, buffer[i]);
      }

      BOOST_CHECK_EQUAL(mb.total_bytes(), 4 * small);
      BOOST_CHECK_EQUAL(mb.bytes_to_write(), 4 * small - 100);
      BOOST_CHECK_EQUAL(mb.bytes_to_read(), 100);
      BOOST_CHECK_NE((void*) mb.read_ptr(), (void*) mb.write_ptr());

      char buffer2[100];
      mb.read(buffer2, 100);
      for (int i=0; i < 100; i++) {
        BOOST_CHECK_EQUAL(i, buffer2[i]);
      }

      BOOST_CHECK_EQUAL(mb.total_bytes(), small);
      BOOST_CHECK_EQUAL(mb.bytes_to_write(), small);
      BOOST_CHECK_EQUAL(mb.bytes_to_read(), 0);
      BOOST_CHECK_EQUAL(mb.read_ptr(), mb.write_ptr());
    }
  }
  FC_LOG_AND_RETHROW()
}

/// Test automatic allocation when advancing the read_ptr to the end.
BOOST_AUTO_TEST_CASE(message_buffer_write_ptr_to_end)
{
  try {
    {
      const uint32_t small = 32;
      eosio::message_buffer<small> mb;
      BOOST_CHECK_EQUAL(mb.total_bytes(), small);
      BOOST_CHECK_EQUAL(mb.bytes_to_write(), small);
      BOOST_CHECK_EQUAL(mb.bytes_to_read(), 0);
      BOOST_CHECK_EQUAL(mb.read_ptr(), mb.write_ptr());
      BOOST_CHECK_EQUAL(mb.read_index().first, 0);
      BOOST_CHECK_EQUAL(mb.read_index().second, 0);
      BOOST_CHECK_EQUAL(mb.write_index().first, 0);
      BOOST_CHECK_EQUAL(mb.write_index().second, 0);

      char* write_ptr = mb.write_ptr();
      for (char ind = 0; ind < small; ind++) {
        *write_ptr = ind;
        write_ptr++;
      }
      mb.advance_write_ptr(small);

      BOOST_CHECK_EQUAL(mb.total_bytes(), 2 * small);
      BOOST_CHECK_EQUAL(mb.bytes_to_write(), small);
      BOOST_CHECK_EQUAL(mb.bytes_to_read(), small);
      BOOST_CHECK_NE((void*) mb.read_ptr(), (void*) mb.write_ptr());
      BOOST_CHECK_EQUAL(mb.read_index().first, 0);
      BOOST_CHECK_EQUAL(mb.read_index().second, 0);
      BOOST_CHECK_EQUAL(mb.write_index().first, 1);
      BOOST_CHECK_EQUAL(mb.write_index().second, 0);

      auto mbs = mb.get_buffer_sequence_for_boost_async_read();
      auto mbsi = mbs.begin();
      BOOST_CHECK_EQUAL(mb_size(*mbsi), small);
      BOOST_CHECK_EQUAL(mb_data(*mbsi), mb.write_ptr());
      BOOST_CHECK_EQUAL(mb.read_ptr()+small, mb.write_ptr());
      mbsi++;
      BOOST_CHECK(mbsi == mbs.end());
    }
  }
  FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace eosio

