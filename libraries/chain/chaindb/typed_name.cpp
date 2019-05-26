#include <cyberway/chaindb/typed_name.hpp>
#include <cyberway/chaindb/exception.hpp>
#include <cyberway/chaindb/common.hpp>
#include <cyberway/chaindb/names.hpp>
#include <cyberway/chaindb/table_info.hpp>

#include <eosio/chain/symbol.hpp>
#include <eosio/chain/name.hpp>

#include <assert.h>

#define TYPED_ASSERT(expr, FORMAT, ...) CYBERWAY_ASSERT(expr, invalid_typed_name_exception, FORMAT, __VA_ARGS__)
#define TYPED_THROW(FORMAT, ...) CYBERWAY_THROW(invalid_typed_name_exception, FORMAT, __VA_ARGS__)

namespace cyberway { namespace chaindb {

    using eosio::chain::name;
    using eosio::chain::symbol;
    using eosio::chain::symbol_info;

    typed_name::typed_name(const typed_name::value_kind kind, const typed_name::value_type value)
    : kind_(kind),
      value_(value) {
    }

    typed_name typed_name::create(const typed_name::value_kind kind, const typed_name::value_type value) {
        TYPED_ASSERT(is_valid(kind, value), "Invalid typed name");
        return typed_name(kind, value);
    }

    typed_name::value_kind typed_name::kind_from_string(const string& kind) {
        constexpr auto symbol_len = (sizeof("symbol") / sizeof(("symbol")[0])) - 1;

        switch (kind.front()) {
            case 'i':
                assert(kind == "int64");
                return Int64;

            case 'u':
                assert(kind == "uint64");
                return Uint64;

            case 'n':
                assert(kind == "name");
                return Name;

            case 's':
                if (kind.size() == symbol_len) {
                    assert(kind == "symbol");
                    return Symbol;
                } else {
                    assert(kind == "symbol_code");
                    return SymbolCode;
                }
                break;

            default:
                break;
        }

        return Unknown;
    }

    typed_name typed_name::from_value(const value_kind) {
        return create(Unknown, 0);
    }

    typed_name typed_name::from_value(const value_kind kind, const int64_t value) {
        switch (kind) {
            case Int64:
                return create(kind, static_cast<value_type>(value));

            case Uint64:
            case Name:
            case Symbol:
            case SymbolCode:
            case Unknown:
            default:
                break;
        }

        TYPED_THROW("Bad kind of typed name on initialize from int64");
    }

    typed_name typed_name::from_value(const value_kind kind, const uint64_t value) {
        switch (kind) {
            case Uint64:
                return create(kind, value);

            case Int64:
            case Name:
            case Symbol:
            case SymbolCode:
            case Unknown:
            default:
                break;
        }

        TYPED_THROW("Bad kind of typed name on initialize from uint64");
    }

    typed_name typed_name::from_value(const value_kind kind, const string& value) {
        switch (kind) {
            case Name:
                return create(kind, name(value).value);

            case SymbolCode:
                return create(kind, symbol(0, value.c_str()).to_symbol_code());

            case Symbol:
                return create(kind, symbol::from_string(value).value());

            case Int64:
            case Uint64:
            case Unknown:
            default:
                break;
        }

        TYPED_THROW("Bad kind of typed name on initialize from string");
    }

    typed_name typed_name::from_string(const value_kind kind, const string& value) {
        switch (kind) {
            case Int64:
                return from_value(kind, fc::to_int64(value));

            case Uint64:
                return from_value(kind, fc::to_uint64(value));

            case Name:
            case SymbolCode:
            case Symbol:
            case Unknown:
            default:
                break;
        }

        return from_value(kind, value);
    }

    bool typed_name::is_valid(const value_kind kind , const value_type value) {
        switch (kind) {
            case Int64:
            case Uint64:
            case Name:
                return true; // all range

            case Symbol:
                return symbol(value).valid(); // 'A'-'Z' and precision

            case SymbolCode:
                return symbol(value << 8).valid(); // 'A'-'Z'

            case Unknown:
            default:
                break;
        }
        return false;
    }

    bool typed_name::is_valid() const {
       return is_valid(kind_, value_);
    }

    bool typed_name::is_valid_kind(const string& kind) {
        return
            kind == "int64" ||
            kind == "uint64" ||
            kind == "name"  ||
            kind == "symbol_code" ||
            kind == "symbol";
    }

    string typed_name::to_string() const {
        switch (kind_) {
            case Int64:
                return std::to_string(static_cast<int64_t>(value_));

            case Uint64:
                return std::to_string(static_cast<uint64_t>(value_));

            case Name:
                return name(value_).to_string();

            case Symbol:
                return symbol(value_).to_string();

            case SymbolCode:
                return symbol(value_ << 8).name();

            case Unknown:
            default:
                break;
        }

        TYPED_THROW("Invalid type of the typed name on converting to string");
    }

    variant typed_name::to_variant() const {
        switch (kind_) {
            case Int64:
                return variant(static_cast<int64_t>(value_));

            case Uint64:
                return variant(value_);

            case Name:
                return variant(name(value_));

            case Symbol:
                return variant(symbol_info(value_));

            case SymbolCode:
                return variant(symbol(value_ << 8).name());

            case Unknown:
            default:
                break;
        }

        CYBERWAY_THROW(invalid_typed_name_exception, "Invalid type of the typed name on converting to variant");
    }

    ///------------------------------------

    primary_key::primary_key(typed_name&& src)
    : typed_name(src.kind(), src.value()) {
    }

    typed_name::value_kind primary_key::kind_from_table(const table_info& info) {
        assert(info.pk_order);
        assert(!info.pk_order->path.empty());

        return kind_from_string(info.pk_order->type);
    }

    primary_key primary_key::from_string(const table_info& info, const string& value) {
        return typed_name::from_string(kind_from_table(info), value);
    }

    primary_key primary_key::from_table(const table_info& info, const value_type value) {
        assert(info.pk_order);
        assert(!info.pk_order->type.empty());

        return create(kind_from_string(info.pk_order->type), value);
    }

    variant primary_key::to_variant(const table_info& info) const {
        assert(info.pk_order);
        assert(!info.pk_order->path.empty());

        auto& pk_path = info.pk_order->path;
        auto  value   = typed_name::to_variant();

        for (auto itr = pk_path.rbegin(), etr = pk_path.rend(); etr != itr; ++itr) {
            value = variant(fc::mutable_variant_object(*itr, value));
        }

        return value;
    }

    variant primary_key::to_variant(const table_info& info, const value_type value) {
        return from_table(info, value).to_variant(info);
    }

    ///------------------------------------

    scope_name::scope_name(typed_name&& src)
    : typed_name(src.kind(), src.value()) {
    }

    typed_name::value_kind scope_name::kind_from_table(const table_info& info) {
        assert(info.table);

        auto& scope_type = info.table->scope_type;
        // by default as eosio::chain::name
        if (scope_type.empty()) {
            return typed_name::Name;
        }
        return typed_name::kind_from_string(scope_type);
    }

    scope_name scope_name::from_string(const table_info& info, const string& value) {
        return typed_name::from_string(kind_from_table(info), value);
    }

    scope_name scope_name::from_table(const table_info& info) {
        return create(kind_from_table(info), info.scope);
    }

    bool scope_name::is_valid_kind(const string& kind) {
        return kind.empty() || typed_name::is_valid_kind(kind);
    }

} } // namespace cyberway::chaindb