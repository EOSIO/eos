#pragma once

#include <chainbase/chainbase.hpp>
#include <cyberway/chaindb/index_object.hpp>

namespace fc {
    class variant;

    template<typename T> inline void to_variant(const chainbase::oid<T>&, variant&);
    template<typename T> inline void from_variant(const variant&, chainbase::oid<T>&);
    template<typename T> inline void from_variant(const variant&, chainbase::oid<T>&);

    template<typename T> inline void to_variant(const cyberway::chaindb::oid<T>&, variant&);
    template<typename T> inline void from_variant(const variant&, cyberway::chaindb::oid<T>&);
    template<typename T> inline void from_variant(const variant&, cyberway::chaindb::oid<T>&);

    inline void to_variant(const std::vector<bool>& t, variant& vo);

    namespace raw {
        template<typename Stream, typename T> inline void pack(Stream& s, const chainbase::oid<T>& o);
        template<typename Stream, typename T> inline void unpack(Stream& s, chainbase::oid<T>& o);

        template<typename Stream, typename T> inline void pack(Stream& s, const cyberway::chaindb::oid<T>& o);
        template<typename Stream, typename T> inline void unpack(Stream& s, cyberway::chaindb::oid<T>& o);

        template<typename Stream> inline void unpack(Stream& s, std::vector<bool>& value);
    } // namespace raw
} // namespace fc

#include <fc/io/raw.hpp>

namespace fc {

    inline void to_variant(const std::vector<bool>& vect, variant& v) {
        std::vector<variant> vars(vect.size());
        size_t i = 0;
        for (bool b : vect) {
            vars[i++] = variant(b);
        }
        v = std::move(vars);
    }

    template<typename T> void to_variant(const cyberway::chaindb::oid<T>& oid, variant& v) {
        v = variant(oid._id);
    }

    template<typename T> void from_variant(const variant& v, cyberway::chaindb::oid<T>& oid) {
        from_variant(v, oid._id);
    }

    namespace raw {
        template<typename Stream, typename T> inline void pack(Stream& s, const chainbase::oid<T>& o) {
            fc::raw::pack(s, o._id);
        }

        template<typename Stream, typename T> inline void unpack(Stream& s, chainbase::oid<T>& o) {
            fc::raw::unpack(s, o._id);
        }

        template<typename Stream, typename T> inline void pack(Stream& s, const cyberway::chaindb::oid<T>& o) {
            fc::raw::pack(s, o._id);
        }

        template<typename Stream, typename T> inline void unpack(Stream& s, cyberway::chaindb::oid<T>& o) {
            fc::raw::unpack(s, o._id);
        }

        /** std::vector<bool> has custom implementation and pack bools as bits */
        template<typename Stream> inline void unpack(Stream& s, std::vector<bool>& value) {
            // TODO: can serialize as bitmap to save up to 8x storage (implement proper pack)
            unsigned_int size;
            fc::raw::unpack(s, size);
            FC_ASSERT(size.value <= MAX_NUM_ARRAY_ELEMENTS);
            value.resize(size.value);
            auto itr = value.begin();
            auto end = value.end();
            while (itr != end) {
                bool b = true;
                fc::raw::unpack(s, b);
                *itr = b;
                ++itr;
            }
        }
    } // namespace raw
} // namespace fc
