#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>
#include <string_view>

namespace eosio::session {

class shared_bytes;

template <typename T>
shared_bytes make_shared_bytes(const T* data, size_t length);

inline shared_bytes make_shared_bytes(const uint8_t* data, size_t length);

template <typename T>
shared_bytes make_shared_bytes(const T* data, size_t length);

// \brief An immutable type to represent a pointer and a length.
//
class shared_bytes {
 public:
   template <typename T>
   friend shared_bytes make_shared_bytes(const T* data, size_t length);

   friend shared_bytes make_shared_bytes(const uint8_t* data, size_t length);

   using iterator       = const uint8_t*;
   using const_iterator = const iterator;

 public:
   shared_bytes(const shared_bytes& b) = default;
   shared_bytes(shared_bytes&& b)      = default;
   ~shared_bytes()                     = default;

   shared_bytes& operator=(const shared_bytes& b) = default;
   shared_bytes& operator=(shared_bytes&& b) = default;

   const uint8_t* const data() const;
   size_t               size() const;

   uint8_t operator[](size_t index) const;

   bool operator==(const shared_bytes& other) const;
   bool operator!=(const shared_bytes& other) const;

   bool operator!() const;
        operator bool() const;

   bool operator<(const shared_bytes& other) const;
   bool operator>(const shared_bytes& other) const;

   iterator begin() const;
   iterator end() const;

   static const shared_bytes& invalid();

 private:
   shared_bytes() = default;

 private:
   std::shared_ptr<uint8_t> m_data;
   size_t                   m_size{ 0 };
};

// \brief Creates a new shared_bytes instance with the given pointer and length.
//
// \tparam T The type stored in the array.
// \\param data A pointer to an array of items to store in a shared_bytes instance. \param
// length The number of items in the array.
template <typename T>
shared_bytes make_shared_bytes(const T* data, size_t length) {
   return make_shared_bytes(reinterpret_cast<const uint8_t*>(data), length * sizeof(T));
}

inline shared_bytes make_shared_bytes(const uint8_t* data, size_t length) {
   auto result = shared_bytes{};

   if (!data || length == 0) {
      return result;
   }

   auto* chunk = std::allocator<uint8_t>{}.allocate(length);
   std::memcpy(chunk, data, length);

   auto deleter  = [&](auto* chunk) { std::allocator<uint8_t>{}.deallocate(chunk, length); };
   result.m_data = std::shared_ptr<uint8_t>(chunk, deleter);
   result.m_size = length;

   return result;
}

inline const shared_bytes& shared_bytes::invalid() {
   static shared_bytes bad;
   return bad;
}

inline const uint8_t* const shared_bytes::data() const { return m_data ? &(m_data.get()[0]) : nullptr; }

inline size_t shared_bytes::size() const { return m_size; }

inline uint8_t shared_bytes::operator[](size_t index) const {
   assert(index < size());
   assert(m_data);
   return m_data.get()[index];
}

inline bool shared_bytes::operator==(const shared_bytes& other) const {
   if (m_data == other.m_data) {
      // Same pointer.
      return true;
   }

   if (m_data && other.m_data) {
      if (size() != other.size()) {
         return false;
      }
      return std::memcmp(m_data.get(), other.m_data.get(), size()) == 0 ? true : false;
   }

   return false;
}

inline bool shared_bytes::operator!=(const shared_bytes& other) const { return !(*this == other); }

inline bool shared_bytes::operator<(const shared_bytes& other) const {
   if (*this != eosio::session::shared_bytes::invalid() && other != eosio::session::shared_bytes::invalid()) {
      return std::string_view{ reinterpret_cast<const char*>(this->data()), this->size() } <
             std::string_view{ reinterpret_cast<const char*>(other.data()), other.size() };
   }
   if (*this == eosio::session::shared_bytes::invalid()) {
      return false;
   }
   return true;
}

inline bool shared_bytes::operator>(const shared_bytes& other) const {
   if (*this != eosio::session::shared_bytes::invalid() && other != eosio::session::shared_bytes::invalid()) {
      return std::string_view{ reinterpret_cast<const char*>(this->data()), this->size() } >
             std::string_view{ reinterpret_cast<const char*>(other.data()), other.size() };
   }
   if (*this == eosio::session::shared_bytes::invalid()) {
      return false;
   }
   return true;
}

inline bool shared_bytes::operator!() const { return *this == shared_bytes::invalid(); }

inline shared_bytes::operator bool() const { return *this != shared_bytes::invalid(); }

inline shared_bytes::iterator shared_bytes::begin() const { return m_data.get(); }

inline shared_bytes::iterator shared_bytes::end() const { return m_data.get() + size(); }

} // namespace eosio::session

namespace std {

template <>
struct less<eosio::session::shared_bytes> {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      return lhs < rhs;
   };
};

template <>
struct greater<eosio::session::shared_bytes> {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      return lhs > rhs;
   };
};

template <>
struct hash<eosio::session::shared_bytes> {
   size_t operator()(const eosio::session::shared_bytes& b) const {
      return std::hash<std::string_view>{}({ reinterpret_cast<const char*>(b.data()), b.size() });
   }
};

template <>
struct equal_to<eosio::session::shared_bytes> {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      return lhs == rhs;
   }
};

} // namespace std
