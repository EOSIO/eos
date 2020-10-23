#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

#include <boost/endian/detail/intrinsic.hpp>

#include <fc/crypto/base64.hpp>

#include <eosio/chain/exceptions.hpp>

namespace eosio::session {

// \brief An immutable type to represent a pointer and a length.
//
class shared_bytes {
 public:
   using underlying_type_t = char;

   template <typename T>
   friend class std::hash;

   template <typename Iterator_traits>
   class shared_bytes_iterator {
    public:
      friend shared_bytes;

      using difference_type   = typename Iterator_traits::difference_type;
      using value_type        = typename Iterator_traits::value_type;
      using pointer           = typename Iterator_traits::pointer;
      using reference         = typename Iterator_traits::reference;
      using iterator_category = typename Iterator_traits::iterator_category;

      shared_bytes_iterator()                                   = default;
      shared_bytes_iterator(const shared_bytes_iterator& other) = default;
      shared_bytes_iterator(shared_bytes_iterator&& other)      = default;
      shared_bytes_iterator(char* buffer, size_t begin, size_t end, int64_t index);

      shared_bytes_iterator& operator=(const shared_bytes_iterator& other) = default;
      shared_bytes_iterator& operator=(shared_bytes_iterator&& other) = default;
      shared_bytes_iterator& operator++();
      shared_bytes_iterator& operator--();
      value_type             operator*() const;
      value_type             operator->() const;
      reference              operator*();
      reference              operator->();
      bool                   operator==(const shared_bytes_iterator& other) const;
      bool                   operator!=(const shared_bytes_iterator& other) const;
      shared_bytes_iterator& operator+=(int64_t offset);
      shared_bytes_iterator& operator-=(int64_t offset);
      shared_bytes_iterator  operator+(int64_t offset);
      shared_bytes_iterator  operator-(int64_t offset);
      difference_type        operator-(const shared_bytes_iterator& other);
      reference              operator[](size_t index);
      value_type             operator[](size_t index) const;

    private:
      underlying_type_t* m_buffer{ nullptr };
      size_t             m_end{ 0 };
      size_t             m_begin{ 0 };
      int64_t            m_index{ -1 };
   };

   struct iterator_traits {
      using difference_type   = int64_t;
      using value_type        = underlying_type_t;
      using pointer           = value_type*;
      using reference         = value_type&;
      using iterator_category = std::random_access_iterator_tag;
   };
   using iterator = shared_bytes_iterator<iterator_traits>;

 public:
   shared_bytes() = default;
   template <typename T>
   shared_bytes(const T* data, size_t size);
   shared_bytes(size_t size);
   shared_bytes(const shared_bytes& b) = default;
   shared_bytes(shared_bytes&& b)      = default;
   ~shared_bytes()                     = default;

   shared_bytes& operator=(const shared_bytes& b) = default;
   shared_bytes& operator=(shared_bytes&& b) = default;

   template <typename Compare1, typename Compare2>
   bool compare(const shared_bytes& other, const Compare1& compare1, const Compare2& compare2) const;
   bool operator==(const shared_bytes& other) const;
   bool operator!=(const shared_bytes& other) const;
   bool operator<(const shared_bytes& other) const;
   bool operator<=(const shared_bytes& other) const;
   bool operator>(const shared_bytes& other) const;
   bool operator>=(const shared_bytes& other) const;

   bool operator!() const;
        operator bool() const;

   shared_bytes      next();
   size_t            size() const;
   size_t            aligned_size() const;
   char*             data();
   const char* const data() const;

   iterator begin() const;
   iterator end() const;

 private:
   size_t                             m_size{ 0 };
   size_t                             m_offset{ 0 };
   std::shared_ptr<underlying_type_t> m_data;
};

namespace details {
   template <size_t byte_size = sizeof(uint64_t)>
   inline size_t aligned_size(size_t size) {
      if (size % byte_size != 0) {
         return ((size / byte_size) + 1) * byte_size;
      }
      return size;
   }
} // namespace details

template <typename T>
shared_bytes::shared_bytes(const T* data, size_t size)
    : m_size{ size * sizeof(T) }, m_offset{ eosio::session::details::aligned_size(m_size) - m_size }, m_data{ [&]() {
         if (!data || size == 0) {
            return std::shared_ptr<char>{};
         }

         auto  actual_size = m_size + m_offset;
         auto  result      = std::shared_ptr<underlying_type_t>{ new underlying_type_t[actual_size],
                                                           std::default_delete<underlying_type_t[]>() };
         auto* buffer      = result.get();
         std::memcpy(buffer, reinterpret_cast<const void*>(data), m_size);
         std::memset(buffer + m_size, 0, m_offset);
         return result;
      }() } {}

inline shared_bytes::shared_bytes(size_t size)
    : m_size{ size }, m_offset{ eosio::session::details::aligned_size(m_size) - m_size }, m_data{ [&]() {
         if (size == 0) {
            return std::shared_ptr<char>{};
         }

         auto actual_size = m_size + m_offset;
         auto result      = std::shared_ptr<underlying_type_t>{ new underlying_type_t[actual_size],
                                                           std::default_delete<underlying_type_t[]>() };
         std::memset(result.get(), 0, actual_size);
         return result;
      }() } {}

inline shared_bytes shared_bytes::next() {
   auto buffer = std::vector<unsigned char>{ std::begin(*this), std::end(*this) };

   while (!buffer.empty()) {
      if (++buffer.back()) {
         break;
      }
      buffer.pop_back();
   }

   return eosio::session::shared_bytes(buffer.data(), buffer.size());
}

inline size_t            shared_bytes::size() const { return m_size; }
inline size_t            shared_bytes::aligned_size() const { return eosio::session::details::aligned_size(m_size); }
inline char*             shared_bytes::data() { return m_data.get(); }
inline const char* const shared_bytes::data() const { return m_data.get(); }

template <typename Compare1, typename Compare2>
bool shared_bytes::compare(const shared_bytes& other, const Compare1& compare1, const Compare2& compare2) const {
   auto aligned_left_size  = eosio::session::details::aligned_size(m_size);
   auto aligned_right_size = eosio::session::details::aligned_size(other.m_size);

   auto* left_ptr  = m_data.get();
   auto* right_ptr = other.m_data.get();
   auto  left      = uint64_t{};
   auto  right     = uint64_t{};
   auto  result    = false;

   for (size_t i = 0; i < std::min(aligned_left_size, aligned_right_size) / sizeof(uint64_t); ++i) {
      std::memcpy(&left, left_ptr + (i * sizeof(uint64_t)), sizeof(uint64_t));
      std::memcpy(&right, right_ptr + (i * sizeof(uint64_t)), sizeof(uint64_t));

      left  = BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_8(left);
      right = BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_8(right);

      if (compare1(left, right, result)) {
         return result;
      }
   }

   return compare2();
}

inline bool shared_bytes::operator==(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return true;
   }

   if (size() != other.size()) {
      return false;
   }

   return compare(
         other,
         [](auto& left, auto& right, auto& result) {
            if (left != right) {
               result = false;
               return true;
            }
            return false;
         },
         []() { return true; });
}

inline bool shared_bytes::operator!=(const shared_bytes& other) const { return !(*this == other); }

inline bool shared_bytes::operator<(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return false;
   }
   return compare(
         other,
         [](auto& left, auto& right, auto& result) {
            if (left < right) {
               result = true;
               return true;
            } else if (left > right) {
               result = false;
               return true;
            }
            return false;
         },
         [&]() { return size() < other.size(); });
}

inline bool shared_bytes::operator<=(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return true;
   }
   return compare(
         other,
         [](auto& left, auto& right, auto& result) {
            if (left > right) {
               result = false;
               return true;
            }
            return false;
         },
         [&]() { return size() <= other.size(); });
}

inline bool shared_bytes::operator>(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return false;
   }
   return compare(
         other,
         [](auto& left, auto& right, auto& result) {
            if (left > right) {
               result = true;
               return true;
            } else if (left < right) {
               result = false;
               return true;
            }
            return false;
         },
         [&]() { return size() > other.size(); });
}

inline bool shared_bytes::operator>=(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return true;
   }
   return compare(
         other,
         [](auto& left, auto& right, auto& result) {
            if (left < right) {
               result = false;
               return true;
            }
            return false;
         },
         [&]() { return size() >= other.size(); });
}

inline bool shared_bytes::operator!() const { return *this == shared_bytes{}; }

inline shared_bytes::operator bool() const { return *this != shared_bytes{}; }

inline shared_bytes::iterator shared_bytes::begin() const {
   return iterator{ m_data.get(), 0, m_size - 1, m_size == 0 ? -1 : 0 };
}

inline shared_bytes::iterator shared_bytes::end() const { return iterator{ m_data.get(), 0, m_size - 1, -1 }; }

inline std::ostream& operator<<(std::ostream& os, const shared_bytes& bytes) {
   os << fc::base64_encode({ std::begin(bytes), std::end(bytes) });
   return os;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>::shared_bytes_iterator(char* buffer, size_t begin, size_t end,
                                                                            int64_t index)
    : m_buffer{ buffer }, m_end{ end }, m_begin{ begin }, m_index{ index } {}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>&
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator++() {
   if (m_index < m_end) {
      ++m_index;
   } else if (m_index == m_end) {
      m_index = -1;
   }
   return *this;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>&
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator--() {
   if (m_index > m_begin) {
      --m_index;
   }
   return *this;
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::value_type
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator*() const {
   return m_buffer[m_index];
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::value_type
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator->() const {
   return m_buffer[m_index];
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::reference
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator*() {
   return m_buffer[m_index];
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::reference
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator->() {
   return m_buffer[m_index];
}

template <typename Iterator_traits>
bool shared_bytes::shared_bytes_iterator<Iterator_traits>::operator==(const shared_bytes_iterator& other) const {
   return m_buffer == other.m_buffer && m_end == other.m_end && m_begin == other.m_begin && m_index == other.m_index;
}

template <typename Iterator_traits>
bool shared_bytes::shared_bytes_iterator<Iterator_traits>::operator!=(const shared_bytes_iterator& other) const {
   return !(*this == other);
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>&
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator+=(int64_t offset) {
   if (m_index + offset <= m_end) {
      m_index += offset;
   } else {
      m_index = -1;
   }
   return *this;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>&
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator-=(int64_t offset) {
   if (m_index - offset >= m_begin) {
      m_index -= offset;
   } else {
      m_index = m_begin;
   }
   return *this;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator+(int64_t offset) {
   auto new_iterator = *this;
   if (m_index + offset <= m_end) {
      new_iterator.m_index += offset;
   } else {
      new_iterator.m_index = -1;
   }
   return new_iterator;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator-(int64_t offset) {
   auto new_iterator = *this;
   if (m_index - offset >= m_begin) {
      new_iterator.m_index -= offset;
   } else {
      new_iterator.m_index = m_begin;
   }
   return new_iterator;
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::difference_type
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator-(const shared_bytes_iterator& other) {
   auto left_index  = m_index == -1 ? static_cast<int64_t>(m_end) + 1 : m_index;
   auto right_index = other.m_index == -1 ? static_cast<int64_t>(other.m_end) + 1 : other.m_index;
   return std::abs<int64_t>(left_index - right_index);
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::reference
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator[](size_t index) {
   return m_buffer[index];
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::value_type
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator[](size_t index) const {
   return m_buffer[index];
}

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
      if (b.size() == 0) {
         return 0;
      }
      return std::hash<std::string_view>{}({ b.m_data.get() + b.m_offset, b.m_size });
   }
};

template <>
struct equal_to<eosio::session::shared_bytes> {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      return lhs == rhs;
   }
};

} // namespace std
