#pragma once

#include <eosio/state_history/serialization.hpp>
#include <eosio/chain/backing_store/db_combined.hpp>

namespace eosio {
namespace state_history {

/*
 *   Base class for receiver of rocksdb data for SHiP delta generation.
 *
*/
class rocksdb_receiver {
public:
   // This method is not required for this use case, but the interface requires we implement it.
   void add_row(fc::unsigned_int x) {

   }

protected:
   rocksdb_receiver(std::vector<table_delta> &deltas, const chainbase::database& db) : deltas_(deltas), db_(db) {};

   std::optional<chain::backing_store::table_id_object_view> table_;
   bool present_{true};

   template<typename Object>
   void add_delta_with_table(const char *name, const Object& row){
      deltas_[get_delta_index(name)].rows.obj.emplace_back(present_, fc::raw::pack(make_history_context_wrapper(db_, *table_, row)));
   }

   template<typename Object>
   void add_delta(const char *name, const Object& row){
      deltas_[get_delta_index(name)].rows.obj.emplace_back(present_, fc::raw::pack(make_history_serial_wrapper(db_, row)));
   }

private:
   std::map<std::string, int> name_to_index;

   int get_delta_index(std::string name) {
      if(!name_to_index.count(name)) {
         deltas_.push_back({});
         deltas_.back().name = name;
         name_to_index[name] = deltas_.size() - 1;
      }

      return name_to_index[name];
   }

   std::vector<table_delta> &deltas_;
   const chainbase::database& db_;

};

/*
 * Specialization of rocksdb data's receiver for SHiP delta generation when we are getting the whole db.
 *
 * When we are receiving the whole current db content, then we store the deltas of the objects as we
 * receive them.
 */
class rocksdb_receiver_whole_db : public rocksdb_receiver {
public:
   using rocksdb_receiver::add_row;

   rocksdb_receiver_whole_db(std::vector<table_delta> &deltas, const chainbase::database& db) : rocksdb_receiver(deltas, db) {}

   void add_row(const chain::kv_object_view& row) {
      add_delta("key_value", row);
   }

   void add_row(const chain::backing_store::table_id_object_view& row) {
      add_delta("contract_table", row);
      table_ = row;
   }

   void add_row(const chain::backing_store::primary_index_view& row) {
      add_delta_with_table("contract_row", row);
   }

   void add_row(const chain::backing_store::secondary_index_view<uint64_t>& row) {
      add_delta_with_table("contract_index64", row);
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::uint128_t>& row) {
      add_delta_with_table("contract_index128", row);
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::key256_t>& row) {
      add_delta_with_table("contract_index256", row);
   }

   void add_row(const chain::backing_store::secondary_index_view<float64_t>& row) {
      add_delta_with_table("contract_index_double", row);
   }

   void add_row(const chain::backing_store::secondary_index_view<float128_t>& row) {
      add_delta_with_table("contract_index_long_double", row);
   }
};


/*
 * Specialization of rocksdb data's receiver for SHiP delta generation when we are operating on single key entries (updated or deleted).
 *
 * When operating on the "single entry" mode, it will be used when working with single key entries, because
 * some of these objects (primary and secondary indexes) require information about the table they belong to, then for
 * these objects we will receive 2 calls, the first one related to the table_id_object_view (but in this case this object
 * does not contain proper values for payer so it can't be used to create its delta) associated with them and
 * the second call for the object itself. For that reason there is logic also associated to detect when we want
 * to store the table_id_object_view itself (which is when we receive only 1 call for this and the next call is not
 * any of the primary or secondary indexes).
*/
class rocksdb_receiver_single_entry : public rocksdb_receiver {
public:
   using rocksdb_receiver::add_row;

   rocksdb_receiver_single_entry(std::vector<table_delta> &deltas, const chainbase::database& db) : rocksdb_receiver(deltas, db) {}

   void add_row(const chain::kv_object_view& row) {
      maybe_process_table();

      add_delta("key_value", row);
   }

   /*
    * In this case we do not immediately report it as a delta until we see what comes next. If it is followed by a row
    * that requires the table context (i.e. calls add_row_with_table) then the table_id_object_view was just being
    * provided as the context. If it is followed by any other row, then it was indicating a change in that
    * table_id_object_view, and it needs to be added to the delta.
   */
   void add_row(const chain::backing_store::table_id_object_view& row) {
      maybe_process_table();
      table_ = row;
   }

   void add_row(const chain::backing_store::primary_index_view& row) {
      add_delta_with_table("contract_row", row);
      table_ = {};
   }

   void add_row(const chain::backing_store::secondary_index_view<uint64_t>& row) {
      add_delta_with_table("contract_index64", row);
      table_ = {};
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::uint128_t>& row) {
      add_delta_with_table("contract_index128", row);
      table_ = {};
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::key256_t>& row) {
      add_delta_with_table("contract_index256", row);
      table_ = {};
   }

   void add_row(const chain::backing_store::secondary_index_view<float64_t>& row) {
      add_delta_with_table("contract_index_double", row);
      table_ = {};
   }

   void add_row(const chain::backing_store::secondary_index_view<float128_t>& row) {
      add_delta_with_table("contract_index_long_double", row);
      table_ = {};
   }

   void set_delta_present_flag(bool value) {
      maybe_process_table();

      present_ = value;
   }

   ~rocksdb_receiver_single_entry() {
      maybe_process_table();
   }

private:

   void maybe_process_table() {
      if(table_) {
         add_delta("contract_table", *table_);
         table_ = {};
      }
   }
};

}
}