#pragma once

#include <b1/rodeos/callbacks/kv.hpp>
#include <eosio/ship_protocol.hpp>
#include <eosio/to_key.hpp>

namespace eosio {
using b1::rodeos::kv_environment;
}

#include "../wasms/table.hpp"

namespace b1::rodeos {

struct fill_status_v0 {
   eosio::checksum256 chain_id        = {};
   uint32_t           head            = {};
   eosio::checksum256 head_id         = {};
   uint32_t           irreversible    = {};
   eosio::checksum256 irreversible_id = {};
   uint32_t           first           = {};
};

EOSIO_REFLECT(fill_status_v0, chain_id, head, head_id, irreversible, irreversible_id, first)

using fill_status = std::variant<fill_status_v0>;

inline bool operator==(const fill_status_v0& a, fill_status_v0& b) {
   return std::tie(a.head, a.head_id, a.irreversible, a.irreversible_id, a.first) ==
          std::tie(b.head, b.head_id, b.irreversible, b.irreversible_id, b.first);
}

inline bool operator!=(const fill_status_v0& a, fill_status_v0& b) { return !(a == b); }

struct fill_status_kv : eosio::table<fill_status> {
   index primary_index{ eosio::name{ "primary" }, [](const auto& var) { return std::vector<char>{}; } };

   fill_status_kv(eosio::kv_environment environment) : eosio::table<fill_status>{ std::move(environment) } {
      init(eosio::name{ "eosio.state" }, eosio::name{ "state" }, eosio::name{ "fill.status" }, primary_index);
   }
};

struct block_info_v0 {
   uint32_t                         num                = {};
   eosio::checksum256               id                 = {};
   eosio::block_timestamp           timestamp          = {};
   eosio::name                      producer           = {};
   uint16_t                         confirmed          = {};
   eosio::checksum256               previous           = {};
   eosio::checksum256               transaction_mroot  = {};
   eosio::checksum256               action_mroot       = {};
   uint32_t                         schedule_version   = {};
   std::optional<producer_schedule> new_producers      = {};
   eosio::signature                 producer_signature = {};
};

EOSIO_REFLECT(block_info_v0, num, id, timestamp, producer, confirmed, previous, transaction_mroot, action_mroot,
              schedule_version, new_producers, producer_signature)

using block_info = std::variant<block_info_v0>;

// todo: move out of "state"?
struct block_info_kv : eosio::table<block_info> {
   index primary_index{ eosio::name{ "primary" }, [](const auto& var) {
                          return std::visit(
                                [](const auto& obj) { return eosio::check(eosio::convert_to_key(obj.num)).value(); },
                                var);
                       } };

   index id_index{ eosio::name{ "id" }, [](const auto& var) {
                     return std::visit(
                           [](const auto& obj) { return eosio::check(eosio::convert_to_key(obj.id)).value(); }, var);
                  } };

   block_info_kv(eosio::kv_environment environment) : eosio::table<block_info>{ std::move(environment) } {
      init(eosio::name{ "eosio.state" }, eosio::name{ "state" }, eosio::name{ "block.info" }, primary_index, id_index);
   }
};

struct global_property_kv : eosio::table<global_property> {
   index primary_index{ eosio::name{ "primary" }, [](const auto& var) { return std::vector<char>{}; } };

   global_property_kv(eosio::kv_environment environment) : eosio::table<global_property>{ std::move(environment) } {
      init(eosio::name{ "eosio.state" }, eosio::name{ "state" }, eosio::name{ "global.prop" }, primary_index);
   }
};

struct account_kv : eosio::table<account> {
   index primary_index{ eosio::name{ "primary" }, [](const auto& var) {
                          return std::visit(
                                [](const auto& obj) {
                                   return eosio::check(eosio::convert_to_key(std::tie(obj.name))).value();
                                },
                                var);
                       } };

   account_kv(eosio::kv_environment environment) : eosio::table<account>{ std::move(environment) } {
      init(eosio::name{ "eosio.state" }, eosio::name{ "state" }, eosio::name{ "account" }, primary_index);
   }
};

struct account_metadata_kv : eosio::table<account_metadata> {
   index primary_index{ eosio::name{ "primary" }, [](const auto& var) {
                          return std::visit(
                                [](const auto& obj) {
                                   return eosio::check(eosio::convert_to_key(std::tie(obj.name))).value();
                                },
                                var);
                       } };

   account_metadata_kv(eosio::kv_environment environment) : eosio::table<account_metadata>{ std::move(environment) } {
      init(eosio::name{ "eosio.state" }, eosio::name{ "state" }, eosio::name{ "account.meta" }, primary_index);
   }
};

struct code_kv : eosio::table<code> {
   index primary_index{ eosio::name{ "primary" }, [](const auto& var) {
                          return std::visit(
                                [](const auto& obj) {
                                   return eosio::check(eosio::convert_to_key(
                                                             std::tie(obj.vm_type, obj.vm_version, obj.code_hash)))
                                         .value();
                                },
                                var);
                       } };

   code_kv(eosio::kv_environment environment) : eosio::table<code>{ std::move(environment) } {
      init(eosio::name{ "eosio.state" }, eosio::name{ "state" }, eosio::name{ "code" }, primary_index);
   }
};

struct contract_table_kv : eosio::table<contract_table> {
   index primary_index{
      eosio::name{ "primary" },
      [](const auto& var) {
         return std::visit(
               [](const auto& obj) {
                  return eosio::check(eosio::convert_to_key(std::tie(obj.code, obj.table, obj.scope))).value();
               },
               var);
      }
   };

   contract_table_kv(eosio::kv_environment environment) : eosio::table<contract_table>{ std::move(environment) } {
      init(eosio::name{ "eosio.state" }, eosio::name{ "state" }, eosio::name{ "contract.tab" }, primary_index);
   }
};

struct contract_row_kv : eosio::table<contract_row> {
   index primary_index{ eosio::name{ "primary" }, [](const auto& var) {
                          return std::visit(
                                [](const auto& obj) {
                                   return eosio::check(eosio::convert_to_key(
                                                             std::tie(obj.code, obj.table, obj.scope, obj.primary_key)))
                                         .value();
                                },
                                var);
                       } };

   contract_row_kv(eosio::kv_environment environment) : eosio::table<contract_row>{ std::move(environment) } {
      init(eosio::name{ "eosio.state" }, eosio::name{ "state" }, eosio::name{ "contract.row" }, primary_index);
   }
};

struct contract_index64_kv : eosio::table<contract_index64> {
   index primary_index{ eosio::name{ "primary" }, [](const auto& var) {
                          return std::visit(
                                [](const auto& obj) {
                                   return eosio::check(eosio::convert_to_key(
                                                             std::tie(obj.code, obj.table, obj.scope, obj.primary_key)))
                                         .value();
                                },
                                var);
                       } };
   index secondary_index{ eosio::name{ "secondary" }, [](const auto& var) {
                            return std::visit(
                                  [](const auto& obj) {
                                     return eosio::check(
                                                  eosio::convert_to_key(std::tie(obj.code, obj.table, obj.scope,
                                                                                 obj.secondary_key, obj.primary_key)))
                                           .value();
                                  },
                                  var);
                         } };

   contract_index64_kv(eosio::kv_environment environment) : eosio::table<contract_index64>{ std::move(environment) } {
      init(eosio::name{ "eosio.state" }, eosio::name{ "state" }, eosio::name{ "contract.i1" }, primary_index,
           secondary_index);
   }
};

struct contract_index128_kv : eosio::table<contract_index128> {
   index primary_index{ eosio::name{ "primary" }, [](const auto& var) {
                          return std::visit(
                                [](const auto& obj) {
                                   return eosio::check(eosio::convert_to_key(
                                                             std::tie(obj.code, obj.table, obj.scope, obj.primary_key)))
                                         .value();
                                },
                                var);
                       } };
   index secondary_index{ eosio::name{ "secondary" }, [](const auto& var) {
                            return std::visit(
                                  [](const auto& obj) {
                                     return eosio::check(
                                                  eosio::convert_to_key(std::tie(obj.code, obj.table, obj.scope,
                                                                                 obj.secondary_key, obj.primary_key)))
                                           .value();
                                  },
                                  var);
                         } };

   contract_index128_kv(eosio::kv_environment environment) : eosio::table<contract_index128>{ std::move(environment) } {
      init(eosio::name{ "eosio.state" }, eosio::name{ "state" }, eosio::name{ "contract.i2" }, primary_index,
           secondary_index);
   }
};

template <typename Table, typename F>
void store_delta_typed(eosio::kv_environment environment, table_delta_v0& delta, bool bypass_preexist_check, F f) {
   Table table{ environment };
   for (auto& row : delta.rows) {
      f();
      auto obj = eosio::check(eosio::from_bin<typename Table::value_type>(row.data)).value();
      if (row.present)
         table.insert(obj, bypass_preexist_check);
      else
         table.erase(obj);
   }
}

template <typename F>
void store_delta_kv(eosio::kv_environment environment, table_delta_v0& delta, F f) {
   for (auto& row : delta.rows) {
      f();
      auto  obj  = eosio::check(eosio::from_bin<key_value>(row.data)).value();
      auto& obj0 = std::get<key_value_v0>(obj);
      if (row.present)
         environment.kv_set(obj0.database.value, obj0.contract.value, obj0.key.pos, obj0.key.remaining(),
                            obj0.value.pos, obj0.value.remaining());
      else
         environment.kv_erase(obj0.database.value, obj0.contract.value, obj0.key.pos, obj0.key.remaining());
   }
}

template <typename F>
inline void store_delta(eosio::kv_environment environment, table_delta_v0& delta, bool bypass_preexist_check, F f) {
   if (delta.name == "global_property")
      store_delta_typed<global_property_kv>(environment, delta, bypass_preexist_check, f);
   if (delta.name == "account")
      store_delta_typed<account_kv>(environment, delta, bypass_preexist_check, f);
   if (delta.name == "account_metadata")
      store_delta_typed<account_metadata_kv>(environment, delta, bypass_preexist_check, f);
   if (delta.name == "code")
      store_delta_typed<code_kv>(environment, delta, bypass_preexist_check, f);
   if (delta.name == "contract_table")
      store_delta_typed<contract_table_kv>(environment, delta, bypass_preexist_check, f);
   if (delta.name == "contract_row")
      store_delta_typed<contract_row_kv>(environment, delta, bypass_preexist_check, f);
   if (delta.name == "contract_index64")
      store_delta_typed<contract_index64_kv>(environment, delta, bypass_preexist_check, f);
   if (delta.name == "contract_index128")
      store_delta_typed<contract_index128_kv>(environment, delta, bypass_preexist_check, f);
   if (delta.name == "key_value")
      store_delta_kv(environment, delta, f);
}

inline void store_deltas(eosio::kv_environment environment, std::vector<table_delta>& deltas,
                         bool bypass_preexist_check) {
   for (auto& delta : deltas) //
      store_delta(environment, std::get<table_delta_v0>(delta), bypass_preexist_check, [] {});
}

} // namespace b1::rodeos
