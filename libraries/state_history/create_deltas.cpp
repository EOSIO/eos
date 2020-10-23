#include <eosio/state_history/create_deltas.hpp>
#include <eosio/state_history/serialization.hpp>
#include <eosio/chain/backing_store/db_combined.hpp>

namespace eosio {
namespace state_history {

template <typename T>
bool include_delta(const T& old, const T& curr) {
   return true;
}

bool include_delta(const chain::table_id_object& old, const chain::table_id_object& curr) {
   return old.payer != curr.payer;
}

bool include_delta(const chain::resource_limits::resource_limits_object& old,
                   const chain::resource_limits::resource_limits_object& curr) {
   return                                   //
       old.net_weight != curr.net_weight || //
       old.cpu_weight != curr.cpu_weight || //
       old.ram_bytes != curr.ram_bytes;
}

bool include_delta(const chain::resource_limits::resource_limits_state_object& old,
                   const chain::resource_limits::resource_limits_state_object& curr) {
   return                                                                                       //
       old.average_block_net_usage.last_ordinal != curr.average_block_net_usage.last_ordinal || //
       old.average_block_net_usage.value_ex != curr.average_block_net_usage.value_ex ||         //
       old.average_block_net_usage.consumed != curr.average_block_net_usage.consumed ||         //
       old.average_block_cpu_usage.last_ordinal != curr.average_block_cpu_usage.last_ordinal || //
       old.average_block_cpu_usage.value_ex != curr.average_block_cpu_usage.value_ex ||         //
       old.average_block_cpu_usage.consumed != curr.average_block_cpu_usage.consumed ||         //
       old.total_net_weight != curr.total_net_weight ||                                         //
       old.total_cpu_weight != curr.total_cpu_weight ||                                         //
       old.total_ram_bytes != curr.total_ram_bytes ||                                           //
       old.virtual_net_limit != curr.virtual_net_limit ||                                       //
       old.virtual_cpu_limit != curr.virtual_cpu_limit;
}

bool include_delta(const chain::account_metadata_object& old, const chain::account_metadata_object& curr) {
   return                                               //
       old.name != curr.name ||                         //
       old.is_privileged() != curr.is_privileged() ||   //
       old.last_code_update != curr.last_code_update || //
       old.vm_type != curr.vm_type ||                   //
       old.vm_version != curr.vm_version ||             //
       old.code_hash != curr.code_hash;
}

bool include_delta(const chain::code_object& old, const chain::code_object& curr) { //
   return false;
}

bool include_delta(const chain::protocol_state_object& old, const chain::protocol_state_object& curr) {
   return old.activated_protocol_features != curr.activated_protocol_features;
}

std::vector<table_delta> create_deltas(const chainbase::database& db, bool full_snapshot) {
   std::vector<table_delta>                          deltas;
   const auto&                                       table_id_index = db.get_index<chain::table_id_multi_index>();
   std::map<uint64_t, const chain::table_id_object*> removed_table_id;
   for (auto& rem : table_id_index.last_undo_session().removed_values)
      removed_table_id[rem.id._id] = &rem;

   auto get_table_id = [&](uint64_t tid) -> const chain::table_id_object& {
      auto obj = table_id_index.find(tid);
      if (obj)
         return *obj;
      auto it = removed_table_id.find(tid);
      EOS_ASSERT(it != removed_table_id.end(), chain::plugin_exception, "can not found table id ${tid}", ("tid", tid));
      return *it->second;
   };

   auto pack_row          = [&](auto& row) { return fc::raw::pack(make_history_serial_wrapper(db, row)); };
   auto pack_contract_row = [&](auto& row) {
      return fc::raw::pack(make_history_context_wrapper(db, get_table_id(row.t_id._id), row));
   };

   auto process_table = [&](auto* name, auto& index, auto& pack_row) {
      if (full_snapshot) {
         if (index.indices().empty())
            return;
         deltas.push_back({});
         auto& delta = deltas.back();
         delta.name  = name;
         for (auto& row : index.indices())
            delta.rows.obj.emplace_back(true, pack_row(row));
      } else {
         auto undo = index.last_undo_session();
         if (undo.old_values.empty() && undo.new_values.empty() && undo.removed_values.empty())
            return;
         deltas.push_back({});
         auto& delta = deltas.back();
         delta.name  = name;
         for (auto& old : undo.old_values) {
            auto& row = index.get(old.id);
            if (include_delta(old, row))
               delta.rows.obj.emplace_back(true, pack_row(row));
         }
         for (auto& old : undo.removed_values)
            delta.rows.obj.emplace_back(false, pack_row(old));
         for (auto& row : undo.new_values) {
            delta.rows.obj.emplace_back(true, pack_row(row));
         }
      }
   };

   process_table("account", db.get_index<chain::account_index>(), pack_row);
   process_table("account_metadata", db.get_index<chain::account_metadata_index>(), pack_row);
   process_table("code", db.get_index<chain::code_index>(), pack_row);

   process_table("contract_table", db.get_index<chain::table_id_multi_index>(), pack_row);
   process_table("contract_row", db.get_index<chain::key_value_index>(), pack_contract_row);
   process_table("contract_index64", db.get_index<chain::index64_index>(), pack_contract_row);
   process_table("contract_index128", db.get_index<chain::index128_index>(), pack_contract_row);
   process_table("contract_index256", db.get_index<chain::index256_index>(), pack_contract_row);
   process_table("contract_index_double", db.get_index<chain::index_double_index>(), pack_contract_row);
   process_table("contract_index_long_double", db.get_index<chain::index_long_double_index>(), pack_contract_row);

   process_table("key_value", db.get_index<chain::kv_index>(), pack_row);

   process_table("global_property", db.get_index<chain::global_property_multi_index>(), pack_row);
   process_table("generated_transaction", db.get_index<chain::generated_transaction_multi_index>(), pack_row);
   process_table("protocol_state", db.get_index<chain::protocol_state_multi_index>(), pack_row);

   process_table("permission", db.get_index<chain::permission_index>(), pack_row);
   process_table("permission_link", db.get_index<chain::permission_link_index>(), pack_row);

   process_table("resource_limits", db.get_index<chain::resource_limits::resource_limits_index>(), pack_row);
   process_table("resource_usage", db.get_index<chain::resource_limits::resource_usage_index>(), pack_row);
   process_table("resource_limits_state", db.get_index<chain::resource_limits::resource_limits_state_index>(),
                 pack_row);
   process_table("resource_limits_config", db.get_index<chain::resource_limits::resource_limits_config_index>(),
                 pack_row);

   return deltas;
}

using prim_store = std::vector<std::pair<int, chain::backing_store::primary_index_view>>;

template<typename SecKey>
using sec_store = std::vector<std::pair<int, chain::backing_store::secondary_index_view<SecKey>>>;

using table_key = chain::backing_store::table_id_object_view;

using table_store = std::vector<table_key>;

struct rocksdb_receiver{
   void add_row(const chain::backing_store::primary_index_view& row, const chainbase::database& db){
      prim.push_back({table_st.size()-1, row});
   }

   void add_row(const chain::backing_store::secondary_index_view<uint64_t>& row, const chainbase::database& db){
      sec_i64.push_back({table_st.size()-1, row});
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::uint128_t>& row, const chainbase::database& db){
      sec_i128.push_back({table_st.size()-1, row});
   }

   void add_row(const chain::backing_store::secondary_index_view<chain::key256_t>& row, const chainbase::database& db){
      sec_i256.push_back({table_st.size()-1, row});
   }

   void add_row(const chain::backing_store::secondary_index_view<float64_t>& row, const chainbase::database& db){
      sec_f64.push_back({table_st.size()-1, row});
   }

   void add_row(const chain::backing_store::secondary_index_view<float128_t>& row, const chainbase::database& db){
      sec_f128.push_back({table_st.size()-1, row});
   }

   void add_row(const chain::backing_store::table_id_object_view& row, const chainbase::database& db){
      table_st.emplace_back(row);
   }

   void add_row(fc::unsigned_int x, const chainbase::database& db){

   }

   table_store table_st;
   prim_store prim;
   sec_store<uint64_t> sec_i64;
   sec_store<chain::uint128_t> sec_i128;
   sec_store<chain::key256_t> sec_i256;
   sec_store<float64_t> sec_f64;
   sec_store<float128_t> sec_f128;
};

std::vector<table_delta> create_deltas_rocksdb(eosio::session::undo_stack<chain::rocks_db_type> &db, rocksdb_receiver &rec){
   std::vector<table_delta>                          deltas;

   auto pack_row          = [&](auto& row) { return fc::raw::pack(make_history_serial_wrapper(db, row)); };
   auto pack_contract_row = [&](auto& row) {
      return fc::raw::pack(make_history_context_wrapper(db, rec.table_st[row.first], row.second));
   };

   auto process_table_full_snapshot = [&](auto *name, auto &indices, auto &pack_row) {
      if (indices.empty())
         return;
      deltas.push_back({});
      auto& delta = deltas.back();
      delta.name  = name;
      for (auto& row : indices)
         delta.rows.obj.emplace_back(true, pack_row(row));
   };

   process_table_full_snapshot("contract_table", rec.table_st, pack_row);
   process_table_full_snapshot("contract_row", rec.prim, pack_contract_row);
   process_table_full_snapshot("contract_index64", rec.sec_i64, pack_contract_row);
   process_table_full_snapshot("contract_index128", rec.sec_i128, pack_contract_row);
   process_table_full_snapshot("contract_index256", rec.sec_i256, pack_contract_row);
   process_table_full_snapshot("contract_index_double", rec.sec_f64, pack_contract_row);
   process_table_full_snapshot("contract_index_long_double", rec.sec_f128, pack_contract_row);

   return deltas;
}

std::vector<table_delta> create_deltas(const chain::combined_database& db, bool full_snapshot){
   std::vector<table_delta>                          deltas_result;
   rocksdb_receiver rec;

   chain::backing_store::rocksdb_contract_db_table_writer writer(rec, db.get_db(), *db.get_kv_undo_stack());

   const std::vector<char> rocksdb_contract_db_prefix{ 0x12 }; // for DB API

   chain::backing_store::walk_rocksdb_entries_with_prefix(db.get_kv_undo_stack(), rocksdb_contract_db_prefix, writer);

   deltas_result = create_deltas_rocksdb(*db.get_kv_undo_stack(), rec);

   return create_deltas(db.get_db(), full_snapshot);
}

} // namespace state_history
} // namespace eosio
