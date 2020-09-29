#pragma once

#include <algorithm>
#include <memory>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <fc/crypto/base64.hpp>

#include <eosio/chain/exceptions.hpp>

namespace eosio::session {

class shared_bytes;

// \brief An immutable type to represent a pointer and a length.
//
class shared_bytes {
   public:
      shared_bytes()                      = default;
      shared_bytes(const shared_bytes& b) = default;
      shared_bytes(shared_bytes&& b)      = default;
      ~shared_bytes()                     = default;

      template <typename T>
      inline shared_bytes(const T* data, size_t sz)
         : m_data(create_bytes(reinterpret_cast<const char*>(data), sz*sizeof(T))), m_size(sz*sizeof(T)) {}
      inline shared_bytes(const std::vector<char>& v)
         : m_data(create_bytes(v.data(), v.size())), m_size(v.size()) {}
      inline shared_bytes(std::string_view sv)
         : m_data(create_bytes(sv.data(), sv.size())), m_size(sv.size()) {}
      inline shared_bytes(const std::string& s)
         : m_data(create_bytes(s.c_str(), s.size())), m_size(s.size()) {}

      shared_bytes& operator=(const shared_bytes& b) = default;
      shared_bytes& operator=(shared_bytes&& b) = default;

      inline shared_bytes& operator=(const std::vector<char>& v) {
         m_data = create_bytes(v.data(), v.size());
         m_size = v.size();
         return *this;
      }
      inline shared_bytes& operator=(std::string_view sv) {
         m_data = create_bytes(sv.data(), sv.size());
         m_size = sv.size();
         return *this;
      }
      inline shared_bytes& operator=(const std::string& s) {
         m_data = create_bytes(s.data(), s.size());
         m_size = s.size();
         return *this;
      }

      using iterator       = const char*;
      using const_iterator = const iterator;

      inline const char* data() const { return m_data.get(); }
      inline size_t      size() const { return m_size; }

      char& operator[](size_t index) { return m_data.get()[index]; }
      char operator[](size_t index) const { return m_data.get()[index]; }
      char& at(size_t index) {
         EOS_ASSERT(index < m_size, eosio::chain::chain_exception, "shared_bytes index out-of-bounds");
         EOS_ASSERT(m_data, eosio::chain::chain_exception, "shared_bytes data is null");
         return (*this)[index];
      }
      char at(size_t index) const {
         EOS_ASSERT(index < m_size, eosio::chain::chain_exception, "shared_bytes index out-of-bounds");
         EOS_ASSERT(m_data, eosio::chain::chain_exception, "shared_bytes data is null");
         return (*this)[index];
      }

      bool operator==(const shared_bytes& other) const {
         return m_size == other.m_size &&
            (((m_data && other.m_data) && std::memcmp(data(), other.data(), size()) == 0) ||
             (!m_data && !other.m_data));
      }
      inline bool operator!=(const shared_bytes& other) const { return !((*this) == other); }

      inline bool operator<(const shared_bytes& other) const {
         return ((m_data && other.m_data) && std::memcmp(data(), other.data(), std::min(size(), other.size())) < 0) || (!m_data);
      }
      inline bool operator>(const shared_bytes& other) const {
         return ((m_data && other.m_data) && std::memcmp(data(), other.data(), std::min(size(), other.size())) > 0) || (!m_data);
      }
      inline bool operator>=(const shared_bytes& other) const { return (*this) < other; }
      inline bool operator<=(const shared_bytes& other) const { return (*this) > other; }

      operator bool() const { return static_cast<bool>(m_data); }
      bool operator!() const { return !static_cast<bool>(m_data); }

      // pop n bytes from shared_bytes and return a new instance
      inline shared_bytes pop(size_t n=1) const {
         EOS_ASSERT(n < m_size, eosio::chain::chain_exception, "shared_bytes pop N too large");
         return {m_data.get(), m_size - n};
      }

      inline shared_bytes operator++(int) const { return pop(); }

      inline iterator begin() const { return data(); }
      inline iterator end() const   { return data() + size(); }

      inline static const shared_bytes invalid() { return {}; }

   private:
      inline std::shared_ptr<char> create_bytes(const char* p, size_t s) {
         if (p) {
            std::shared_ptr<char> ret = {new char[s], std::default_delete<char[]>()};
            std::memcpy(ret.get(), p, s);
            return ret;
         } else {
            return std::shared_ptr<char>{};
         }
      }
      std::shared_ptr<char> m_data = nullptr;
      size_t                m_size = 0;
};

inline std::ostream& operator<<(std::ostream& os, const shared_bytes& bytes) {
    std::cout << fc::base64_encode({bytes.data(), bytes.size()});
    return os;
}

} // namespace eosio::session

namespace std {
template <>
struct hash<eosio::session::shared_bytes> {
   size_t operator()(const eosio::session::shared_bytes& b) const {
      return std::hash<std::string_view>{}({ reinterpret_cast<const char*>(b.data()), b.size() });
   }
};
} // namespace std
