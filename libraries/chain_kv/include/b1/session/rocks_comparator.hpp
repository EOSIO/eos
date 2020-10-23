#pragma once

#include <boost/endian/detail/intrinsic.hpp>
#include <b1/session/shared_bytes.hpp>
#include <rocksdb/comparator.h>
#include <rocksdb/slice.h>

namespace eosio::session {

class rocks_comparator : public rocksdb::Comparator {
 public:
   int         Compare(const rocksdb::Slice& a, const rocksdb::Slice& b) const override;
   const char* Name() const override;
   void        FindShortestSeparator(std::string* start, const rocksdb::Slice& limit) const override;
   void        FindShortSuccessor(std::string* key) const override;
};

inline int rocks_comparator::Compare(const rocksdb::Slice& a, const rocksdb::Slice& b) const {
   const auto min_len = std::min(eosio::session::details::aligned_size(a.size_), eosio::session::details::aligned_size(b.size_));
   auto       left    = uint64_t{};
   auto       right   = uint64_t{};

   for (size_t i = 0; i < min_len / sizeof(uint64_t); ++i) {
      std::memcpy(&left, a.data_ + (i * sizeof(uint64_t)), sizeof(uint64_t));
      std::memcpy(&right, b.data_ + (i * sizeof(uint64_t)), sizeof(uint64_t));

      left  = BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_8(left);
      right = BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_8(right);

      if (left < right) {
         return -1;
      }
      if (left > right) {
         return 1;
      }
   }

   if (a.size_ < b.size_)
      return -1;
   else if (a.size_ > b.size_)
      return 1;

   return 0;
}

inline const char* rocks_comparator::Name() const { return "rocks chunked comparator"; }
inline void        rocks_comparator::FindShortestSeparator(std::string* start, const rocksdb::Slice& limit) const {}
inline void        rocks_comparator::FindShortSuccessor(std::string* key) const {}

} // namespace eosio::session
