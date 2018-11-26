/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/statetrack_plugin/statetrack_plugin_impl.hpp>

namespace eosio
{

using namespace chain;
using namespace chainbase;

// statetrack plugin implementation

void statetrack_plugin_impl::send_zmq_msg(const fc::string content)
{
  if (sender_socket == nullptr)
    return;

  zmq::message_t message(content.length());
  memcpy(message.data(), content.c_str(), content.length());
  sender_socket->send(message);
}

bool statetrack_plugin_impl::filter(const account_name &code, const fc::string &scope, const table_name &table)
{
  if (filter_on.size() > 0)
  {
    bool pass_on = false;
    if (filter_on.find({code, 0, 0}) != filter_on.end())
    {
      pass_on = true;
    }
    if (filter_on.find({code, scope, 0}) != filter_on.end())
    {
      pass_on = true;
    }
    if (filter_on.find({code, scope, table}) != filter_on.end())
    {
      pass_on = true;
    }
    if (!pass_on)
    {
      return false;
    }
  }

  if (filter_out.size() > 0)
  {
    if (filter_out.find({code, 0, 0}) != filter_out.end())
    {
      return false;
    }
    if (filter_out.find({code, scope, 0}) != filter_out.end())
    {
      return false;
    }
    if (filter_out.find({code, scope, table}) != filter_out.end())
    {
      return false;
    }
  }

  return true;
}

const table_id_object *statetrack_plugin_impl::get_kvo_tio(const database &db, const key_value_object &kvo)
{
  return db.find<table_id_object>(kvo.t_id);
}

const account_object &statetrack_plugin_impl::get_tio_co(const database &db, const table_id_object &tio)
{
  const account_object *co = db.find<account_object, by_name>(tio.code);
  EOS_ASSERT(co != nullptr, account_query_exception, "Fail to retrieve account for ${code}", ("code", tio.code));
  return *co;
}

const abi_def &statetrack_plugin_impl::get_co_abi(const account_object &co)
{
  abi_def *abi = new abi_def();
  abi_serializer::to_abi(co.abi, *abi);
  return *abi;
}

fc::variant statetrack_plugin_impl::get_kvo_row(const database &db, const table_id_object &tio, const key_value_object &kvo, bool json)
{
  vector<char> data;
  copy_inline_row(kvo, data);

  if (json)
  {
    const account_object &co = get_tio_co(db, tio);
    const abi_def &abi = get_co_abi(co);

    abi_serializer abis;
    abis.set_abi(abi, abi_serializer_max_time);

    return abis.binary_to_variant(abis.get_table_type(tio.table), data, abi_serializer_max_time, shorten_abi_errors);
  }
  else
  {
    return fc::variant(data);
  }
}

db_op statetrack_plugin_impl::get_db_op(const database &db, const account_object &co, op_type_enum op_type)
{
  db_op op;

  name system = N(system);

  op.id = co.id._id;
  op.op_type = op_type;
  op.code = system;
  op.scope = system.to_string();
  op.table = N(accounts);

  db_account account;
  account.name = co.name;
  account.vm_type = co.vm_type;
  account.vm_version = co.vm_version;
  account.privileged = co.privileged;
  account.last_code_update = co.last_code_update;
  account.code_version = co.code_version;
  account.creation_date = co.creation_date;

  op.value = fc::variant(account);

  return op;
}

db_op statetrack_plugin_impl::get_db_op(const database &db, const permission_object &po, op_type_enum op_type)
{
  db_op op;

  name system = N(system);

  op.id = po.id._id;
  op.op_type = op_type;
  op.code = system;
  op.scope = system.to_string();
  op.table = N(permissions);
  op.value = fc::variant(po);

  return op;
}

db_op statetrack_plugin_impl::get_db_op(const database &db, const permission_link_object &plo, op_type_enum op_type)
{
  db_op op;

  name system = N(system);

  op.id = plo.id._id;
  op.op_type = op_type;
  op.code = system;
  op.scope = system.to_string();
  op.table = N(permission_links);
  op.value = fc::variant(plo);

  return op;
}

db_op statetrack_plugin_impl::get_db_op(const database &db, const table_id_object &tio, const key_value_object &kvo, op_type_enum op_type, bool json)
{
  db_op op;

  op.id = kvo.primary_key;
  op.op_type = op_type;

  db_action_trac& dat = current_trx_actions.back();
  op.trx_id = dat.trx_id;
  op.act_index = current_trx_actions.size();
  op.inline_action = dat.inline_action;

  if(is_undo_state) {
    dat.op_size--;

    if(dat.op_size <= 0)
      current_trx_actions.pop_back();
  }
  else {
    
    dat.op_size++;
  }

  op.code = tio.code;
  op.scope = scope_sym_to_string(tio.scope);
  op.table = tio.table;

  if (op_type == op_type_enum::ROW_CREATE ||
      op_type == op_type_enum::ROW_MODIFY)
  {
    op.value = get_kvo_row(db, tio, kvo, json);
  }

  return op;
}

db_rev statetrack_plugin_impl::get_db_rev(const int64_t revision, op_type_enum op_type)
{
  db_rev rev = {op_type, revision};
  return rev;
}

void statetrack_plugin_impl::on_applied_table(const database &db, const table_id_object &tio, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  auto code = tio.code;
  auto scope = scope_sym_to_string(tio.scope);
  auto table = tio.table;

  if (filter(code, scope, table))
  {
    db_table table;

    table.op_type = op_type;
    table.code = tio.code;
    table.scope = scope_sym_to_string(tio.scope);
    table.table = tio.table;

    fc::string data = fc::json::to_string(table);
    ilog("STATETRACK table_id_object ${op_type}: ${data}", ("op_type", op_type)("data", data));
    send_zmq_msg(data);
  }
}

void statetrack_plugin_impl::on_applied_op(const database &db, const account_object &co, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  name system = N(system);
  auto code = system;
  auto scope = system.to_string();
  auto table = N(accounts);

  if (filter(code, scope, table))
  {
    fc::string data = fc::json::to_string(get_db_op(db, co, op_type));
    ilog("STATETRACK account_object ${op_type}: ${data}", ("op_type", op_type)("data", data));
    send_zmq_msg(data);
  }
}

void statetrack_plugin_impl::on_applied_op(const database &db, const permission_object &po, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  name system = N(system);
  auto code = system;
  auto scope = system.to_string();
  auto table = N(permission);

  if (filter(code, scope, table))
  {
    fc::string data = fc::json::to_string(get_db_op(db, po, op_type));
    ilog("STATETRACK permission_object ${op_type}: ${data}", ("op_type", op_type)("data", data));
    send_zmq_msg(data);
  }
}

void statetrack_plugin_impl::on_applied_op(const database &db, const permission_link_object &plo, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  name system = N(system);
  auto code = system;
  auto scope = system.to_string();
  auto table = N(permission_links);

  if (filter(code, scope, table))
  {
    fc::string data = fc::json::to_string(get_db_op(db, plo, op_type));
    ilog("STATETRACK permission_link_object ${op_type}: ${data}", ("op_type", op_type)("data", data));
    send_zmq_msg(data);
  }
}

void statetrack_plugin_impl::on_applied_op(const database &db, const key_value_object &kvo, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  const table_id_object *tio_ptr = get_kvo_tio(db, kvo);

  if (tio_ptr == nullptr)
    return;

  const table_id_object &tio = *tio_ptr;

  auto code = tio.code;
  auto scope = scope_sym_to_string(tio.scope);
  auto table = tio.table;

  if (filter(code, scope, table))
  {
    fc::string data = fc::json::to_string(get_db_op(db, *tio_ptr, kvo, op_type));
    ilog("STATETRACK key_value_object ${op_type}: ${data}", ("op_type", op_type)("data", data));
    send_zmq_msg(data);
  }
}

void statetrack_plugin_impl::on_applied_rev(const int64_t revision, op_type_enum op_type)
{
  if (sender_socket == nullptr)
    return;

  fc::string data = fc::json::to_string(get_db_rev(revision, op_type));
  ilog("STATETRACK ${op_type}: ${data}", ("op_type", op_type)("data", data));
  send_zmq_msg(data);
}

void statetrack_plugin_impl::on_applied_undo(const int64_t revision)
{
  if (sender_socket == nullptr)
    return;

  is_undo_state = true;

  fc::string data = fc::json::to_string(get_db_rev(revision, op_type_enum::REV_UNDO));
  ilog("STATETRACK ${op_type}: ${data}", ("op_type", op_type_enum::REV_UNDO)("data", data));
  send_zmq_msg(data);
}

void statetrack_plugin_impl::on_applied_transaction(const transaction_trace_ptr &ttp)
{
  if (ttp->receipt)
  {
    cached_traces[ttp->id] = ttp;
  }

  current_trx_actions.clear();
}

void statetrack_plugin_impl::on_accepted_block(const block_state_ptr &bsp)
{
  //TODO send accepted block information

  for (auto &t : bsp->block->transactions)
  {
    transaction_id_type id;
    if (t.trx.contains<transaction_id_type>())
    {
      id = t.trx.get<transaction_id_type>();
    }
    else
    {
      id = t.trx.get<packed_transaction>().id();
    }

    if (t.status == transaction_receipt_header::executed)
    {
      // Send traces only for executed transactions
      auto it = cached_traces.find(id);
      if (it == cached_traces.end() || !it->second->receipt)
      {
        ilog("missing trace for transaction {id}", ("id", id));
        continue;
      }

      for (const auto &atrace : it->second->action_traces)
      {
        on_action_trace(atrace, bsp);
      }
    }
  }

  cached_traces.clear();
}

void statetrack_plugin_impl::on_action_trace(const action_trace &at, const block_state_ptr &bsp)
{
  if (sender_socket == nullptr)
    return;

  // check the action against the blacklist
  auto search_acc = blacklist_actions.find(at.act.account);
  if (search_acc != blacklist_actions.end())
  {
    auto search_act = search_acc->second.find(at.act.name);
    if (search_act != search_acc->second.end())
    {
      return;
    }
  }

  auto &chain = chain_plug->chain();

  db_op op;

  name system = N(system);
  op.id = at.receipt.global_sequence;
  op.op_type = op_type_enum::TRX_ACTION;
  op.code = system;
  op.scope = scope_sym_to_string(system);
  op.table = N(actions);

  db_action da;
  da.block_num = bsp->block->block_num();
  da.block_time = bsp->block->timestamp;
  da.action_trace = chain.to_variant_with_abi(at, abi_serializer_max_time);
  da.last_irreversible_block = chain.last_irreversible_block_num();

  op.value = fc::variant(da);

  fc::string data = fc::json::to_string(op);
  ilog("STATETRACK ${op_type}: ${data}", ("op_type", op_type_enum::TRX_ACTION)("data", data));
  send_zmq_msg(data);
}

void statetrack_plugin_impl::on_irreversible_block(const block_state_ptr &bsp)
{
  if (sender_socket == nullptr)
    return;

  fc::string data = fc::json::to_string(get_db_rev(bsp->block_num, op_type_enum::REV_COMMIT));
  ilog("STATETRACK ${op_type}: ${data}", ("op_type", op_type_enum::REV_COMMIT)("data", data));
  send_zmq_msg(data);
}

void statetrack_plugin_impl::on_pre_apply_action(std::pair<action_trace&, bool>& trace) 
{
  db_action_trac dat;
  dat.trx_id = trace.first.trx_id;
  dat.inline_action = trace.second;
  dat.op_size = 0;

  current_trx_actions.emplace_back(dat);
  is_undo_state = false;
}

} // namespace eosio