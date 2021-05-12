#include "eosio.token.hpp"

using namespace eosio;
using namespace std;

void token_kv_contract::add_balance(const name& owner, const asset& value) {
   auto& at = global<accounts_table>();
   const auto existing = at.by_account_code.get(std::tuple(owner.value, value.symbol.code().raw()));
   if(!existing.has_value()) {
       at.put({
           .account_name = owner,
           .balance = value
       });
   } else {
       auto new_balance = existing->balance + value;
       at.put({
           .account_name = owner,
           .balance = new_balance
       });
   }
}

void token_kv_contract::sub_balance(const name& owner, const asset& value) {
   auto& at = global<accounts_table>();
   const auto existing = at.by_account_code.get(std::tuple(owner.value, value.symbol.code().raw()));
   check(existing.has_value(), "no balance object found");
   check(existing->balance.amount >= value.amount, "overdrawn balance");
   auto new_balance = existing->balance - value;
   at.put({
       .account_name = owner,
       .balance = new_balance
   });
}

void token_kv_contract::create(const name& issuer, const asset& maximum_supply) {
    require_auth(get_self());

    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( maximum_supply.is_valid(), "invalid supply");
    check( maximum_supply.amount > 0, "max-supply must be positive");

    auto& st = global<stats_table>();
    check(!st.by_code.exists(sym.code().raw()), "token with symbol already exists");

    st.put({
        .supply.symbol = maximum_supply.symbol,
        .max_supply    = maximum_supply, 
        .issuer        = issuer
    });
}

void token_kv_contract:: issue(const name& to, const asset& quantity, const string& memo) {
    auto sym = quantity.symbol;
    check(sym.is_valid(), "invalid symbol name");
    check(memo.size() <= 256, "memo has more than 256 bytes");

    auto& st = global<stats_table>();
    const auto existing = st.by_code.get(sym.code().raw());
    check(existing.has_value(), "token with symbol does not exist, create token before issue");
    check(to == existing->issuer, "tokens can only be issued to issuer account");

    require_auth(existing->issuer);
    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must issue positive quantity");

    check(quantity.symbol == existing->supply.symbol, "symbol precision mismatch");
    check(quantity.amount <= existing->max_supply.amount - existing->supply.amount, "quantity exceeds available supply");

    const auto existing_user = st.by_issuer_code.get(std::tuple(to.value, sym.code().raw()));
    auto new_quantity = existing_user->supply + quantity;
    st.put({
        .supply     = new_quantity,
        .issuer     = existing_user->issuer,
        .max_supply = existing_user->max_supply
    });

    add_balance(existing_user->issuer, quantity);
}

void token_kv_contract:: retire(const asset& quantity, const string& memo)
{
    auto sym = quantity.symbol;
    check(sym.is_valid(), "invalid symbol name");
    check(memo.size() <= 256, "memo has more than 256 bytes");

    auto& st = global<stats_table>();
    const auto existing = st.by_code.get(sym.code().raw());
    check(existing.has_value(), "token with symbol does not exist" );

    require_auth(existing->issuer);
    check(quantity.is_valid(), "invalid quantity" );
    check(quantity.amount > 0, "must retire positive quantity");

    check(quantity.symbol == existing->supply.symbol, "symbol precision mismatch");

    auto new_supply = existing->supply - quantity;
    st.put({
        .supply     = new_supply,
        .issuer     = existing->issuer,
        .max_supply = existing->max_supply
    });

    sub_balance(existing->issuer, quantity);
}

void token_kv_contract:: transfer(const name& from, const name& to, const asset& quantity, const string& memo) {
    check( from != to, "cannot transfer to self" );
    require_auth(from);
    check(is_account(to), "to account does not exist");
    auto sym = quantity.symbol.code();
    auto& st = global<stats_table>();
    const auto existing = st.by_code.get(sym.raw());

    require_recipient(from);
    require_recipient(to);

    check(quantity.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "must transfer positive quantity");
    check(quantity.symbol == existing->supply.symbol, "symbol precision mismatch");
    check(memo.size() <= 256, "memo has more than 256 bytes");

    sub_balance(from, quantity);
    add_balance(to, quantity);
}

void token_kv_contract:: open(const name& owner, const symbol& symbol) {
   require_auth(_self );

   check(is_account(owner), "owner account does not exist");

   auto sym_code_raw = symbol.code().raw();
   auto& st = global<stats_table>();
   const auto existing = st.by_code.get(sym_code_raw);
   check(existing.has_value(), "symbol does not exist");
   check(existing->supply.symbol == symbol, "symbol precision mismatch");

   auto& at = global<accounts_table>();
   const auto existing_account = at.by_account_code.get(std::tuple(owner.value, sym_code_raw));
   if(!existing_account.has_value()) {
       at.put({
           .account_name = owner,
           .balance = asset{0, symbol}
       });
   }
}

void token_kv_contract::close( const name& owner, const symbol& symbol ) {
   require_auth(owner);
   auto& at = global<accounts_table>();
   const auto existing = at.by_account_code.get(std::tuple(owner.value, symbol.code().raw()));
   check(existing.has_value(), "Balance row already deleted or never existed. Action won't have any effect.");
   check(existing->balance.amount == 0, "Cannot close because the balance is not zero.");
   at.erase(existing.value());
}
