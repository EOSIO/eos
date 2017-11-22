#pragma once

#include <eosio/chain/authority.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>

#include <boost/multiprecision/cpp_int.hpp>

namespace eosio { namespace chain { namespace contracts {

using namespace boost::multiprecision;

template<size_t Size>
using uint_t = number<cpp_int_backend<Size, Size, unsigned_magnitude, unchecked, void> >;
template<size_t Size>
using int_t = number<cpp_int_backend<Size, Size, signed_magnitude, unchecked, void> >;

using uint8     = uint_t<8>;
using uint16    = uint_t<16>;
using uint32    = uint_t<32>;
using uint64    = uint_t<64>;

using fixed_string32 = fc::fixed_string<fc::array<uint64,4>>;
using fixed_string16 = fc::fixed_string<>;
using type_name      = fixed_string32;
using field_name     = fixed_string16;
using table_name     = name;
using action_name    = eosio::chain::action_name;


struct type_def {
   type_def() = default;
   type_def(const type_name& new_type_name, const type_name& type)
   :new_type_name(new_type_name), type(type)
   {}

   type_name   new_type_name;
   type_name   type;
};

struct field_def {
   field_def() = default;
   field_def(const field_name& name, const type_name& type)
   :name(name), type(type)
   {}
   
   field_name name;
   type_name  type;

   bool operator==(const field_def& other) const {
      return std::tie(name, type) == std::tie(other.name, other.type);
   }
};

struct struct_def {
   struct_def() = default;
   struct_def(const type_name& name, const type_name& base, const vector<field_def>& fields)
   :name(name), base(base), fields(fields)
   {}

   type_name            name;
   type_name            base;
   vector<field_def>    fields;

   bool operator==(const struct_def& other) const {
      return std::tie(name, base, fields) == std::tie(other.name, other.base, other.fields);
   }
};

struct action_def {
   action_def() = default;
   action_def(const action_name& name, const type_name& type)
   :name(name), type(type)
   {}

   action_name name;
   type_name type;
};

struct table_def {
   table_def() = default;
   table_def(const table_name& name, const type_name& index_type, const vector<field_name>& key_names, const vector<type_name>& key_types, const type_name& type)
   :name(name), index_type(index_type), key_names(key_names), key_types(key_types), type(type)
   {}

   table_name         name;  // the name of the table
   type_name          index_type;  // the kind of index, i64, i128i128, etc
   vector<field_name> key_names;   // names for the keys defined by key_types
   vector<type_name>  key_types;   // the type of key parameters
   type_name          type;        // type of binary data stored in this table
};

struct abi_def {
   abi_def() = default;
   abi_def(const vector<type_def>& types, const vector<struct_def>& structs, const vector<action_def>& actions, const vector<table_def>& tables)
   :types(types), structs(structs), actions(actions), tables(tables)
   {}

   vector<type_def>     types;
   vector<struct_def>   structs;
   vector<action_def>   actions;
   vector<table_def>    tables;
};

struct transfer {
   account_name   from;
   account_name   to;
   uint64         amount;
   string         memo;

   static name get_scope() {
      return config::system_account_name;
   }

   static name get_name() {
      return N(transfer);
   }
};

struct lock {
   lock() = default;
   lock(const account_name& from, const account_name& to, const share_type& amount)
   :from(from), to(to), amount(amount)
   {}

   account_name                      from;
   account_name                      to;
   share_type                        amount;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(lock);
   }
};

struct unlock {
   unlock() = default;
   unlock(const account_name& account, const share_type& amount)
   :account(account), amount(amount)
   {}

   account_name                      account;
   share_type                        amount;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(unlock);
   }
};

struct claim {
   claim() = default;
   claim(const account_name& account, const share_type& amount)
   :account(account), amount(amount)
   {}

   account_name                      account;
   share_type                        amount;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(claim);
   }
};

struct newaccount {
   /*
   newaccount() = default;
   newaccount(const account_name& creator, const account_name& name, const authority& owner, const authority& active, const authority& recovery, const asset& deposit)
   :creator(creator), name(name), owner(owner), active(active), recovery(recovery), deposit(deposit)
   {}
   */

   account_name                     creator;
   account_name                     name;
   authority                        owner;
   authority                        active;
   authority                        recovery;
   asset                            deposit;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(newaccount);
   }
};

struct setcode {
   /*
   setcode() = default;
   setcode(const account_name& account, const uint8& vmtype, const uint8& vmversion, const bytes& code)
   :account(account), vmtype(vmtype), vmversion(vmversion), code(code)//, abi(abi)
   {}
   */

   account_name                     account;
   uint8                            vmtype;
   uint8                            vmversion;
   bytes                            code;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(setcode);
   }
};

struct setabi {
   /*
   setabi() = default;
   setabi(const account_name& account, const abi_def& abi)
   :account(account), abi(abi)
   {}
   */

   account_name                     account;
   abi_def                          abi;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(setabi);
   }
};

struct setproducer {
   setproducer() = default;
   setproducer(const account_name& name, const public_key_type& key, const chain_config& configuration)
   :name(name), key(key), configuration(configuration)
   {}

   account_name            name;
   public_key_type         key;
   chain_config            configuration;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(setproducer);
   }
};

struct okproducer {
   okproducer() = default;
   okproducer(const account_name& voter, const account_name& producer, const int8_t& approve)
   :voter(voter), producer(producer), approve(approve)
   {}

   account_name                      voter;
   account_name                      producer;
   int8_t                            approve;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(okproducer);
   }
};

struct setproxy {
   setproxy() = default;
   setproxy(const account_name& stakeholder, const account_name& proxy)
   :stakeholder(stakeholder), proxy(proxy)
   {}

   account_name                      stakeholder;
   account_name                      proxy;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(setproxy);
   }
};

struct updateauth {
   account_name                      account;
   permission_name                   permission;
   permission_name                   parent;
   authority                         authority;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(updateauth);
   }
};

struct deleteauth {
   deleteauth() = default;
   deleteauth(const account_name& account, const permission_name& permission)
   :account(account), permission(permission)
   {}

   account_name                      account;
   permission_name                   permission;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(deleteauth);
   }
};

struct linkauth {
   linkauth() = default;
   linkauth(const account_name& account, const account_name& code, const action_name& type, const permission_name& requirement)
   :account(account), code(code), type(type), requirement(requirement)
   {}

   account_name                      account;
   account_name                      code;
   action_name                       type;
   permission_name                   requirement;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(linkauth);
   }
};

struct unlinkauth {
   unlinkauth() = default;
   unlinkauth(const account_name& account, const account_name& code, const action_name& type)
   :account(account), code(code), type(type)
   {}

   account_name                      account;
   account_name                      code;
   action_name                       type;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(unlinkauth);
   }
};

using nonce_type = name;
struct nonce {
   nonce_type value;

   static scope_name get_scope() {
      return config::system_account_name;
   }

   static action_name get_name() {
      return N(nonce);
   }
};

} } } /// namespace eosio::chain::contracts

FC_REFLECT( eosio::chain::contracts::type_def                         , (new_type_name)(type) )
FC_REFLECT( eosio::chain::contracts::field_def                        , (name)(type) )
FC_REFLECT( eosio::chain::contracts::struct_def                       , (name)(base)(fields) )
FC_REFLECT( eosio::chain::contracts::action_def                       , (name)(type) )
FC_REFLECT( eosio::chain::contracts::table_def                        , (name)(index_type)(key_names)(key_types)(type) )
FC_REFLECT( eosio::chain::contracts::abi_def                          , (types)(structs)(actions)(tables) )
FC_REFLECT( eosio::chain::contracts::transfer                         , (from)(to)(amount)(memo) )
FC_REFLECT( eosio::chain::contracts::lock                             , (from)(to)(amount) )
FC_REFLECT( eosio::chain::contracts::unlock                           , (account)(amount) )
FC_REFLECT( eosio::chain::contracts::claim                            , (account)(amount) )
FC_REFLECT( eosio::chain::contracts::newaccount                       , (creator)(name)(owner)(active)(recovery)(deposit) )
FC_REFLECT( eosio::chain::contracts::setcode                          , (account)(vmtype)(vmversion)(code) ) //abi
FC_REFLECT( eosio::chain::contracts::setabi                           , (account)(abi) )
FC_REFLECT( eosio::chain::contracts::setproducer                      , (name)(key)(configuration) )
FC_REFLECT( eosio::chain::contracts::okproducer                       , (voter)(producer)(approve) )
FC_REFLECT( eosio::chain::contracts::setproxy                         , (stakeholder)(proxy) )
FC_REFLECT( eosio::chain::contracts::updateauth                       , (account)(permission)(parent)(authority) )
FC_REFLECT( eosio::chain::contracts::deleteauth                       , (account)(permission) )
FC_REFLECT( eosio::chain::contracts::linkauth                         , (account)(code)(type)(requirement) )
FC_REFLECT( eosio::chain::contracts::unlinkauth                       , (account)(code)(type) )
FC_REFLECT( eosio::chain::contracts::nonce                            , (value) )
