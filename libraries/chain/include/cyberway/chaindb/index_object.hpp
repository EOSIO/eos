#pragma once

#include <ostream>

#define OBJECT_CTOR(NAME) \
    NAME() = delete; \
    public: \
    template<typename Constructor> \
    NAME(Constructor&& c, int) \
    { c(*this); }

namespace cyberway { namespace chaindb {

    template<typename T> class oid {
    public:
        oid(uint64_t i = 0)
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

        friend bool operator != (const oid& a, const oid& b) {
            return a._id != b._id;
        }

        friend std::ostream& operator<<(std::ostream& s, const oid& id) {
            s << boost::core::demangle(typeid(oid<T>).name()) << '(' << id._id << ')'; return s;
        }

        uint64_t _id = 0;
    }; // class oid

    template<uint16_t TypeNumber, typename Derived> struct object
    {
        typedef oid<Derived> id_type;
        static constexpr uint16_t type_id = TypeNumber;
    }; // struct object

} } // namespace cyberway::chain