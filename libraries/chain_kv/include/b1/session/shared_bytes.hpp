#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>
#include <numeric>
#include <string_view>
#include <vector>

#include <boost/endian/detail/intrinsic.hpp>

#include <fc/crypto/base64.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/raw.hpp>

#include <eosio/chain/exceptions.hpp>

namespace eosio::session {

class shared_bytes;

template <typename StringView, std::size_t N>
shared_bytes make_shared_bytes(std::array<StringView, N>&& data);

/// \brief A structure that represents a pointer and its length.
class shared_bytes {
 public:
   using underlying_type_t = char;

   template <typename T>
   friend struct std::hash;

   template <typename StringView, std::size_t N>
   friend shared_bytes make_shared_bytes(std::array<StringView, N>&& data);

   /// \brief Random access iterator on a shared_bytes instance.
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

      /// \brief Constructor.
      /// \param buffer A pointer to the beginning of the buffer that this iterator is iterating over.
      /// \param begin The starting point of the range within the buffer the iterator iterates over, inclusive.
      /// \param end The ending point of the range within the buffer the iterator iterates over, inclusive.
      /// \param index The starting index within the range to start iterating.
      /// \remarks -1 for the index indicates the end iterator.
      shared_bytes_iterator(char* buffer, int64_t begin, int64_t end, int64_t index);

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
      int64_t            m_end{ 0 };
      int64_t            m_begin{ 0 };
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
   /// \brief Default Constructor.
   /// \remarks Constructs a shared_bytes with a nullptr and no size.
   shared_bytes() = default;

   /// \brief Constructs a shared_bytes from an array and size.
   /// \param data A pointer to the beginning of an array.
   /// \param size The length of the array.
   /// \remarks The data is copied into this instance.  Memory is aligned on uint64_t boundary.
   /// \tparam T The data type of the items in the array.
   template <typename T>
   shared_bytes(const T* data, size_t size);

   /// \brief Constructs a shared_bytes with a buffer of the given size.
   /// \remarks Memory is aligned on uint64_t boundary.
   shared_bytes(size_t size);

   shared_bytes(const shared_bytes& b) = default;
   shared_bytes(shared_bytes&& b)      = default;
   ~shared_bytes()                     = default;

   shared_bytes& operator=(const shared_bytes& b) = default;
   shared_bytes& operator=(shared_bytes&& b) = default;

   bool operator==(const shared_bytes& other) const;
   bool operator!=(const shared_bytes& other) const;
   bool operator<(const shared_bytes& other) const;
   bool operator<=(const shared_bytes& other) const;
   bool operator>(const shared_bytes& other) const;
   bool operator>=(const shared_bytes& other) const;

   underlying_type_t& operator[](size_t index);
   underlying_type_t  operator[](size_t index) const;

   /// \brief Not operator.
   /// \return True if the underlying pointer is null or the size is 0, false otherwise.
   bool operator!() const;

   /// \brief bool conversion operator.
   /// \return True if the underlying pointer is not null or the size is greater than 0, false otherwise.
   operator bool() const;

   /// \brief Returns a new shared_bytes instances that represents the next largest value in lexicographical ordering.
   /// \remarks This is accomplished by adding one to last value in this shared_bytes instance and carrying over the
   /// value
   ///          to the next byte (from back to front) until no carry over is encountered.
   shared_bytes next() const;
   size_t       size() const;

   /// \brief Returns the size of the buffer when aligned on a the size of uint64_t.
   size_t aligned_size() const;

   bool              empty() const;
   char*             data();
   const char* const data() const;

   iterator begin() const;
   iterator end() const;

   static shared_bytes from_hex_string(const std::string& str);
   static shared_bytes truncate_key(const shared_bytes &key);

 private:
   size_t                             m_size{ 0 };
   size_t                             m_offset{ 0 };
   std::shared_ptr<underlying_type_t> m_data;
};

namespace details {
   /// \brief Adjusts the size to be aligned to the given byte_size value.
   /// \tparam byte_size The number of bytes of the alignment.
   template <size_t byte_size = sizeof(uint64_t)>
   inline size_t aligned_size(size_t size) {
      return (size + (byte_size - 1)) & ~(byte_size - 1);
   }

   /// \brief Compares two aligned buffers, lexicographically.
   /// \tparam byte_size The number of bytes of the alignment.
   /// \param left A pointer to the beginning of the buffer.
   /// \param left_size The unaligned size of the left buffer.
   /// \param right A pointer to the beginning of the buffer.
   /// \param right_size The unalinged size of the right buffer.
   /// \return
   ///  - A value less than 0 if left is less than right.
   ///  - A value greater than 0 if left is greater than right.
   ///  - A value of 0 if both buffers are equal.
   /// \remarks This comparison works by walking over the buffers, taking each 8 bytes and type punning that to a
   /// uint64_t, swapping the bytes of those integers and then performing integer comparison.
   template <size_t byte_size = sizeof(uint64_t)>
   inline int64_t aligned_compare(const char* left, int64_t left_size, const char* right, int64_t right_size) {
      auto iterations = std::min(aligned_size(left_size), aligned_size(right_size)) / byte_size;
      for (size_t i = 0; i < iterations; ++i) {
         auto offset      = i * byte_size;
         auto left_value  = uint64_t{};
         auto right_value = uint64_t{};
         std::memcpy(&left_value, left + offset, byte_size);
         std::memcpy(&right_value, right + offset, byte_size);
         // swizzle the bytes before performing the comparison.
         left_value  = BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_8(left_value);
         right_value = BOOST_ENDIAN_INTRINSIC_BYTE_SWAP_8(right_value);
         if (left_value == right_value) {
            continue;
         }
         return left_value < right_value ? -1 : 1;
      }
      return left_size - right_size;
   }
} // namespace details

/// \brief Constructs a new shared_bytes instance from an array of StringView instances.
/// \tparam StringView A type that contains a data() and size() method.
/// \tparam N The size of the array containing StringView instances.
/// \param data An array of StringView instances.
/// \returns A new shared_bytes instance which is the concatenation of all the StringView instances.
template <typename StringView, std::size_t N>
shared_bytes make_shared_bytes(std::array<StringView, N>&& data) {
   auto result = shared_bytes{};

   const std::size_t length =
         std::accumulate(data.begin(), data.end(), 0, [](std::size_t a, const StringView& b) { return a + b.size(); });

   if (length == 0) {
      return result;
   }

   result.m_size   = length;
   result.m_offset = details::aligned_size(length) - length;
   result.m_data   = std::shared_ptr<shared_bytes::underlying_type_t>{
      new shared_bytes::underlying_type_t[result.m_size + result.m_offset],
      std::default_delete<shared_bytes::underlying_type_t[]>()
   };
   char* chunk_ptr = result.m_data.get();
   for (const auto& view : data) {
      const char* const view_ptr = view.data();
      if (!view_ptr || !view.size()) {
         continue;
      }
      std::memcpy(chunk_ptr, view_ptr, view.size());
      chunk_ptr += view.size();
   }
   std::memset(chunk_ptr, 0, result.m_offset);

   return result;
}

template <typename T>
shared_bytes::shared_bytes(const T* data, size_t size)
    : m_size{ size * sizeof(T) }, m_offset{ eosio::session::details::aligned_size(m_size) - m_size }, m_data{ [&]() {
         if (!data || size == 0) {
            return std::shared_ptr<char>{};
         }

         // Make sure to instantiate a buffer that is aligned to the size of a uint64_t.
         auto  actual_size = m_size + m_offset;
         auto  result      = std::shared_ptr<underlying_type_t>{ new underlying_type_t[actual_size],
                                                           std::default_delete<underlying_type_t[]>() };
         auto* buffer      = result.get();
         std::memcpy(buffer, reinterpret_cast<const void*>(data), m_size);
         // Pad with zeros at the end.
         std::memset(buffer + m_size, 0, m_offset);
         return result;
      }() } {}

inline shared_bytes::shared_bytes(size_t size)
    : m_size{ size }, m_offset{ eosio::session::details::aligned_size(m_size) - m_size }, m_data{ [&]() {
         if (size == 0) {
            return std::shared_ptr<char>{};
         }

         // Make sure to instantiate a buffer that is aligned to the size of a uint64_t.
         auto actual_size = m_size + m_offset;
         auto result      = std::shared_ptr<underlying_type_t>{ new underlying_type_t[actual_size],
                                                           std::default_delete<underlying_type_t[]>() };
         std::memset(result.get(), 0, actual_size);
         return result;
      }() } {}

inline shared_bytes shared_bytes::next() const {
   auto buffer = std::vector<unsigned char>{ std::begin(*this), std::end(*this) };

   while (!buffer.empty()) {
      if (++buffer.back()) {
         break;
      }
      buffer.pop_back();
   }

   EOS_ASSERT(!buffer.empty(), eosio::chain::chain_exception, "shared_bytes::next() result buffer is empty");
   return eosio::session::shared_bytes(buffer.data(), buffer.size());
}

inline size_t            shared_bytes::size() const { return m_size; }
inline size_t            shared_bytes::aligned_size() const { return eosio::session::details::aligned_size(m_size); }
inline char*             shared_bytes::data() { return m_data.get(); }
inline const char* const shared_bytes::data() const { return m_data.get(); }

inline bool shared_bytes::empty() const { return m_size == 0; }

inline bool shared_bytes::operator==(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return true;
   }
   if (size() != other.size()) {
      return false;
   }
   return details::aligned_compare(m_data.get(), m_size, other.m_data.get(), other.m_size) == 0;
}

inline bool shared_bytes::operator!=(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return false;
   }
   if (size() != other.size()) {
      return true;
   }
   return details::aligned_compare(m_data.get(), m_size, other.m_data.get(), other.m_size) != 0;
}

inline bool shared_bytes::operator<(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return false;
   }
   return details::aligned_compare(m_data.get(), m_size, other.m_data.get(), other.m_size) < 0;
}

inline bool shared_bytes::operator<=(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return true;
   }
   return details::aligned_compare(m_data.get(), m_size, other.m_data.get(), other.m_size) <= 0;
}

inline bool shared_bytes::operator>(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return false;
   }
   return details::aligned_compare(m_data.get(), m_size, other.m_data.get(), other.m_size) > 0;
}

inline bool shared_bytes::operator>=(const shared_bytes& other) const {
   if (m_data.get() == other.m_data.get()) {
      return true;
   }
   return details::aligned_compare(m_data.get(), m_size, other.m_data.get(), other.m_size) >= 0;
}

inline bool shared_bytes::operator!() const { return *this == shared_bytes{}; }

inline shared_bytes::operator bool() const { return *this != shared_bytes{}; }

inline shared_bytes::underlying_type_t& shared_bytes::operator[](size_t index) { return m_data.get()[index]; }

inline shared_bytes::underlying_type_t shared_bytes::operator[](size_t index) const { return m_data.get()[index]; }

inline shared_bytes::iterator shared_bytes::begin() const {
   return iterator{ m_data.get(), 0, static_cast<int64_t>(m_size) - 1, m_size == 0 ? -1 : 0 };
}

inline shared_bytes::iterator shared_bytes::end() const {
   return iterator{ m_data.get(), 0, static_cast<int64_t>(m_size) - 1, -1 };
}

inline shared_bytes shared_bytes::truncate_key(const shared_bytes &key) {
   EOS_ASSERT(!key.empty(), eosio::chain::chain_exception, "chain_plugin::truncate_key() invalid key parameter: empty");

   return shared_bytes(key.data(), key.size() - 1);
}

inline std::ostream& operator<<(std::ostream& os, const shared_bytes& bytes) {
   for (const auto c : bytes) {
      std::cout << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << (0xFF & static_cast<int>(c));
   }
   return os;
}

template <typename Stream>
inline Stream& operator<<(Stream& ds, const shared_bytes& b) {
   fc::raw::pack( ds, b.size() );
   ds.write(b.data(), b.size());
   return ds;
}

template <typename Stream>
inline Stream& operator>>(Stream& ds, shared_bytes& b) {
   std::size_t sz;
   fc::raw::unpack( ds, sz );
   shared_bytes tmp = {sz};
   ds.read(tmp.data(), tmp.size());
   b = tmp;
   return ds;
}

inline shared_bytes shared_bytes::from_hex_string(const std::string& str) {
   if (str.empty()) {
      return shared_bytes{};
   }

   auto bytes = std::vector<char>{};
   bytes.reserve(str.size() / 2);
   for (size_t i = 0; i < str.size(); i += 2) {
      std::string hex    = str.substr(i, 2);
      int8_t      result = std::stoi(hex, 0, 16);
      char        c{ 0 };
      std::memcpy(&c, &result, 1);
      bytes.push_back(c);
   }

   return shared_bytes{ bytes.data(), bytes.size() };
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>::shared_bytes_iterator(char* buffer, int64_t begin, int64_t end,
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
   auto left_index  = m_index == -1 ? m_end + 1 : m_index;
   auto right_index = other.m_index == -1 ? other.m_end + 1 : other.m_index;
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
