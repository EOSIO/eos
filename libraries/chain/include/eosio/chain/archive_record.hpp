#pragma once

namespace eosio { namespace chain {

    struct archive_record {
        account_name code;
        vector<char> data;

        archive_record() = default;

        archive_record(account_name _code, const std::vector<char>& _data)
        : code(_code), data(_data)
        {}
    };

} } // namespace eosio::chain


FC_REFLECT(eosio::chain::archive_record,
            (code)(data))
