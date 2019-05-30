#include <bsoncxx/builder/basic/document.hpp>

#include <fc/variant_object.hpp>

#include <cyberway/chaindb/mongo_driver_utils.hpp>
#include <cyberway/chaindb/mongo_asset_converter.hpp>
#include <cyberway/chaindb/names.hpp>

#include <eosio/chain/asset.hpp>

namespace cyberway { namespace chaindb {

    mongo_asset_converter::mongo_asset_converter(const bsoncxx::document::view& src) {
        convert(src);
    }

    void mongo_asset_converter::convert(const bsoncxx::document::view& src) try {
        auto tmp_kind   = Unknown;
        auto amount_itr = src.begin();
        auto decs_itr   = amount_itr;

        if (!amount_itr->key().compare(names::amount_field)) {
            tmp_kind = Asset;
            ++decs_itr;
        }

        if (!decs_itr->key().compare(names::decs_field)) {
            if (Unknown == tmp_kind) {
                tmp_kind = Symbol;
                amount_itr = src.end();
            }
        } else {
            return; // !`amount` && !`decs`
        }

        auto sym_itr = decs_itr; ++sym_itr;
        if (sym_itr->key().compare(names::sym_field)) {
            return;
        }

        auto end_itr = sym_itr; ++end_itr;
        if (end_itr != src.end()) {
            return; // external fields
        }

        auto decs = from_decimal128(decs_itr->get_decimal128());
        auto sym  = sym_itr->get_utf8().value.to_string();

        switch (tmp_kind) {
            case Asset: {
                auto amount = amount_itr->get_int64().value;
                convert_to_asset(amount, decs, sym);
                break;
            }

            case Symbol:
                convert_to_symbol(decs, sym);
                break;

            case Unknown:
            default:
                break;
        }
    } catch (...) {
        // do nothing
    }

    fc::variant mongo_asset_converter::get_variant() const {
        assert(is_valid_value());
        return fc::variant(value_);
    }

    void mongo_asset_converter::convert_to_asset(const int64_t amount, const uint64_t decs, const string& sym) {
        auto symbol_name  = std::to_string(decs).append(1, ',').append(sym);
        auto symbol_value = eosio::chain::symbol::from_string(symbol_name);

        value_ = eosio::chain::asset(amount, symbol_value).to_string();
        kind_  = Asset;
    }

    void mongo_asset_converter::convert_to_symbol(const uint64_t decs, const string& sym) {
        auto symbol_name  = std::to_string(decs).append(1, ',').append(sym);

        eosio::chain::symbol::from_string(symbol_name);
        value_ = std::move(symbol_name);
        kind_  = Symbol;
    }

} } // namespace cyberway::chaindb
