#pragma once

#include <eos/chain/config.hpp>
#include <eos/chain/types.hpp>

namespace eosio { namespace chain { namespace contracts {

struct transfer {
   transfer() = default;
   transfer(const account_name& from, const account_name& to, const uint64_t& amount, const string& memo) 
   :from(from), to(to), amount(amount), memo(memo)
   {}

   account_name   from;
   account_name   to;
   uint64_t       amount;
   string         memo;

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(transfer);
   }
};

template<> struct get_struct<transfer> {
   static const struct_def& type() {
      static struct_def result = { "transfer", "", {
            {"from", "account_name"},
            {"to", "account_name"},
            {"amount", "uint64_t"},
            {"memo", "string"},
         }
      };
      return result;
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

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(lock);
   }
};

template<> struct get_struct<lock> {
   static const struct_def& type() {
      static struct_def result = { "lock", "", {
            {"from", "account_name"},
            {"to", "account_name"},
            {"amount", "share_type"},
         }
      };
      return result;
   }
};

struct unlock {
   unlock() = default;
   unlock(const account_name& account, const share_type& amount)
   :account(account), amount(amount)
   {}

   account_name                      account;
   share_type                        amount;

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(unlock);
   }
};

template<> struct get_struct<unlock> {
   static const struct_def& type() {
      static struct_def result = { "unlock", "", {
            {"account", "account_name"},
            {"amount", "share_type"},
         }
      };
      return result;
   }
};

struct claim {
   claim() = default;
   claim(const account_name& account, const share_type& amount)
   :account(account), amount(amount)
   {}

   account_name                      account;
   share_type                        amount;

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(claim);
   }
};

template<> struct get_struct<claim> {
   static const struct_def& type() {
      static struct_def result = { "claim", "", {
         {"account", "account_name"},
         {"amount", "share_type"},
      }
      };
      return result;
   }
};

struct newaccount {
   newaccount() = default;
   newaccount(const account_name& creator, const account_name& name, const authority& owner, const authority& active, const authority& recovery, const asset& deposit)
   :creator(creator), name(name), owner(owner), active(active), recovery(recovery), deposit(deposit)
   {}

   account_name                     creator;
   account_name                     name;
   authority                        owner;
   authority                        active;
   authority                        recovery;
   asset                            deposit;

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(newaccount);
   }
};

template<> struct get_struct<newaccount> {
   static const struct_def& type() {
      static struct_def result = { "newaccount", "", {
            {"creator", "account_name"},
            {"name", "account_name"},
            {"owner", "authority"},
            {"active", "authority"},
            {"recovery", "authority"},
            {"deposit", "asset"},
         }
      };
      return result;
   }
};

struct setcode {
   setcode() = default;
   setcode(const account_name& account, const uint8_t& vmtype, const uint8_t& vmversion, const bytes& code, const abi& abi)
   :account(account), vmtype(vmtype), vmversion(vmversion), code(code), abi(abi)
   {}

   account_name                     account;
   uint8_t                          vmtype;
   uint8_t                          vmversion;
   bytes                            code;
   abi                              abi;

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(setcode);
   }
};

template<> struct get_struct<setcode> {
   static const struct_def& type() {
      static struct_def result = { "setcode", "", {
            {"account", "account_name"},
            {"vmtype", "uint8_t"},
            {"vmversion", "uint8_t"},
            {"code", "bytes"},
            {"abi", "abi"},
         }
      };
      return result;
   }
};

struct setproducer {
   setproducer() = default;
   setproducer(const account_name& name, const public_key& key, const blockchain_configuration& configuration)
   :name(name), key(key), configuration(configuration)
   {}

   account_name                      name;
   public_key                        key;
   blockchain_configuration          configuration;

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(setproducer);
   }
};

template<> struct get_struct<setproducer> {
   static const struct_def& type() {
      static struct_def result = { "setproducer", "", {
            {"name", "account_name"},
            {"key", "public_key"},
            {"configuration", "blockchain_configuration"},
         }
      };
      return result;
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

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(okproducer);
   }
};

template<> struct get_struct<okproducer> {
   static const struct_def& type() {
      static struct_def result = { "okproducer", "", {
            {"voter", "account_name"},
            {"producer", "account_name"},
            {"approve", "int8_t"},
         }
      };
      return result;
   }
};

struct setproxy {
   setproxy() = default;
   setproxy(const account_name& stakeholder, const account_name& proxy)
   :stakeholder(stakeholder), proxy(proxy)
   {}

   account_name                      stakeholder;
   account_name                      proxy;

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(setproxy);
   }
};

template<> struct get_struct<setproxy> {
   static const struct_def& type() {
      static struct_def result = { "setproxy", "", {
            {"stakeholder", "account_name"},
            {"proxy", "account_name"},
         }
      };
      return result;
   }
};

struct updateauth {
   updateauth() = default;
   updateauth(const account_name& account, const permission_name& permission, const permission_name& parent, const authority& authority)
   :account(account), permission(permission), parent(parent), authority(authority)
   {}

   account_name                      account;
   permission_name                   permission;
   permission_name                   parent;
   authority                        authority;

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(updateauth);
   }
};

template<> struct get_struct<updateauth> {
   static const struct_def& type() {
      static struct_def result = { "updateauth", "", {
            {"account", "account_name"},
            {"permission", "permission_name"},
            {"parent", "permission_name"},
            {"authority", "authority"},
         }
      };
      return result;
   }
};

struct deleteauth {
   deleteauth() = default;
   deleteauth(const account_name& account, const permission_name& permission)
   :account(account), permission(permission)
   {}

   account_name                      account;
   permission_name                   permission;

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(deleteauth);
   }
};

template<> struct get_struct<deleteauth> {
   static const struct_def& type() {
      static struct_def result = { "deleteauth", "", {
            {"account", "account_name"},
            {"permission", "permission_name"},
         }
      };
      return result;
   }
};

struct linkauth {
   linkauth() = default;
   linkauth(const account_name& account, const account_name& code, const action_name& type, const permission_name& requirement)
   :account(account), code(code), type(type), requirement(requirement)
   {}

   account_name                      account;
   account_name                      code;
   action_name                        type;
   permission_name                   requirement;

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(linkauth);
   }
};

template<> struct get_struct<linkauth> {
   static const struct_def& type() {
      static struct_def result = { "linkauth", "", {
            {"account", "account_name"},
            {"code", "account_name"},
            {"type", "action_name"},
            {"requirement", "permission_name"},
         }
      };
      return result;
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

   static constexpr scope_name get_scope() {
      return config::system_account_name;
   }

   static constexpr action_name get_name() {
      return N(unlinkauth);
   }
};

template<> struct get_struct<unlinkauth> {
   static const struct_def& type() {
      static struct_def result = { "unlinkauth", "", {
            {"account", "account_name"},
            {"code", "account_name"},
            {"type", "action_name"},
         }
      };
      return result;
   }
};

} } } /// namespace eosio::chain::contracts

FC_REFLECT( eosio::chain::contracts::transfer                         , (from)(to)(amount)(memo) )
FC_REFLECT( eosio::chain::contracts::lock                             , (from)(to)(amount) )
FC_REFLECT( eosio::chain::contracts::unlock                           , (account)(amount) )
FC_REFLECT( eosio::chain::contracts::claim                            , (account)(amount) )
FC_REFLECT( eosio::chain::contracts::newaccount                       , (creator)(name)(owner)(active)(recovery)(deposit) )
FC_REFLECT( eosio::chain::contracts::setcode                          , (account)(vmtype)(vmversion)(code)(abi) )
FC_REFLECT( eosio::chain::contracts::setproducer                      , (name)(key)(configuration) )
FC_REFLECT( eosio::chain::contracts::okproducer                       , (voter)(producer)(approve) )
FC_REFLECT( eosio::chain::contracts::setproxy                         , (stakeholder)(proxy) )
FC_REFLECT( eosio::chain::contracts::updateauth                       , (account)(permission)(parent)(authority) )
FC_REFLECT( eosio::chain::contracts::deleteauth                       , (account)(permission) )
FC_REFLECT( eosio::chain::contracts::linkauth                         , (account)(code)(type)(requirement) )
FC_REFLECT( eosio::chain::contracts::unlinkauth                       , (account)(code)(type) )