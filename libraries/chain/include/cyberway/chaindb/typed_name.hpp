#pragma once

#include <string>

#include <cyberway/chaindb/exception.hpp>

namespace fc {
    class variant;
}

namespace cyberway { namespace chaindb {

    using std::string;
    using fc::variant;

    struct table_info;

    struct typed_name {
        using value_type = uint64_t;

        enum value_kind {
            Unknown,
            Int64,
            Uint64,
            Name,
            Symbol,
            SymbolCode
        };

        typed_name(typed_name&&) = default;

        value_type value() const { return value_; }
        value_kind kind()  const { return kind_;  }

        bool is_valid() const;

        static bool is_valid_kind(const string&);

        string  to_string()  const;
        variant to_variant() const;

        template <typename DataStream> friend DataStream& operator<<(DataStream& ds, const typed_name& s) {
            return ds << s.to_string();
        }

    protected:
        typed_name(value_kind, value_type);

        static bool is_valid(value_kind, value_type);

        static typed_name create(value_kind, value_type);

        static value_kind kind_from_string(const string&);

        static typed_name from_value(value_kind);
        static typed_name from_value(value_kind, uint64_t);
        static typed_name from_value(value_kind, int64_t);
        static typed_name from_value(value_kind, const string&);

        static typed_name from_string(value_kind, const string&);

    private:
        value_kind kind_  = Unknown;
        value_type value_ = 0;
    }; // struct typed_name

    struct primary_key final: public typed_name {
        enum : value_type {
            Unset = value_type(-2),
            End   = value_type(-1),
        }; // constants ...

        primary_key(primary_key&&) = default;

        static bool is_good(const value_type v) {
            return v != Unset && v != End;
        }

        bool is_good() const {
            return is_good(value());
        }

        template<typename... Args>
        static primary_key from_value(const table_info& info, Args&&... args) {
            return typed_name::from_value(kind_from_table(info), std::forward<Args>(args)...);
        }

        static primary_key from_string(const table_info&, const string&);

        static primary_key from_table(const table_info&, value_type);

        variant to_variant(const table_info&) const;
        static variant to_variant(const table_info& info, value_type value);

    private:
        primary_key(typed_name&&);
        using typed_name::typed_name;

        static value_kind kind_from_table(const table_info&);
    }; // struct primary_key

    struct scope_name final: public typed_name {
        scope_name(scope_name&&) = default;

        template<typename... Args>
        static scope_name from_value(const table_info& info, Args&&... args) {
            return typed_name::from_value(kind_from_table(info), std::forward<Args>(args)...);
        }

        static scope_name from_string(const table_info&, const string&);

        static scope_name from_table(const table_info&);

        static bool is_valid_kind(const string&);

    private:
        scope_name(typed_name&&);
        using typed_name::typed_name;

        static value_kind kind_from_table(const table_info&);
    }; // struct scope_name

} } // namespace cyberway::chaindb