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

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/signals2/connection.hpp>

using tcp    = boost::asio::ip::tcp;
namespace ws = boost::beast::websocket;

extern const char* const state_history_plugin_abi;

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
   fc::raw::pack(ds, unsigned_int(v.size()));
   for (auto& x : v)
      ds << make_history_serial_wrapper(x);
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>& ds, const history_serial_wrapper<eosio::chain::account_object>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.producer_name.value));
   fc::raw::pack(ds, as_type<eosio::chain::public_key_type>(obj.obj.block_signing_key));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                            ds,
                                  const history_serial_wrapper<eosio::chain::shared_producer_schedule_type>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.version));
   serialize_shared_vector(ds, obj.obj.producers);
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>& ds, const history_serial_wrapper<eosio::chain::chain_config>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.global_action_sequence));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                   ds,
                                  const history_serial_wrapper<eosio::chain::block_summary_object>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<eosio::chain::block_id_type>(obj.obj.block_id));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                 ds,
                                  const history_serial_wrapper<eosio::chain::transaction_object>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<fc::time_point_sec>(obj.obj.expiration));
   fc::raw::pack(ds, as_type<eosio::chain::transaction_id_type>(obj.obj.trx_id));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                           ds,
                                  const history_serial_wrapper<eosio::chain::generated_transaction_object>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<eosio::chain::public_key_type>(obj.obj.key));
   fc::raw::pack(ds, as_type<uint16_t>(obj.obj.weight));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                               ds,
                                  const history_serial_wrapper<eosio::chain::permission_level>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.actor.value));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.permission.value));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                      ds,
                                  const history_serial_wrapper<eosio::chain::permission_level_weight>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, make_history_serial_wrapper(as_type<eosio::chain::permission_level>(obj.obj.permission)));
   fc::raw::pack(ds, as_type<uint16_t>(obj.obj.weight));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>& ds, const history_serial_wrapper<eosio::chain::wait_weight>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.wait_sec));
   fc::raw::pack(ds, as_type<uint16_t>(obj.obj.weight));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                               ds,
                                  const history_serial_wrapper<eosio::chain::shared_authority>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.threshold));
   serialize_shared_vector(ds, obj.obj.keys);
   serialize_shared_vector(ds, obj.obj.accounts);
   serialize_shared_vector(ds, obj.obj.waits);
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                ds,
                                  const history_serial_wrapper<eosio::chain::permission_object>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.usage_id._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.parent._id));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.owner.value));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.name.value));
   fc::raw::pack(ds, as_type<fc::time_point>(obj.obj.last_updated));
   fc::raw::pack(ds, make_history_serial_wrapper(as_type<eosio::chain::shared_authority>(obj.obj.auth)));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                      ds,
                                  const history_serial_wrapper<eosio::chain::permission_usage_object>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.id._id));
   fc::raw::pack(ds, as_type<fc::time_point>(obj.obj.last_used));
   return ds;
}

template <typename ST>
static datastream<ST>& operator<<(datastream<ST>&                                                     ds,
                                  const history_serial_wrapper<eosio::chain::permission_link_object>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint32_t>(obj.obj.last_ordinal));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.value_ex));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.consumed));
   return ds;
}

template <typename ST>
static datastream<ST>&
operator<<(datastream<ST>&                                                                     ds,
           const history_serial_wrapper<eosio::chain::resource_limits::resource_usage_object>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.numerator));
   fc::raw::pack(ds, as_type<uint64_t>(obj.obj.denominator));
   return ds;
}

template <typename ST>
static datastream<ST>&
operator<<(datastream<ST>&                                                                        ds,
           const history_serial_wrapper<eosio::chain::resource_limits::elastic_limit_parameters>& obj) {
   fc::raw::pack(ds, fc::unsigned_int(0));
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
   fc::raw::pack(ds, fc::unsigned_int(0));
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

enum class state_history_db_object_type {
   null_object_type = 0,
   state_history_object_type,
};

struct state_history_object
    : public chainbase::object<(uint16_t)state_history_db_object_type::state_history_object_type,
                               state_history_object> {
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
static void for_each_table(const chainbase::database& db, F f) {
   f("account", db.get_index<account_index>(), [](auto&) { return true; });
   f("account_sequence", db.get_index<account_sequence_index>(), [](auto&) { return true; });

   f("table_id", db.get_index<table_id_multi_index>(), [](auto&) { return true; });
   f("key_value", db.get_index<key_value_index>(), [](auto&) { return true; });
   f("index64", db.get_index<index64_index>(), [](auto&) { return true; });
   f("index128", db.get_index<index128_index>(), [](auto&) { return true; });
   f("index256", db.get_index<index256_index>(), [](auto&) { return true; });
   f("index_double", db.get_index<index_double_index>(), [](auto&) { return true; });
   f("index_long_double", db.get_index<index_long_double_index>(), [](auto&) { return true; });

   f("global_property", db.get_index<global_property_multi_index>(), [](auto&) { return true; });
   f("dynamic_global_property", db.get_index<dynamic_global_property_multi_index>(), [](auto&) { return true; });
   f("block_summary", db.get_index<block_summary_multi_index>(),
     [](auto& row) { return row.block_id != block_id_type(); });
   f("transaction", db.get_index<transaction_multi_index>(), [](auto&) { return true; });
   f("generated_transaction", db.get_index<generated_transaction_multi_index>(), [](auto&) { return true; });

   f("permission", db.get_index<permission_index>(), [](auto&) { return true; });
   f("permission_usage", db.get_index<permission_usage_index>(), [](auto&) { return true; });
   f("permission_link", db.get_index<permission_link_index>(), [](auto&) { return true; });

   f("resource_limits", db.get_index<resource_limits::resource_limits_index>(), [](auto&) { return true; });
   f("resource_usage", db.get_index<resource_limits::resource_usage_index>(), [](auto&) { return true; });
   f("resource_limits_state", db.get_index<resource_limits::resource_limits_state_index>(), [](auto&) { return true; });
   f("resource_limits_config", db.get_index<resource_limits::resource_limits_config_index>(),
     [](auto&) { return true; });
}

template <typename F>
auto catch_and_log(F f) {
   try {
      return f();
   } catch (const fc::exception& e) {
      elog("${e}", ("e", e.to_detail_string()));
   } catch (const std::exception& e) {
      elog("${e}", ("e", e.what()));
   } catch (...) {
      elog("unknown exception");
   }
}

/*
 *   *.log:
 *   +---------+----------------+-----------+------------------+-----+---------+----------------+
 *   | Entry i | Pos of Entry i | Entry i+1 | Pos of Entry i+1 | ... | Entry z | Pos of Entry z |
 *   +---------+----------------+-----------+------------------+-----+---------+----------------+
 *
 *   *.index:
 *   +----------------+------------------+-----+----------------+
 *   | Pos of Entry i | Pos of Entry i+1 | ... | Pos of Entry z |
 *   +----------------+------------------+-----+----------------+
 *
 * each entry:
 *    uint32_t    block_num
 *    uint64_t    size of payload
 *    uint8_t     version
 *                payload
 *
 * irreversible_state payload:
 *    uint32_t    size of deltas
 *    char[]      deltas
 */

struct history_log_header {
   uint32_t block_num    = 0;
   uint64_t payload_size = 0;
   uint16_t version      = 0;
};

struct history_log {
   const char* const name = "";
   std::fstream      log;
   std::fstream      index;
   uint32_t          begin_block = 0;
   uint32_t          end_block   = 0;

   void open_log(const std::string& filename) {
      log.open(filename, std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app);
      uint64_t size = log.tellg();
      if (size >= sizeof(history_log_header)) {
         history_log_header header;
         log.seekg(0);
         log.read((char*)&header, sizeof(header));
         EOS_ASSERT(header.version == 0 && sizeof(header) + header.payload_size + sizeof(uint64_t) <= size,
                    plugin_exception, "corrupt ${name}.log (1)", ("name", name));
         begin_block = header.block_num;

         uint64_t end_pos;
         log.seekg(size - sizeof(end_pos));
         log.read((char*)&end_pos, sizeof(end_pos));
         EOS_ASSERT(end_pos <= size && end_pos + sizeof(header) <= size, plugin_exception, "corrupt ${name}.log (2)",
                    ("name", name));
         log.seekg(end_pos);
         log.read((char*)&header, sizeof(header));
         EOS_ASSERT(end_pos + sizeof(header) + header.payload_size + sizeof(uint64_t) == size, plugin_exception,
                    "corrupt ${name}.log (3)", ("name", name));
         end_block = header.block_num + 1;

         EOS_ASSERT(begin_block < end_block, plugin_exception, "corrupt ${name}.log (4)", ("name", name));
         ilog("${name}.log has blocks ${b}-${e}", ("name", name)("b", begin_block)("e", end_block - 1));
      } else {
         EOS_ASSERT(!size, plugin_exception, "corrupt ${name}.log (5)", ("name", name));
         ilog("${name}.log is empty", ("name", name));
      }
   }

   void open_index(const std::string& filename) {
      index.open(filename, std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::app);
      if (index.tellg() == (end_block - begin_block) * sizeof(uint64_t))
         return;
      ilog("Regenerate ${name}.index", ("name", name));
      index.close();
      index.open(filename, std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

      log.seekg(0, std::ios_base::end);
      uint64_t size = log.tellg();
      uint64_t pos  = 0;
      while (pos < size) {
         index.write((char*)&pos, sizeof(pos));
         history_log_header header;
         EOS_ASSERT(pos + sizeof(header) <= size, plugin_exception, "corrupt ${name}.log (6)", ("name", name));
         log.seekg(pos);
         log.read((char*)&header, sizeof(header));
         uint64_t suffix_pos = pos + sizeof(header) + header.payload_size;
         uint64_t suffix;
         EOS_ASSERT(suffix_pos + sizeof(suffix) <= size, plugin_exception, "corrupt ${name}.log (7)", ("name", name));
         log.seekg(suffix_pos);
         log.read((char*)&suffix, sizeof(suffix));
         // ilog("block ${b} at ${pos}-${end} suffix=${suffix} file_size=${fs}",
         //      ("b", header.block_num)("pos", pos)("end", suffix_pos + sizeof(suffix))("suffix", suffix)("fs", size));
         EOS_ASSERT(suffix == pos, plugin_exception, "corrupt ${name}.log (8)", ("name", name));
         pos = suffix_pos + sizeof(suffix);
      }
   }

   template <typename F>
   void write_entry(history_log_header header, F write_payload) {
      EOS_ASSERT(begin_block == end_block || header.block_num == end_block, plugin_exception,
                 "writing unexpected block_num to ${name}.log", ("name", name));
      log.seekg(0, std::ios_base::end);
      uint64_t pos = log.tellg();
      log.write((char*)&header, sizeof(header));
      write_payload(log);
      uint64_t end = log.tellg();
      EOS_ASSERT(end == pos + sizeof(header) + header.payload_size, plugin_exception,
                 "wrote payload with incorrect size to ${name}.log", ("name", name));
      log.write((char*)&pos, sizeof(pos));

      index.seekg(0, std::ios_base::end);
      index.write((char*)&pos, sizeof(pos));
      if (begin_block == end_block)
         begin_block = header.block_num;
      end_block = header.block_num + 1;
   }

   // returns stream positioned at payload
   std::fstream& get_entry(uint32_t block_num, history_log_header& header) {
      EOS_ASSERT(block_num >= begin_block && block_num < end_block, plugin_exception,
                 "read non-existing block in ${name}.log", ("name", name));
      uint64_t pos;
      index.seekg((block_num - begin_block) * sizeof(pos));
      index.read((char*)&pos, sizeof(pos));
      log.seekg(pos);
      log.read((char*)&header, sizeof(header));
      return log;
   }
}; // history_log

struct state_history_plugin_impl : std::enable_shared_from_this<state_history_plugin_impl> {
   chain_plugin*                        chain_plug = nullptr;
   std::unique_ptr<chainbase::database> db;
   history_log                          state_log{"irreversible_state"};
   fc::optional<scoped_connection>      accepted_block_connection;
   fc::optional<scoped_connection>      irreversible_block_connection;
   string                               endpoint_address = "0.0.0.0";
   uint16_t                             endpoint_port    = 4321;
   std::unique_ptr<tcp::acceptor>       acceptor;

   void open_irreversible(const boost::filesystem::path& path) {
      state_log.open_log((path / "irreversible_state.log").string());
      state_log.open_index((path / "irreversible_state.index").string());
   }

   void get_irreversible_state(uint32_t block_num, bytes& deltas) {
      history_log_header header;
      auto&              stream = state_log.get_entry(block_num, header);
      uint32_t           s;
      stream.read((char*)&s, sizeof(s));
      deltas.resize(s);
      if (s)
         stream.read(deltas.data(), s);
   }

   struct session : std::enable_shared_from_this<session> {
      std::shared_ptr<state_history_plugin_impl> plugin;
      std::unique_ptr<ws::stream<tcp::socket>>   stream;
      bool                                       sending = false;
      std::vector<std::vector<char>>             send_queue;

      session(std::shared_ptr<state_history_plugin_impl> plugin)
          : plugin(std::move(plugin)) {}

      void start(tcp::socket socket) {
         ilog("incoming connection");
         stream = std::make_unique<ws::stream<tcp::socket>>(std::move(socket));
         stream->binary(true);
         stream->next_layer().set_option(boost::asio::ip::tcp::no_delay(true));
         stream->next_layer().set_option(boost::asio::socket_base::send_buffer_size(1024 * 1024));
         stream->next_layer().set_option(boost::asio::socket_base::receive_buffer_size(1024 * 1024));
         stream->async_accept([self = shared_from_this(), this](boost::system::error_code ec) {
            if (!plugin->db)
               return;
            callback(ec, "async_accept", [&] {
               start_read();
               send(state_history_plugin_abi);
            });
         });
      }

      void start_read() {
         auto in_buffer = std::make_shared<boost::beast::flat_buffer>();
         stream->async_read(
             *in_buffer, [self = shared_from_this(), this, in_buffer](boost::system::error_code ec, size_t) {
                if (!plugin->db)
                   return;
                callback(ec, "async_read", [&] {
                   auto d = boost::asio::buffer_cast<char const*>(boost::beast::buffers_front(in_buffer->data()));
                   auto s = boost::asio::buffer_size(in_buffer->data());
                   fc::datastream<const char*> ds(d, s);
                   state_request               req;
                   fc::raw::unpack(ds, req);
                   req.visit(*this);
                   start_read();
                });
             });
      }

      void send(const char* s) {
         // todo: send as string
         send_queue.push_back({s, s + strlen(s)});
         send();
      }

      template <typename T>
      void send(T obj) {
         send_queue.push_back(fc::raw::pack(state_result{std::move(obj)}));
         send();
      }

      void send() {
         if (sending || send_queue.empty())
            return;
         sending = true;
         stream->async_write( //
             boost::asio::buffer(send_queue[0]),
             [self = shared_from_this(), this](boost::system::error_code ec, size_t) {
                if (!plugin->db)
                   return;
                callback(ec, "async_write", [&] {
                   send_queue.erase(send_queue.begin());
                   sending = false;
                   send();
                });
             });
      }

      using result_type = void;
      void operator()(get_state_request_v0&) {
         get_state_result_v0 result;
         auto&               ind = plugin->db->get_index<state_history_index>().indices();
         if (!ind.empty())
            result.last_block_num = (--ind.end())->id._id;
         send(std::move(result));
      }

      void operator()(get_block_request_v0& req) {
         // ilog("${b} get_block_request_v0", ("b", req.block_num));
         get_block_result_v0 result{req.block_num};
         auto&               ind = plugin->db->get_index<state_history_index>().indices();
         auto                it  = ind.find(req.block_num);
         if (it != ind.end()) {
            result.found = true;
            result.deltas.assign(it->deltas.begin(), it->deltas.end());
            // dlog("    bytes: ${b}", ("b", result.deltas));
         } else if (req.block_num >= plugin->state_log.begin_block && req.block_num < plugin->state_log.end_block) {
            plugin->get_irreversible_state(req.block_num, result.deltas);
            result.found = true;
         }
         send(std::move(result));
      }

      template <typename F>
      void catch_and_close(F f) {
         try {
            f();
         } catch (const fc::exception& e) {
            elog("${e}", ("e", e.to_detail_string()));
            close();
         } catch (const std::exception& e) {
            elog("${e}", ("e", e.what()));
            close();
         } catch (...) {
            elog("unknown exception");
            close();
         }
      }

      template <typename F>
      void callback(boost::system::error_code ec, const char* what, F f) {
         if (ec)
            return on_fail(ec, what);
         catch_and_close(f);
      }

      void on_fail(boost::system::error_code ec, const char* what) {
         try {
            elog("${w}: ${m}", ("w", what)("m", ec.message()));
            close();
         } catch (...) {
            elog("uncaught exception on close");
         }
      }

      void close() {
         stream->next_layer().close();
         plugin->sessions.erase(this);
      }
   };
   std::map<session*, std::shared_ptr<session>> sessions;

   void listen() {
      boost::system::error_code ec;
      auto                      address  = boost::asio::ip::make_address(endpoint_address);
      auto                      endpoint = tcp::endpoint{address, endpoint_port};
      acceptor                           = std::make_unique<tcp::acceptor>(app().get_io_service());

      auto check_ec = [&](const char* what) {
         if (!ec)
            return;
         elog("${w}: ${m}", ("w", what)("m", ec.message()));
         EOS_ASSERT(false, plugin_exception, "unable top open listen socket");
      };

      acceptor->open(endpoint.protocol(), ec);
      check_ec("open");
      acceptor->set_option(boost::asio::socket_base::reuse_address(true));
      acceptor->bind(endpoint, ec);
      check_ec("bind");
      acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
      check_ec("listen");
      do_accept();
   }

   void do_accept() {
      auto socket = std::make_shared<tcp::socket>(app().get_io_service());
      acceptor->async_accept(*socket, [self = shared_from_this(), socket, this](auto ec) {
         if (!db)
            return;
         if (ec) {
            if (ec == boost::system::errc::too_many_files_open)
               catch_and_log([&] { do_accept(); });
            return;
         }
         catch_and_log([&] {
            auto s            = std::make_shared<session>(self);
            sessions[s.get()] = s;
            s->start(std::move(*socket));
         });
         do_accept();
      });
   }

   void on_accept(boost::system::error_code ec) {}

   void on_accepted_block(const block_state_ptr& block_state) {
      // ilog("block ${n}", ("n", block_state->block->block_num()));
      auto& chain = chain_plug->chain();
      auto& idx   = db->get_index<state_history_index>();

      // todo: ignore blocks already in irreversible_state.log

      uint64_t num_removed = 0;
      while (!idx.indices().empty() && (--idx.indices().end())->id._id >= block_state->block->block_num()) {
         db->remove(*--idx.indices().end());
         ++num_removed;
      }
      if (num_removed)
         ilog("fork: removed ${n} blocks", ("n", num_removed));
      EOS_ASSERT(idx.indices().empty() || (--idx.indices().end())->id._id + 1 == block_state->block->block_num(),
                 plugin_exception, "missed a fork switch");

      bool fresh = idx.indices().empty();
      if (fresh)
         ilog("Placing initial state in block ${n}", ("n", block_state->block->block_num()));

      db->create<state_history_object>([&](state_history_object& hist) {
         hist.id = block_state->block->block_num();
         std::vector<table_delta> deltas;
         for_each_table(chain.db(), [&, this](auto* name, auto& index, auto filter) {
            if (fresh) {
               if (index.indices().empty())
                  return;
               deltas.push_back({});
               auto& delta = deltas.back();
               delta.name  = name;
               for (auto& row : index.indices())
                  if (filter(row))
                     delta.rows.emplace_back(row.id._id, fc::raw::pack(make_history_serial_wrapper(row)));
            } else {
               if (index.stack().empty())
                  return;
               auto& undo = index.stack().back();
               if (undo.old_values.empty() && undo.new_ids.empty() && undo.removed_values.empty())
                  return;
               deltas.push_back({});
               auto& delta = deltas.back();
               delta.name  = name;
               for (auto& old : undo.old_values) {
                  auto& row = index.get(old.first);
                  if (filter(row))
                     delta.rows.emplace_back(old.first._id, fc::raw::pack(make_history_serial_wrapper(row)));
               }
               for (auto id : undo.new_ids) {
                  auto& row = index.get(id);
                  if (filter(row))
                     delta.rows.emplace_back(id._id, fc::raw::pack(make_history_serial_wrapper(row)));
               }
               for (auto& old : undo.removed_values)
                  if (filter(old.second))
                     delta.removed.push_back(old.first._id);
            }
         });
         auto bin = fc::raw::pack(deltas);
         // dlog("    ${s} bytes", ("s", bin.size()));
         hist.deltas.insert(hist.deltas.end(), bin.begin(), bin.end());
      });
   } // on_accepted_block

   void on_irreversible_block(const block_state_ptr& block_state) {
      // ilog("irreversible ${n}", ("n", block_state->block->block_num()));
      auto& idx = db->get_index<state_history_index>().indices();
      while (!idx.empty()) {
         auto& obj = *idx.begin();
         if (obj.id._id > block_state->block->block_num())
            break;
         if (obj.id._id == state_log.end_block || state_log.begin_block == state_log.end_block) {
            // ilog("write irreversible ${n}", ("n", obj.id._id));
            EOS_ASSERT(obj.deltas.size() == (uint32_t)obj.deltas.size(), plugin_exception, "deltas is too big");
            history_log_header header{.block_num    = (uint32_t)obj.id._id,
                                      .payload_size = sizeof(uint32_t) + obj.deltas.size()};
            state_log.write_entry(header, [&](auto& stream) {
               uint32_t s = (uint32_t)obj.deltas.size();
               stream.write((char*)&s, sizeof(s));
               if (!obj.deltas.empty())
                  stream.write(obj.deltas.data(), obj.deltas.size());
            });
         }
         if (idx.size() >= 2 && (++idx.begin())->id._id <= block_state->block->block_num()) {
            // ilog("drop irreversible ${n}", ("n", obj.id._id));
            db->remove(obj);
         } else
            break;
      }
   } // on_irreversible_block
};   // state_history_plugin_impl

state_history_plugin::state_history_plugin()
    : my(std::make_shared<state_history_plugin_impl>()) {}

state_history_plugin::~state_history_plugin() {}

void state_history_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto options = cfg.add_options();
   options("state-history-dir", bpo::value<bfs::path>()->default_value("state-history"),
           "the location of the state-history directory (absolute path or relative to application data dir)");
   options("state-history-db-size-mb", bpo::value<uint64_t>()->default_value(1024),
           "Maximum size (in MiB) of the state history database");
   options("delete-state-history", bpo::bool_switch()->default_value(false), "clear state history database");
   options("state-history-endpoint", bpo::value<string>()->default_value("0.0.0.0:8080"),
           "the endpoint upon which to listen for incoming connections");
}

void state_history_plugin::plugin_initialize(const variables_map& options) {
   try {
      // todo: check for --disable-replay-opts

      my->chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT(my->chain_plug, chain::missing_chain_plugin_exception, "");
      auto& chain = my->chain_plug->chain();
      my->accepted_block_connection.emplace(
          chain.accepted_block.connect([&](const block_state_ptr& p) { my->on_accepted_block(p); }));
      my->irreversible_block_connection.emplace(
          chain.irreversible_block.connect([&](const block_state_ptr& p) { my->on_irreversible_block(p); }));

      auto                    dir_option = options.at("state-history-dir").as<bfs::path>();
      boost::filesystem::path state_history_dir;
      if (dir_option.is_relative())
         state_history_dir = app().data_dir() / dir_option;
      else
         state_history_dir = dir_option;
      auto state_history_size = options.at("state-history-db-size-mb").as<uint64_t>() * 1024 * 1024;

      auto ip_port         = options.at("state-history-endpoint").as<string>();
      auto port            = ip_port.substr(ip_port.find(':') + 1, ip_port.size());
      auto host            = ip_port.substr(0, ip_port.find(':'));
      my->endpoint_address = host;
      my->endpoint_port    = std::stoi(port);
      idump((ip_port)(host)(port));

      if (options.at("delete-state-history").as<bool>()) {
         ilog("Deleting state history");
         boost::filesystem::remove_all(state_history_dir);
      }

      my->db = std::make_unique<chainbase::database>(state_history_dir, database::read_write, state_history_size);
      my->db->add_index<state_history_index>();
      my->open_irreversible(state_history_dir);

      auto& ind = my->db->get_index<state_history_index>().indices();
      ilog("in-memory state history has ${n} blocks", ("n", ind.size()));
      if (my->state_log.end_block != my->state_log.begin_block) {
         // todo: load if ind.empty()
         EOS_ASSERT(!ind.empty(), plugin_exception, "irreversible_state.log and shared_memory don't match");
         auto& x = *ind.begin();
         EOS_ASSERT(x.id._id == my->state_log.end_block - 1, plugin_exception,
                    "irreversible_state.log and shared_memory don't match");
      }

      // size_t total = 0, max_size = 0, max_block = 0, num = 0;
      // for (auto& x : ind) {
      //    if (!(x.id._id % 10000))
      //       dlog("    ${i}: ${s} bytes", ("i", x.id._id)("s", x.deltas.size()));
      //    total += x.deltas.size();
      //    if (x.deltas.size() > max_size) {
      //       max_block = x.id._id;
      //       max_size  = x.deltas.size();
      //    }
      //    ++num;
      // }
      // dlog("total: ${s} bytes", ("s", total));
      // dlog("max:   ${s} bytes in ${b}", ("s", max_size)("b", max_block));
      // dlog("num:   ${s}", ("s", num));
   }
   FC_LOG_AND_RETHROW()
} // namespace eosio

void state_history_plugin::plugin_startup() { my->listen(); }

void state_history_plugin::plugin_shutdown() {
   my->accepted_block_connection.reset();
   my->irreversible_block_connection.reset();
   while (!my->sessions.empty())
      my->sessions.begin()->second->close();
   my->db.reset();
}

} // namespace eosio
