#pragma once

#include <eosio/state_history/serialization.hpp>
#include <eosio/chain/backing_store/db_combined.hpp>

namespace eosio {
namespace state_history {

/*
 *   Receiver of rocksdb data.
 *
 *   It has 2 modes of operation:
 *
 *   1. When operating on the single entry mode, then this mode will be used when working with single key entries, because
 *   some of these objects (primary and secondary indexes) require information about the table they belong to, then for
 *   these values we will receive 2 calls, the first one related to the table_id_object_view (but in this case this object
 *   does not contain proper values for payer so it can't be used to create its delta) associated with them and
 *   the second call for the object itself. For that reason there is logic also associated to detect when we want
 *   to store the table_id_object_view itself (which is when we receive only 1 call for this and the next call is not
 *   any of the primary or secondary indexes).
 *
 *   2. When operating on the not sigle entry mode, then we store the deltas of the objects as we receive them.
 *
 */
class rocksdb_receiver {
public:
   rocksdb_receiver(std::vector<table_delta> &deltas, const chainbase::database& db, bool single_entry = false) : deltas(deltas), db_(db), single_entry(single_entry) {};

   void add_row(const chain::kv_object_view& row) {
      if(single_entry) {
         maybe_process_table();
      }

      deltas[get_delta_index("key_value")].rows.obj.emplace_back(present, fc::raw::pack(make_history_serial_wrapper(db_, row)));
      table_ = {};
   }

   void add_row(const chain::backing_store::table_id_object_view& row) {
      if(single_entry) {
         maybe_process_table();
      } else {
         deltas[get_delta_index("contract_table")].rows.obj.emplace_back(present, fc::raw::pack(make_history_serial_wrapper(db_, row)));
      }

      table_ = row;
   }

   void add_row(const chain::backing_store::primary_index_view& row) {
      deltas[get_delta_index("contract_row")].rows.obj.emplace_back(present, fc::raw::pack(make_history_context_wrapper(db_, *table_, row)));
      table_ = {};
   }

   void add_row(const chain::backing_store::secondary_index_view<uint64_t>& row) {
      deltas[get_delta_index("contract_index64")].rows.obj.emplace_back(present, fc::raw::pack(make_history_context_wrapper(db_, *table_, row)));
      table_ = {};
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::uint128_t>& row) {
      deltas[get_delta_index("contract_index128")].rows.obj.emplace_back(present, fc::raw::pack(make_history_context_wrapper(db_, *table_, row)));
      table_ = {};
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::key256_t>& row) {
      deltas[get_delta_index("contract_index256")].rows.obj.emplace_back(present, fc::raw::pack(make_history_context_wrapper(db_, *table_, row)));
      table_ = {};
   }

   void add_row(const chain::backing_store::secondary_index_view<float64_t>& row) {
      deltas[get_delta_index("contract_index_double")].rows.obj.emplace_back(present, fc::raw::pack(make_history_context_wrapper(db_, *table_, row)));
      table_ = {};
   }

   void add_row(const chain::backing_store::secondary_index_view<float128_t>& row) {
      deltas[get_delta_index("contract_index_long_double")].rows.obj.emplace_back(present, fc::raw::pack(make_history_context_wrapper(db_, *table_, row)));
      table_ = {};
   }

   void add_row(fc::unsigned_int x) {

   }

   void set_delta_present_flag(bool value) {
      present = value;
   }

   ~rocksdb_receiver() {
      if(single_entry) {
         maybe_process_table();
      }
   }

private:
   std::map<std::string, int> name_to_index;

   int get_delta_index(std::string name) {
      if(!name_to_index.count(name)) {
         deltas.push_back({});
         deltas.back().name = name;
         name_to_index[name] = deltas.size() - 1;
      }

      return name_to_index[name];
   }

   std::vector<table_delta> &deltas;
   const chainbase::database& db_;
   std::optional<chain::backing_store::table_id_object_view> table_;

   void maybe_process_table() {
      if(table_) {
         deltas[get_delta_index("contract_table")].rows.obj.emplace_back(present, fc::raw::pack(make_history_serial_wrapper(db_, *table_)));
         table_ = {};
      }
   }

   bool single_entry{false};
   bool present{true};
};

}
}