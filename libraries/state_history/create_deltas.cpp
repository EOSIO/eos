#include <eosio/state_history/create_deltas.hpp>
#include <eosio/state_history/serialization.hpp>

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

void create_deltas(const chainbase::database& db, bool full_snapshot, std::vector<table_delta>& deltas) {
   const auto& table_id_index = db.get_index<chain::table_id_multi_index>();
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

template <typename T>
void create_deltas(const T& db, bool full_snapshot, std::vector<table_delta>& deltas) {
   const auto get_db_typeT = [](const auto& bytes) { return bytes[0]; };
   const auto get_code = [](const auto& bytes) {
      chain::name c;
      std::memcpy(&c,&bytes[1], sizeof(name));
      return c;
   };
   const auto get_scope = [](const auto& bytes) {
      uint64_t s;
      std::memcpy(&s, &bytes[9], sizeof(s));
      return s;
   };
   const auto get_table = [](const auto& bytes) {
      chain::name t;
      std::memcpy(&t, &bytes[17], sizeof(t));
      return t;
   };
   const auto generate_delta = [&](std::string_view nm, auto&& obj, bool removed=false) {
     auto& delta = deltas.back();
     delta.name  = name;
     delta.rows.obj.emplace_back(!removed, pack_row(std::move(obj)));
   };

   table_id_object last_found;
   const auto process_deltas = [&](const auto& keys, bool removed) {
      for (const auto elem : keys) {
         table_id_object tid = {0, get_code(elem),
                                   get_scope(elem),
                                   get_table(elem) };
         if (tid != last_found)
            generate_delta("contract_table", tid);

         key_value_objet kvo = {tid, get_payer(elem), get_primary_key(elem), get_payload(elem)};
      }
   };

   process_deltas(db.removed_keys(), true);
   process_deltas(db.updated_keys(), false);
}

std::vector<table_delta> create_deltas(const chain::combined_database_wrapper& db, bool full_snapshot) {
   std::vector<table_delta> deltas;
   create_deltas(db.chainbase_db(), full_snapshot, deltas);
   create_deltas(db.kv_undo_stack_db(), full_snapshot, deltas);
   return deltas;
}

} // namespace state_history
} // namespace eosio
