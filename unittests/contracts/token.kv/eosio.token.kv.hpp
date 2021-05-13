#pragma once
#include <eosio/asset.hpp>
#include <eosio/crypto.hpp>
#include <eosio/dispatcher.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/transaction.hpp>

#define ACT [[eosio::action]]

using namespace eosio;
using namespace std;

struct currency_stats_record {
    asset supply;
    asset max_supply;
    name  issuer;

    auto by_issuer_code() const { return std::tuple(issuer.value, supply.symbol.code().raw()); }
    auto by_code() const { return supply.symbol.code().raw(); }
};

struct stats_table : kv_table<currency_stats_record> {
    KV_NAMED_INDEX("by.iss.code", by_issuer_code);
    KV_NAMED_INDEX("by.code", by_code);
    stats_table() { init("token.kv"_n, "stat"_n, eosio::kv_ram, by_issuer_code, by_code); }
};

struct account_record {
    name account_name;
    asset balance;

    auto by_account_code() const { return std::tuple(account_name.value, balance.symbol.code().raw()); }
};

struct accounts_table : kv_table<account_record> {
    KV_NAMED_INDEX("by.acc.code", by_account_code);
    accounts_table() { init("token.kv"_n, "accounts"_n, eosio::kv_ram, by_account_code); }
};

class [[eosio::contract("token.kv")]]  token_kv_contract : public eosio::contract {
    private:
         template<typename T>
         auto& global() {
            static T t; return t;
         }

         void add_balance(const name& owner, const asset& value);
         void sub_balance(const name& owner, const asset& value);

    public:
        using contract::contract;

        ACT void create(const name& issuer, const asset& maximum_supply);
        ACT void issue(const name& to, const asset& quantity, const string& memo);
        ACT void retire(const asset& quantity, const string& memo);
        ACT void transfer(const name& from, const name& to, const asset& quantity, const string& memo);
        ACT void open(const name& owner, const symbol& symbol);
        ACT void close(const name& owner, const symbol& symbol);
};
