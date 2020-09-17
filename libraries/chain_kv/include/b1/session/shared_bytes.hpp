#pragma once

#include <algorithm>
#include <memory>
#include <string_view>

#include <softfloat.hpp>

#include <boost/pool/pool.hpp>

#include <session/shared_bytes_fwd_decl.hpp>

namespace eosio::session {

// \brief An immutable type to represent a pointer and a length.
//
class shared_bytes final {
 public:
   template <typename T>
   friend shared_bytes make_shared_bytes(const T* data, size_t length);

   friend shared_bytes make_shared_bytes(const int8_t* data, size_t length);

   template <typename T>
   friend shared_bytes make_shared_bytes_view(const T* data, size_t length);

   template <typename Iterator_traits>
   class shared_bytes_iterator final {
    public:
      friend class shared_bytes;

      using difference_type   = typename Iterator_traits::difference_type;
      using value_type        = typename Iterator_traits::value_type;
      using pointer           = typename Iterator_traits::pointer;
      using reference         = typename Iterator_traits::reference;
      using iterator_category = typename Iterator_traits::iterator_category;

    public:
      shared_bytes_iterator()                                = default;
      shared_bytes_iterator(const shared_bytes_iterator& it) = default;
      shared_bytes_iterator(shared_bytes_iterator&&)         = default;

      shared_bytes_iterator& operator=(const shared_bytes_iterator& it) = default;
      shared_bytes_iterator& operator=(shared_bytes_iterator&&) = default;

      shared_bytes_iterator& operator++();
      shared_bytes_iterator  operator++(int);
      shared_bytes_iterator& operator--();
      shared_bytes_iterator  operator--(int);
      value_type             operator*() const;
      value_type             operator->() const;
      bool                   operator==(const shared_bytes_iterator& other) const;
      bool                   operator!=(const shared_bytes_iterator& other) const;
      value_type             operator[](difference_type index) const;
      shared_bytes_iterator& operator+=(difference_type n);
      shared_bytes_iterator& operator-=(difference_type n);
      shared_bytes_iterator  operator+(difference_type n) const;
      shared_bytes_iterator  operator-(difference_type n) const;
      difference_type        operator-(const shared_bytes_iterator& rhs) const;
      difference_type        operator+(const shared_bytes_iterator& rhs) const;

    private:
      shared_bytes* m_bytes;
      int64_t       m_index{ 0 };
   };

   struct iterator_traits final {
      using difference_type   = int64_t;
      using value_type        = int8_t;
      using pointer           = value_type*;
      using reference         = value_type&;
      using iterator_category = std::random_access_iterator_tag;
   };
   using iterator = shared_bytes_iterator<iterator_traits>;

 public:
   shared_bytes(const shared_bytes& b) = default;
   shared_bytes(shared_bytes&& b)      = default;
   ~shared_bytes()                     = default;

   shared_bytes& operator=(const shared_bytes& b) = default;
   shared_bytes& operator=(shared_bytes&& b) = default;

   const int8_t* const data() const;
   size_t              size() const;

   int8_t operator[](size_t index) const;

   bool operator==(const shared_bytes& other) const;
   bool operator!=(const shared_bytes& other) const;

   bool operator!() const;
        operator bool() const;

   iterator begin() const;
   iterator end() const;

   static const shared_bytes invalid;

 private:
   shared_bytes() = default;

 private:
   std::shared_ptr<int8_t> m_data;
   size_t                  m_size{ 0 };
};

// \brief Creates a new shared_bytes instance with the given pointer and length.
//
// \tparam T The type stored in the array.
// \\param data A pointer to an array of items to store in a shared_bytes instance. \param
// length The number of items in the array.
template <typename T>
shared_bytes make_shared_bytes(const T* data, size_t length) {
   return make_shared_bytes(reinterpret_cast<const int8_t*>(data), length * sizeof(T));
}

inline shared_bytes make_shared_bytes(const int8_t* data, size_t length) {
   auto result = shared_bytes{};

   if (!data || length == 0) {
      return result;
   }

   auto* chunk = std::allocator<int8_t>{}.allocate(length);
   memcpy(chunk, data, length);

   auto deleter  = [&](auto* chunk) { std::allocator<int8_t>{}.deallocate(chunk, length); };
   result.m_data = std::shared_ptr<int8_t>(chunk, deleter);
   result.m_size = length;

   return result;
}

template <>
inline shared_bytes make_shared_bytes_view(const int8_t* data, size_t length) {
   auto result = shared_bytes{};

   auto deleter  = [&](auto* chunk) {};
   result.m_data = std::shared_ptr<int8_t>(const_cast<int8_t*>(data), deleter);
   result.m_size = length;

   return result;
}

// \brief Creates a new shared_bytes instance with the given pointer and length.
//
// This returns a shared_bytes "view" instance, in that the resulting shared_bytes instance does not manage
// the given memory.
//
// \tparam T The type stored in the array.
// \param data A pointer to an array of items to store in a shared_bytes instance.
// \param length The number of items in the array.
// \warning The resulting shared_bytes instance does NOT take ownership of the given data pointer.
// \warning The data pointer must remain in scope for the lifetime of the given shared_bytes instance.
template <typename T>
shared_bytes make_shared_bytes_view(const T* data, size_t length) {
   return make_shared_bytes_view(reinterpret_cast<const int8_t*>(data), length * sizeof(T));
}

inline const shared_bytes shared_bytes::invalid{};

inline const int8_t* const shared_bytes::data() const { return m_data ? &(m_data.get()[0]) : nullptr; }

inline size_t shared_bytes::size() const { return m_size; }

inline int8_t shared_bytes::operator[](size_t index) const {
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
      return memcmp(m_data.get(), other.m_data.get(), size()) == 0 ? true : false;
   }

   return false;
}

inline bool shared_bytes::operator!=(const shared_bytes& other) const { return !(*this == other); }

inline bool shared_bytes::operator!() const { return *this == shared_bytes::invalid; }

inline shared_bytes::operator bool() const { return *this != shared_bytes::invalid; }

inline shared_bytes::iterator shared_bytes::begin() const {
   auto result    = iterator{};
   result.m_bytes = const_cast<shared_bytes*>(this);
   result.m_index = 0;
   return result;
}

inline shared_bytes::iterator shared_bytes::end() const {
   auto result    = iterator{};
   result.m_bytes = const_cast<shared_bytes*>(this);
   result.m_index = size();
   return result;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>&
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator++() {
   if (m_index <= m_bytes->m_size) {
      ++m_index;
   }
   return *this;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator++(int) {
   auto result = *this;
   if (m_index <= m_bytes->m_size) {
      ++m_index;
   }
   return result;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>&
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator--() {
   if (m_index > 0) {
      --m_index;
   }
   return *this;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator--(int) {
   auto result = *this;
   if (m_index > 0) {
      --m_index;
   }
   return result;
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::value_type
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator[](difference_type index) const {
   if (index >= m_bytes->size()) {
      return 0;
   }
   return m_bytes->data()[index];
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>&
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator+=(difference_type n) {
   m_index = std::min(static_cast<int64_t>(m_bytes->size()), m_index - n);
   return *this;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>&
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator-=(difference_type n) {
   m_index = std::max(0, m_index - n);
   return *this;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator+(difference_type n) const {
   auto it    = shared_bytes_iterator<Iterator_traits>{};
   it.m_bytes = m_bytes;
   it.m_index = std::min(static_cast<int64_t>(m_bytes->size()), m_index - n);
   return it;
}

template <typename Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator-(difference_type n) const {
   auto it    = shared_bytes_iterator<Iterator_traits>{};
   it.m_bytes = m_bytes;
   it.m_index = std::max(0, m_index - n);
   return it;
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::difference_type
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator-(const shared_bytes_iterator& rhs) const {
   return m_index - rhs.m_index;
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::difference_type
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator+(const shared_bytes_iterator& rhs) const {
   return m_index + rhs.m_index;
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::value_type
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator*() const {
   if (m_index > m_bytes->m_size) {
      return 0;
   }
   return m_bytes->m_data.get()[m_index];
}

template <typename Iterator_traits>
typename shared_bytes::shared_bytes_iterator<Iterator_traits>::value_type
shared_bytes::shared_bytes_iterator<Iterator_traits>::operator->() const {
   if (m_index > m_bytes->m_size) {
      return 0;
   }
   return m_bytes->m_data.get()[m_index];
}

template <typename Iterator_traits>
bool shared_bytes::shared_bytes_iterator<Iterator_traits>::operator==(const shared_bytes_iterator& other) const {
   return m_bytes == other.m_bytes && m_index == other.m_index;
}

template <typename Iterator_traits>
bool shared_bytes::shared_bytes_iterator<Iterator_traits>::operator!=(const shared_bytes_iterator& other) const {
   return !(*this == other);
}

} // namespace eosio::session

namespace std {

template <>
struct less<eosio::session::shared_bytes> final {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      if (lhs != eosio::session::shared_bytes::invalid && rhs != eosio::session::shared_bytes::invalid) {
         return std::string_view{ reinterpret_cast<const char*>(lhs.data()), lhs.size() } <
                std::string_view{ reinterpret_cast<const char*>(rhs.data()), rhs.size() };
      }
      if (lhs == eosio::session::shared_bytes::invalid) {
         return false;
      }
      return true;
   };
};

template <>
struct greater<eosio::session::shared_bytes> final {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      if (lhs != eosio::session::shared_bytes::invalid && rhs != eosio::session::shared_bytes::invalid) {
         return std::string_view{ reinterpret_cast<const char*>(lhs.data()), lhs.size() } >
                std::string_view{ reinterpret_cast<const char*>(rhs.data()), rhs.size() };
      }
      if (lhs == eosio::session::shared_bytes::invalid) {
         return false;
      }
      return true;
   };
};

template <>
struct hash<eosio::session::shared_bytes> final {
   size_t operator()(const eosio::session::shared_bytes& b) const {
      return std::hash<std::string_view>{}({ reinterpret_cast<const char*>(b.data()), b.size() });
   }
};

template <>
struct equal_to<eosio::session::shared_bytes> final {
   bool operator()(const eosio::session::shared_bytes& lhs, const eosio::session::shared_bytes& rhs) const {
      return lhs == rhs;
   }
};

} // namespace std
