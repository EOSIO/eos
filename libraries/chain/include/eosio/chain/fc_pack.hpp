#pragma once

#include <chainbase/chainbase.hpp>

namespace fc {
    class variant;

    template<typename T>
    inline void to_variant(const chainbase::oid<T>& var,  variant& vo);

    template<typename T>
    inline void from_variant(const variant& var,  chainbase::oid<T>& vo);

    namespace raw {
        template<typename Stream, typename T>
        inline void pack(Stream& s, const chainbase::oid<T>& o);

        template<typename Stream, typename T>
        inline void unpack(Stream& s, chainbase::oid<T>& o);

        template<typename Stream>
        inline void unpack(Stream& s, typename std::vector<bool>::reference& o);
    } // namespace raw
} // namespace fc

#include <fc/io/raw.hpp>

namespace fc {
    template<typename T>
    inline void to_variant(const chainbase::oid<T>& var,  variant& vo) {
        vo = var._id;
    }

    template<typename T>
    inline void from_variant(const variant& var,  chainbase::oid<T>& vo) {
        vo._id = var.as_int64();
    }

    namespace raw {
        template<typename Stream, typename T>
        inline void pack(Stream& s, const chainbase::oid<T>& o) {
            fc::raw::pack(s, o._id);
        }

        template<typename Stream, typename T>
        inline void unpack(Stream& s, chainbase::oid<T>& o) {
            fc::raw::unpack(s, o._id);
        }

        /** std::vector<bool> has custom implementation and pack bools as bits */
        template<typename Stream>
        inline void unpack(Stream& s, typename std::vector<bool>::reference& o) {
            bool v;
            fc::raw::unpack(s, v);
            o = v;
        }
    } // namespace raw
} // namespace fc
