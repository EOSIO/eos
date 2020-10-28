#pragma once

#include <b1/session/shared_bytes.hpp>
#include <rocksdb/comparator.h>
#include <rocksdb/slice.h>

namespace eosio::session {

/// \brief Custom comparator for use in RocksDB.
class rocks_comparator : public rocksdb::Comparator {
 public:
   int         Compare(const rocksdb::Slice& a, const rocksdb::Slice& b) const override;
   const char* Name() const override;
   void        FindShortestSeparator(std::string* start, const rocksdb::Slice& limit) const override;
   void        FindShortSuccessor(std::string* key) const override;
};

inline int rocks_comparator::Compare(const rocksdb::Slice& a, const rocksdb::Slice& b) const {
   if (a.data() == b.data()) {
      return false;
   }
   return eosio::session::details::aligned_compare(a.data(), a.size(), b.data(), b.size());
}

inline const char* rocks_comparator::Name() const { return "rocks chunked comparator"; }
inline void        rocks_comparator::FindShortestSeparator(std::string* start, const rocksdb::Slice& limit) const {}
inline void        rocks_comparator::FindShortSuccessor(std::string* key) const {}

} // namespace eosio::session
