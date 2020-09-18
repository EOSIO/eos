#pragma once

#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>

namespace eosio::chain_kv {

// \brief An immutable type to represent a pointer and a length.
//
class shared_bytes {
   public:
      using iterator = const int8_t*;
      using const_iterator = const iterator;
      template <typename T>
      inline shared_bytes(const T* d, std::size_t s)
         : m_data( new int8_t[sizeof(T)*s], std::default_delete<int8_t[]>() ), m_size(sizeof(T)*s) {
         std::memcpy(m_data.get(), reinterpret_cast<const void*>(d), s);
      }
      shared_bytes() = default;
      shared_bytes(const shared_bytes& b) = default;
      shared_bytes(shared_bytes&& b)      = default;

      shared_bytes& operator=(const shared_bytes& b) = default;
      shared_bytes& operator=(shared_bytes&& b) = default;

      inline const int8_t* data() const { return m_data.get(); }
      inline size_t        size() const { return m_size; }

      inline int8_t& operator[](size_t index) { return m_data.get()[index]; }
      inline int8_t operator[](size_t index) const { return data()[index]; }
      inline int8_t& at(size_t index) {
        if (index >= m_size)
           throw std::runtime_error("at index out of range");
        return (*this)[index];
      }
      inline int8_t at(size_t index) const { return const_cast<shared_bytes*>(this)->at(index); }

      inline bool operator==(const shared_bytes& other) const { return size() == other.size() && std::memcmp(data(), other.data(), size()) == 0; }
      inline bool operator!=(const shared_bytes& other) const { return !((*this) == other); }

      inline bool operator<(const shared_bytes& other) const {
         int32_t cmp = std::memcmp(data(), other.data(), std::min(size(), other.size()));
         return (cmp == 0 && size() < other.size()) || cmp < 1;
      }
      inline bool operator<=(const shared_bytes& other) const { return !((*this) > other); }

      inline bool operator>(const shared_bytes& other) const {
         int32_t cmp = std::memcmp(data(), other.data(), std::min(size(), other.size()));
         return (cmp == 0 && size() > other.size()) || cmp > 1;
      }
      inline bool operator>=(const shared_bytes& other) const { return !((*this) < other); }

      inline operator bool() const { return static_cast<bool>(m_data); }
      inline bool operator!() const { return !static_cast<bool>((*this)); }

      inline iterator begin() const { return data(); }
      inline iterator end() const { return data() + size(); }

      static inline const auto& invalid() {
         static shared_bytes invalid_token = {(int8_t*)nullptr, std::numeric_limits<size_t>::max()};
         return invalid_token;
      }

   private:
      std::shared_ptr<int8_t> m_data;
      size_t                  m_size{ 0 };
};

} // namespace eosio::chain_kv

namespace std {
  template <>
  struct hash<eosio::chain_kv::shared_bytes> {
    std::size_t operator()(const eosio::chain_kv::shared_bytes& k) const {
       return hash<std::string_view>()({(const char*)k.data(), k.size()});
    }
  };
}
