#pragma once

#include <ostream>

namespace cyberway { namespace chaindb {
    using primary_key_t = uint64_t;
} }

#define OBJECT_CTOR(NAME) \
       NAME() = delete; \
    public: \
       template<typename Constructor> \
       NAME(Constructor&& c, int) { \
          c(*this); \
       }

#define CHAINDB_OBJECT_ID_CTOR(NAME) \
       NAME() = delete; \
    public: \
       template<typename Constructor> \
       NAME(cyberway::chaindb::primary_key_t v, Constructor&& c) { \
          id._id = v; \
          c(*this); \
       } \
       cyberway::chaindb::primary_key_t pk() const { \
          return id._id; \
       }

#define CHAINDB_OBJECT_CTOR(NAME, PK) \
       NAME() = delete; \
    public: \
       template<typename Constructor> \
       NAME(cyberway::chaindb::primary_key_t v, Constructor&& c) { \
           PK = v; \
           c(*this); \
       } \
       cyberway::chaindb::primary_key_t pk() const { \
           return PK; \
       }

namespace cyberway { namespace chaindb {

    template<typename Object> class oid {
    public:
        oid(primary_key_t i = 0)
        : _id(i) {
        }

        oid& operator++() {
            ++_id;
            return *this;
        }

        friend bool operator < (const oid& a, const oid& b) {
            return a._id < b._id;
        }

        friend bool operator > (const oid& a, const oid& b) {
            return a._id > b._id;
        }

        friend bool operator == (const oid& a, const oid& b) {
            return a._id == b._id;
        }

        friend bool operator == (const oid& a, const int b) {
            return a._id == primary_key_t(b);
        }

        friend bool operator != (const oid& a, const oid& b) {
            return a._id != b._id;
        }

        friend bool operator != (const oid& a, const primary_key_t b) {
            return a._id != b;
        }

        friend std::ostream& operator<<(std::ostream& s, const oid& id) {
            s << boost::core::demangle(typeid(oid).name()) << '(' << id._id << ')'; return s;
        }

        operator primary_key_t() const {
            return _id;
        }

        primary_key_t _id = 0;
    }; // class oid

    template<uint16_t TypeNumber, typename Derived> struct object {
        typedef oid<Derived> id_type;
        static constexpr uint16_t type_id = TypeNumber;
    }; // struct object

} } // namespace cyberway::chain