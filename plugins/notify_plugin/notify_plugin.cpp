/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/notify_plugin/notify_plugin.hpp>
#include <eosio/notify_plugin/http_async_client.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/block_state.hpp>

#include <fc/io/json.hpp>
#include <fc/network/url.hpp>

#include <boost/signals2/connection.hpp>
#include <boost/algorithm/string.hpp>

#include <unordered_map>

namespace eosio
{
static appbase::abstract_plugin &_notify_plugin = app().register_plugin<notify_plugin>();
using namespace chain;
typedef uint32_t action_seq_type;

class notify_plugin_impl
{
public:
  static const int64_t default_age_limit = 60;
  static const int64_t default_retry_times = 3;
  static const fc::microseconds http_timeout;
  static const fc::microseconds max_deserialization_time;

  fc::url receive_url;
  int64_t age_limit = default_age_limit;
  int64_t retry_times = default_retry_times;
  http_async_client httpc;

  struct sequenced_action : public action
  {
    sequenced_action(const action &act, action_seq_type seq, account_name receiver)
        : action(act), seq_num(seq), receiver(receiver) {}

    action_seq_type seq_num;
    account_name receiver;
  };

  struct action_notify
  {
    action_notify(const sequenced_action &act, transaction_id_type tx_id,
                  const variant &action_data, fc::time_point block_time,
                  uint32_t block_num)
        : tx_id(tx_id), account(act.account), name(act.name), receiver(act.receiver),
          seq_num(act.seq_num), block_time(block_time), block_num(block_num),
          authorization(act.authorization), action_data(action_data) {}

    transaction_id_type tx_id;
    account_name account;
    account_name name;
    account_name receiver;
    action_seq_type seq_num;
    fc::time_point block_time;
    uint32_t block_num;

    vector<permission_level> authorization;
    fc::variant action_data;
  };

  struct message
  {
    message() : irreversible(false), actions() {}
    bool irreversible;
    std::vector<action_notify> actions;
  };

  struct filter_entry
  {
    name receiver;
    name action;

    std::tuple<name, name> key() const
    {
      return std::make_tuple(receiver, action);
    }

    friend bool operator<(const filter_entry &a, const filter_entry &b)
    {
      return a.key() < b.key();
    }
  };

  typedef std::unordered_multimap<transaction_id_type, sequenced_action> action_queue_type;

  chain_plugin *chain_plug = nullptr;
  std::set<notify_plugin_impl::filter_entry> filter_on;
  fc::optional<boost::signals2::scoped_connection> accepted_block_conn;
  fc::optional<boost::signals2::scoped_connection> irreversible_block_conn;
  fc::optional<boost::signals2::scoped_connection> applied_tx_conn;
  action_queue_type action_queue;
  action_queue_type irreversible_action_queue;

  bool filter(const action_trace &act);
  fc::variant deserialize_action_data(action act);
  void build_message(message &msg, const block_state_ptr &block, const transaction_id_type &tx_id, bool irreversible);
  void send_message(const message &msg);
  action_seq_type on_action_trace(const action_trace &act, const transaction_id_type &tx_id, action_seq_type act_s);
  void on_applied_tx(const transaction_trace_ptr &trace);
  void on_accepted_block(const block_state_ptr &block_state);
  void on_irreversible_block(const block_state_ptr &block_state);
};

bool notify_plugin_impl::filter(const action_trace &act)
{
  if (filter_on.find({act.receipt.receiver, act.act.name}) != filter_on.end())
  {
    return true;
  }
  else if (filter_on.find({act.receipt.receiver, 0}) != filter_on.end())
  {
    return true;
  }
  return false;
}

fc::variant notify_plugin_impl::deserialize_action_data(action act)
{
  auto &chain = chain_plug->chain();
  auto serializer = chain.get_abi_serializer(act.account, max_deserialization_time);
  FC_ASSERT(serializer.valid() && serializer->get_action_type(act.name) != action_name(),
            "Unable to get abi for account: ${acc}, action: ${a} Not sending notification.",
            ("acc", act.account)("a", act.name));
  return serializer->binary_to_variant(act.name.to_string(), act.data, max_deserialization_time);
}

void notify_plugin_impl::build_message(message &msg, const block_state_ptr &block, const transaction_id_type &tx_id, const bool irreversible)
{
  // dlog("irreversible: ${a}", ("a", fc::json::to_pretty_string(irreversible)));
  auto range = irreversible ? irreversible_action_queue.equal_range(tx_id) : action_queue.equal_range(tx_id);

  msg.irreversible = irreversible;
  for (auto &it = range.first; it != range.second; it++)
  {
    auto act_data = deserialize_action_data(it->second);
    action_notify notify(it->second, tx_id, std::forward<fc::variant>(act_data),
                         block->block->timestamp, block->block->block_num());
    msg.actions.push_back(notify);
  }
}

void notify_plugin_impl::send_message(const message &msg)
{
  try
  {
    httpc.post(receive_url, msg, fc::time_point::now() + http_timeout);
  }
  FC_CAPTURE_AND_LOG(("Error while sending notification")(msg));
}

action_seq_type notify_plugin_impl::on_action_trace(const action_trace &act, const transaction_id_type &tx_id,
                                                    action_seq_type act_s)
{
  if (filter(act))
  {
    const auto pair = std::make_pair(tx_id, sequenced_action(act.act, act_s, act.receipt.receiver));
    action_queue.insert(pair);
    irreversible_action_queue.insert(pair);
    // dlog("on_action_trace: ${a}", ("a", fc::json::to_pretty_string(act.act)));
  }
  act_s++;

  for (const auto &iline : act.inline_traces)
  {
    act_s = on_action_trace(iline, tx_id, act_s);
  }
  return act_s;
}

void notify_plugin_impl::on_applied_tx(const transaction_trace_ptr &trace)
{
  auto id = trace->id;

  if (!action_queue.count(id) || !irreversible_action_queue.count(id))
  {
    action_seq_type seq = 0;
    for (auto &at : trace->action_traces)
    {
      seq = on_action_trace(at, id, seq);
    }
  }
}

void notify_plugin_impl::on_accepted_block(const block_state_ptr &block_state)
{
  fc::time_point block_time = block_state->block->timestamp;

  if (age_limit == -1 || (fc::time_point::now() - block_time < fc::seconds(age_limit)))
  {
    message msg;
    transaction_id_type tx_id;
    // dlog("block_state->block->transactions: ${a}", ("a", fc::json::to_pretty_string(block_state->block->transactions)));
    for (const auto &trx : block_state->block->transactions)
    {
      if (trx.trx.contains<transaction_id_type>())
      {
        tx_id = trx.trx.get<transaction_id_type>();
      }
      else
      {
        tx_id = trx.trx.get<packed_transaction>().id();
      }

      // dlog("tx_id: ${a}", ("a", fc::json::to_pretty_string(tx_id)));
      // dlog("action_queue.size(): ${a}", ("a", fc::json::to_pretty_string(action_queue.size())));
      if (action_queue.count(tx_id))
      {
        build_message(msg, block_state, tx_id, false);
      }
    }
    // dlog("msg: ${a}", ("a", msg));
    if (msg.actions.size() > 0)
    {
      send_message(msg);
    }
  }
  action_queue.clear();
}

void notify_plugin_impl::on_irreversible_block(const block_state_ptr &block_state)
{
  fc::time_point block_time = block_state->block->timestamp;
  if (age_limit == -1 || (fc::time_point::now() - block_time < fc::seconds(age_limit)))
  {
    message msg;
    transaction_id_type tx_id;
    // dlog("block_state->block->transactions: ${a}", ("a", fc::json::to_pretty_string(block_state->block->transactions)));
    for (const auto &trx : block_state->block->transactions)
    {
      if (trx.trx.contains<transaction_id_type>())
      {
        tx_id = trx.trx.get<transaction_id_type>();
      }
      else
      {
        tx_id = trx.trx.get<packed_transaction>().id();
      }
      // dlog("tx_id: ${a}", ("a", fc::json::to_pretty_string(tx_id)));
      // dlog("irreversible_action_queue.size(): ${a}", ("a", fc::json::to_pretty_string(irreversible_action_queue.size())));
      if (irreversible_action_queue.count(tx_id))
      {
        build_message(msg, block_state, tx_id, true);
      }
    }
    // dlog("msg: ${a}", ("a", msg));
    if (msg.actions.size() > 0)
    {
      send_message(msg);
      irreversible_action_queue.clear();
    }
  }
}

const fc::microseconds notify_plugin_impl::http_timeout = fc::seconds(10);
const fc::microseconds notify_plugin_impl::max_deserialization_time = fc::seconds(5);
const int64_t notify_plugin_impl::default_age_limit;
const int64_t notify_plugin_impl::default_retry_times;

notify_plugin::notify_plugin() : my(new notify_plugin_impl()) {}
notify_plugin::~notify_plugin() {}

void notify_plugin::set_program_options(options_description &, options_description &cfg)
{
  cfg.add_options()("notify-filter-on", bpo::value<vector<string>>()->composing(),
                        "Track actions and make notifications then it match receiver:action. In case action is not specified, "
                        "all actions to specified account are tracked.")
                    ("notify-receive-url", bpo::value<string>(), "Notify URL which can receive the notifications")
                    ("notify-age-limit", bpo::value<int64_t>()->default_value(notify_plugin_impl::default_age_limit),
                        "Age limit in seconds for blocks to send notifications about."
                        " No age limit if this is set to negative.")
                    ("notify-retry-times", bpo::value<int64_t>()->default_value(notify_plugin_impl::default_retry_times),
                        "Retry times of sending http notification if failed.")
                    ;
}

void notify_plugin::plugin_initialize(const variables_map &options)
{
  try
  {
    EOS_ASSERT(options.count("notify-receive-url") == 1, fc::invalid_arg_exception,
               "notify_plugin requires one notify-receiver-url to be specified!");

    EOS_ASSERT(options.count("notify-age-limit") == 1, fc::invalid_arg_exception,
               "notify_plugin requires one notify-age-limit to be specified!");

    EOS_ASSERT(options.count("notify-retry-times") == 1, fc::invalid_arg_exception,
               "notify_plugin requires one notify-retry-times to be specified!");

    string url_str = options.at("notify-receive-url").as<string>();
    my->receive_url = fc::url(url_str);

    if (options.count("notify-filter-on"))
    {
      auto fo = options.at("notify-filter-on").as<vector<string>>();
      for (auto &s : fo)
      {
        std::vector<std::string> v;
        boost::split(v, s, boost::is_any_of(":"));
        EOS_ASSERT(v.size() == 2, fc::invalid_arg_exception,
                   "Invalid value ${s} for --notify-filter-on",
                   ("s", s));
        notify_plugin_impl::filter_entry fe{v[0], v[1]};
        EOS_ASSERT(fe.receiver.value, fc::invalid_arg_exception, "Invalid value ${s} for --notify-filter-on", ("s", s));
        my->filter_on.insert(fe);
      }
    }

    if (options.count("notify-age-limit"))
      my->age_limit = options.at("notify-age-limit").as<int64_t>();

    if (options.count("notify-retry-times"))
      my->retry_times = options.at("notify-retry-times").as<int64_t>();
    
    my->httpc.set_default_retry_times(my->retry_times);
    my->chain_plug = app().find_plugin<chain_plugin>();
    auto &chain = my->chain_plug->chain();
    my->accepted_block_conn.emplace(chain.accepted_block.connect(
        [&](const block_state_ptr &b_state) {
          my->on_accepted_block(b_state);
        }));

    my->irreversible_block_conn.emplace(chain.irreversible_block.connect(
        [&](const block_state_ptr &bs) {
          my->on_irreversible_block(bs);
        }));

    my->applied_tx_conn.emplace(chain.applied_transaction.connect(
        [&](const transaction_trace_ptr &tx) {
          my->on_applied_tx(tx);
        }));
  }
  FC_LOG_AND_RETHROW()
}

void notify_plugin::plugin_startup()
{
  ilog("Notify plugin started");
  my->httpc.start();
}

void notify_plugin::plugin_shutdown()
{
  my->applied_tx_conn.reset();
  my->accepted_block_conn.reset();
  my->irreversible_block_conn.reset();
  my->httpc.stop();
}
} // namespace eosio

FC_REFLECT(eosio::notify_plugin_impl::action_notify, (tx_id)(account)(name)(seq_num)(receiver)(block_time)(block_num)(authorization)(action_data))
FC_REFLECT(eosio::notify_plugin_impl::message, (irreversible)(actions))