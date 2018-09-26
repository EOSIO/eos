#include <eosio/chain/account_object.hpp>
#include <eosio/chain/block_summary_object.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/resource_limits_private.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/transaction_object.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/state_history_plugin/state_history_plugin.hpp>

#include <boost/signals2/connection.hpp>

template <typename T>
struct history_serial_wrapper {
   const T& obj;
};

template <typename T>
static history_serial_wrapper<T> make_history_serial_wrapper(const T& obj) {
   return {obj};
}

namespace fc {

template <typename T>
static const T& as_type(const T& x) {
   return x;
}

template <typename ST, typename T>
static void serialize_shared_vector(datastream<ST>& ds, const T& v) {
   ds << fc::unsigned_int(v.size());
   for (auto& x : v)
      ds << make_history_serial_wrapper(x);
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>& ds, const history_serial_wrapper<eosio::chain::account_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.name.value));
   fc::raw::pack(ds, as_type<uint8_t>(obj.obj.vm_type));
   fc::raw::pack(ds, as_type<uint8_t>(obj.obj.vm_version));
   fc::raw::pack(ds, as_type<bool>(obj.obj.privileged));
   fc::raw::pack(ds, as_type<fc::time_point>(obj.obj.last_code_update));
   fc::raw::pack(ds, as_type<eosio::chain::digest_type>(obj.obj.code_version));
   fc::raw::pack(ds, as_type<eosio::chain::block_timestamp_type>(obj.obj.creation_date));
   fc::raw::pack(ds, as_type<eosio::chain::shared_string>(obj.obj.code));
   fc::raw::pack(ds, as_type<eosio::chain::shared_string>(obj.obj.abi));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                      ds,
                                  const history_serial_wrapper<eosio::chain::account_sequence_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.name.value));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.recv_sequence));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.auth_sequence));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.code_sequence));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.abi_sequence));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                              ds,
                                  const history_serial_wrapper<eosio::chain::table_id_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.code.value));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.scope.value));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.table.value));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.payer.value));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.count));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                               ds,
                                  const history_serial_wrapper<eosio::chain::key_value_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.t_id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.primary_key));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.payer.value));
   fc::raw::pack(ds, as_type<eosio::chain::shared_string>(obj.obj.value));
   return ds;
}

template <typename ST, typename T>
static void serialize_secondary_index_data(datastream<ST>& ds, const T& obj) {
   fc::raw::pack(ds, obj);
}

template <typename ST>
static void serialize_secondary_index_data(datastream<ST>& ds, const float64_t& obj) {
   uint64_t i;
   memcpy(&i, &obj, sizeof(i));
   fc::raw::pack(ds, i);
}

template <typename ST>
static void serialize_secondary_index_data(datastream<ST>& ds, const float128_t& obj) {
   __uint128_t i;
   memcpy(&i, &obj, sizeof(i));
   fc::raw::pack(ds, i);
}

template <typename ST>
static void serialize_secondary_index_data(datastream<ST>& ds, const eosio::chain::key256_t& obj) {
   fc::raw::pack(ds, obj[0]);
   fc::raw::pack(ds, obj[1]);
}

template <typename ST, typename T>
static datastream<ST>& serialize_secondary_index(datastream<ST>& ds, const T& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.t_id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.primary_key));
   fc::raw::pack(ds, as_type<uint64_t>(obj.payer.value));
   serialize_secondary_index_data(ds, obj.secondary_key);
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>& ds, const history_serial_wrapper<eosio::chain::index64_object>& obj) {
   return serialize_secondary_index(ds, obj.obj);
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                              ds,
                                  const history_serial_wrapper<eosio::chain::index128_object>& obj) {
   return serialize_secondary_index(ds, obj.obj);
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                              ds,
                                  const history_serial_wrapper<eosio::chain::index256_object>& obj) {
   return serialize_secondary_index(ds, obj.obj);
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                  ds,
                                  const history_serial_wrapper<eosio::chain::index_double_object>& obj) {
   return serialize_secondary_index(ds, obj.obj);
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                       ds,
                                  const history_serial_wrapper<eosio::chain::index_long_double_object>& obj) {
   return serialize_secondary_index(ds, obj.obj);
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>& ds, const history_serial_wrapper<eosio::chain::producer_key>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.producer_name.value));
   fc::raw::pack(ds, as_type<eosio::chain::public_key_type>(obj.obj.block_signing_key));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                            ds,
                                  const history_serial_wrapper<eosio::chain::shared_producer_schedule_type>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.version));
   serialize_shared_vector(ds, obj.obj.producers);
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>& ds, const history_serial_wrapper<eosio::chain::chain_config>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.max_block_net_usage));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.target_block_net_usage_pct));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.max_transaction_net_usage));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.base_per_transaction_net_usage));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.net_usage_leeway));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.context_free_discount_net_usage_num));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.context_free_discount_net_usage_den));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.max_block_cpu_usage));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.target_block_cpu_usage_pct));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.max_transaction_cpu_usage));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.min_transaction_cpu_usage));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.max_transaction_lifetime));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.deferred_trx_expiration_window));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.max_transaction_delay));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.max_inline_action_size));
   fc::raw::pack(ds, as_type<uint16_t>(obj.obj.max_inline_action_depth));
   fc::raw::pack(ds, as_type<uint16_t>(obj.obj.max_authority_depth));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                     ds,
                                  const history_serial_wrapper<eosio::chain::global_property_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));

   fc::raw::pack(ds, as_type<optional<eosio::chain::block_num_type>>(obj.obj.proposed_schedule_block_num));
   fc::raw::pack(ds, make_history_serial_wrapper(
                         as_type<eosio::chain::shared_producer_schedule_type>(obj.obj.proposed_schedule)));
   fc::raw::pack(ds, make_history_serial_wrapper(as_type<eosio::chain::chain_config>(obj.obj.configuration)));

   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                             ds,
                                  const history_serial_wrapper<eosio::chain::dynamic_global_property_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.global_action_sequence));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                   ds,
                                  const history_serial_wrapper<eosio::chain::block_summary_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<eosio::chain::block_id_type>(obj.obj.block_id));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                 ds,
                                  const history_serial_wrapper<eosio::chain::transaction_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<fc::time_point_sec>(obj.obj.expiration));
   fc::raw::pack(ds, as_type<eosio::chain::transaction_id_type>(obj.obj.trx_id));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                           ds,
                                  const history_serial_wrapper<eosio::chain::generated_transaction_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<eosio::chain::transaction_id_type>(obj.obj.trx_id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.sender.value));
   fc::raw::pack(ds, as_type<__uint128_t>(obj.obj.sender_id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.payer.value));
   fc::raw::pack(ds, as_type<fc::time_point>(obj.obj.delay_until));
   fc::raw::pack(ds, as_type<fc::time_point>(obj.obj.expiration));
   fc::raw::pack(ds, as_type<fc::time_point>(obj.obj.published));
   fc::raw::pack(ds, as_type<eosio::chain::shared_string>(obj.obj.packed_trx));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>& ds, const history_serial_wrapper<eosio::chain::key_weight>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<eosio::chain::public_key_type>(obj.obj.key));
   fc::raw::pack(ds, as_type<uint16_t>(obj.obj.weight));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                               ds,
                                  const history_serial_wrapper<eosio::chain::permission_level>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.actor.value));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.permission.value));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                      ds,
                                  const history_serial_wrapper<eosio::chain::permission_level_weight>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<eosio::chain::permission_level>(obj.obj.permission));
   fc::raw::pack(ds, as_type<uint16_t>(obj.obj.weight));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>& ds, const history_serial_wrapper<eosio::chain::wait_weight>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.wait_sec));
   fc::raw::pack(ds, as_type<uint16_t>(obj.obj.weight));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                               ds,
                                  const history_serial_wrapper<eosio::chain::shared_authority>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.threshold));
   serialize_shared_vector(ds, obj.obj.keys);
   serialize_shared_vector(ds, obj.obj.accounts);
   serialize_shared_vector(ds, obj.obj.waits);
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                ds,
                                  const history_serial_wrapper<eosio::chain::permission_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.usage_id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.parent._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.owner.value));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.name.value));
   fc::raw::pack(ds, as_type<fc::time_point>(obj.obj.last_updated));
   fc::raw::pack(ds, as_type<eosio::chain::shared_authority>(obj.obj.auth));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                      ds,
                                  const history_serial_wrapper<eosio::chain::permission_usage_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<fc::time_point>(obj.obj.last_used));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                     ds,
                                  const history_serial_wrapper<eosio::chain::permission_link_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.account.value));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.code.value));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.message_type.value));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.required_permission.value));
   return ds;
}

template <typename ST>
static datastream<ST>&
operator<<(datastream<ST>&                                                                      ds,
           const history_serial_wrapper<eosio::chain::resource_limits::resource_limits_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.owner.value));
   fc::raw::pack(ds, as_type<bool>(obj.obj.pending));
   fc::raw::pack(ds, as_type<int64_t>(obj.obj.net_weight));
   fc::raw::pack(ds, as_type<int64_t>(obj.obj.cpu_weight));
   fc::raw::pack(ds, as_type<int64_t>(obj.obj.ram_bytes));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                                 ds,
                                  const history_serial_wrapper<eosio::chain::resource_limits::usage_accumulator>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.last_ordinal));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.value_ex));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.consumed));
   return ds;
}

template <typename ST>
static datastream<ST>&
operator<<(datastream<ST>&                                                                     ds,
           const history_serial_wrapper<eosio::chain::resource_limits::resource_usage_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.owner.value));
   fc::raw::pack(
       ds, make_history_serial_wrapper(as_type<eosio::chain::resource_limits::usage_accumulator>(obj.obj.net_usage)));
   fc::raw::pack(
       ds, make_history_serial_wrapper(as_type<eosio::chain::resource_limits::usage_accumulator>(obj.obj.cpu_usage)));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.ram_usage));
   return ds;
}

template <typename ST>
static datastream<ST>&
operator<<(datastream<ST>&                                                                            ds,
           const history_serial_wrapper<eosio::chain::resource_limits::resource_limits_state_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, make_history_serial_wrapper(
                         as_type<eosio::chain::resource_limits::usage_accumulator>(obj.obj.average_block_net_usage)));
   fc::raw::pack(ds, make_history_serial_wrapper(
                         as_type<eosio::chain::resource_limits::usage_accumulator>(obj.obj.average_block_cpu_usage)));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.pending_net_usage));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.pending_cpu_usage));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.total_net_weight));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.total_cpu_weight));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.total_ram_bytes));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.virtual_net_limit));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.virtual_cpu_limit));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                     ds,
                                  const history_serial_wrapper<eosio::chain::resource_limits::ratio>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.numerator));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.denominator));
   return ds;
}

template <typename ST>
static datastream<ST>&
operator<<(datastream<ST>&                                                                        ds,
           const history_serial_wrapper<eosio::chain::resource_limits::elastic_limit_parameters>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.target));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.max));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.periods));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.max_multiplier));
   fc::raw::pack(ds, make_history_serial_wrapper(as_type<eosio::chain::resource_limits::ratio>(obj.obj.contract_rate)));
   fc::raw::pack(ds, make_history_serial_wrapper(as_type<eosio::chain::resource_limits::ratio>(obj.obj.expand_rate)));
   return ds;
}

template <typename ST>
static datastream<ST>&
operator<<(datastream<ST>&                                                                             ds,
           const history_serial_wrapper<eosio::chain::resource_limits::resource_limits_config_object>& obj) {
   ds << fc::unsigned_int(0);
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, make_history_serial_wrapper(as_type<eosio::chain::resource_limits::elastic_limit_parameters>(
                         obj.obj.cpu_limit_parameters)));
   fc::raw::pack(ds, make_history_serial_wrapper(as_type<eosio::chain::resource_limits::elastic_limit_parameters>(
                         obj.obj.net_limit_parameters)));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.account_cpu_usage_average_window));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.account_net_usage_average_window));
   return ds;
};

} // namespace fc

namespace eosio {
using namespace chain;
using boost::signals2::scoped_connection;

static appbase::abstract_plugin& _state_history_plugin = app().register_plugin<state_history_plugin>();

struct state_history_object : public chainbase::object<state_history_object_type, state_history_object> {
   OBJECT_CTOR(state_history_object, (deltas))

   id_type       id;
   shared_string deltas;
};

using state_history_index = chainbase::shared_multi_index_container<
    state_history_object,
    indexed_by<ordered_unique<tag<by_id>,
                              member<state_history_object, state_history_object::id_type, &state_history_object::id>>>>;

} // namespace eosio

CHAINBASE_SET_INDEX_TYPE(eosio::state_history_object, eosio::state_history_index)

namespace eosio {

template <typename F>
static void for_each_table(chainbase::database& db, F f) {
   f("account_index", db.get_index<account_index>());
   f("account_sequence_index", db.get_index<account_sequence_index>());

   f("table_id_multi_index", db.get_index<table_id_multi_index>());
   f("key_value_index", db.get_index<key_value_index>());
   f("index64_index", db.get_index<index64_index>());
   f("index128_index", db.get_index<index128_index>());
   f("index256_index", db.get_index<index256_index>());
   f("index_double_index", db.get_index<index_double_index>());
   f("index_long_double_index", db.get_index<index_long_double_index>());

   f("global_property_multi_index", db.get_index<global_property_multi_index>());
   f("dynamic_global_property_multi_index", db.get_index<dynamic_global_property_multi_index>());
   f("block_summary_multi_index", db.get_index<block_summary_multi_index>());
   f("transaction_multi_index", db.get_index<transaction_multi_index>());
   f("generated_transaction_multi_index", db.get_index<generated_transaction_multi_index>());

   f("permission_index", db.get_index<permission_index>());
   f("permission_usage_index", db.get_index<permission_usage_index>());
   f("permission_link_index", db.get_index<permission_link_index>());

   f("resource_limits_index", db.get_index<resource_limits::resource_limits_index>());
   f("resource_usage_index", db.get_index<resource_limits::resource_usage_index>());
   f("resource_limits_state_index", db.get_index<resource_limits::resource_limits_state_index>());
   f("resource_limits_config_index", db.get_index<resource_limits::resource_limits_config_index>());
}

class state_history_plugin_impl {
 public:
   chain_plugin*                   chain_plug = nullptr;
   fc::optional<scoped_connection> accepted_block_connection;

   void on_accepted_block(const block_state_ptr& block_state) {
      auto& chain = chain_plug->chain();
      auto& db    = chain.db();
      dlog("${i}: ${n} transactions",
           ("i", block_state->block->block_num())("n", block_state->block->transactions.size()));
      db.create<state_history_object>([&](state_history_object& hist) {
         hist.id = block_state->block->block_num();
         std::vector<table_delta> deltas;
         for_each_table(db, [&](auto* name, auto& index) {
            if (index.stack().empty())
               return;
            auto& undo = index.stack().back();
            if (undo.old_values.empty() && undo.new_ids.empty() && undo.removed_values.empty())
               return;
            deltas.push_back({});
            auto& delta = deltas.back();
            delta.name  = name;
            dlog("    ${name} rev=${r} old=${o} new=${n} rem=${rem}",
                 ("name", name)("r", undo.revision)("o", undo.old_values.size())("n", undo.new_ids.size())(
                     "rem", undo.removed_values.size()));
            for (auto& old : undo.old_values)
               delta.rows.emplace_back(old.first._id, fc::raw::pack(make_history_serial_wrapper(index.get(old.first))));
            for (auto id : undo.new_ids)
               delta.rows.emplace_back(id._id, fc::raw::pack(make_history_serial_wrapper(index.get(id))));
            for (auto& old : undo.removed_values)
               delta.removed.push_back(old.first._id);
         });
         auto bin = fc::raw::pack(deltas);
         dlog("    ${s} bytes", ("s", bin.size()));
         hist.deltas.insert(hist.deltas.end(), bin.begin(), bin.end());
      });
   }
};

state_history_plugin::state_history_plugin()
    : my(std::make_shared<state_history_plugin_impl>()) {}

state_history_plugin::~state_history_plugin() {}

void state_history_plugin::set_program_options(options_description& cli, options_description& cfg) {}

void state_history_plugin::plugin_initialize(const variables_map& options) {
   try {
      my->chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT(my->chain_plug, chain::missing_chain_plugin_exception, "");
      auto& chain = my->chain_plug->chain();
      chain.db().add_index<state_history_index>();
      my->accepted_block_connection.emplace(
          chain.accepted_block.connect([&](const block_state_ptr& p) { my->on_accepted_block(p); }));
   }
   FC_LOG_AND_RETHROW()
}

void state_history_plugin::plugin_startup() {}

void state_history_plugin::plugin_shutdown() { my->accepted_block_connection.reset(); }

} // namespace eosio
