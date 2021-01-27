#include <b1/rodeos/rodeos.hpp>

#include <b1/rodeos/callbacks/kv.hpp>
#include <b1/rodeos/rodeos_tables.hpp>
#include <fc/log/trace.hpp>

#include <fc/log/trace.hpp>

namespace b1::rodeos {

namespace ship_protocol = eosio::ship_protocol;

using ship_protocol::get_blocks_result_base;
using ship_protocol::get_blocks_result_v0;
using ship_protocol::get_blocks_result_v1;
using ship_protocol::signed_block_header;
using ship_protocol::signed_block_variant;

rodeos_db_snapshot::rodeos_db_snapshot(std::shared_ptr<rodeos_db_partition> partition, bool persistent)
    : partition{ std::move(partition) }, db{ this->partition->db } {
   if (persistent) {
      undo_stack.emplace(*db, this->partition->undo_prefix);
      write_session.emplace(*db);
   } else {
      snap.emplace(db->rdb.get());
      write_session.emplace(*db, snap->snapshot());
   }

   db_view_state    view_state{ state_account, *db, *write_session, this->partition->contract_kv_prefix };
   fill_status_sing sing{ state_account, view_state, false };
   if (sing.exists()) {
      auto status     = std::get<0>(sing.get());
      chain_id        = status.chain_id;
      head            = status.head;
      head_id         = status.head_id;
      irreversible    = status.irreversible;
      irreversible_id = status.irreversible_id;
      first           = status.first;
   }
}

void rodeos_db_snapshot::refresh() {
   if (undo_stack)
      throw std::runtime_error("can not refresh a persistent snapshot");
   snap.emplace(db->rdb.get());
   write_session->snapshot = snap->snapshot();
   write_session->wipe_cache();
}

void rodeos_db_snapshot::write_fill_status() {
   if (!undo_stack)
      throw std::runtime_error("Can only write to persistent snapshots");
   fill_status status;
   if (irreversible < head)
      status = fill_status_v0{ .chain_id        = chain_id,
                               .head            = head,
                               .head_id         = head_id,
                               .irreversible    = irreversible,
                               .irreversible_id = irreversible_id,
                               .first           = first };
   else
      status = fill_status_v0{ .chain_id        = chain_id,
                               .head            = head,
                               .head_id         = head_id,
                               .irreversible    = head,
                               .irreversible_id = head_id,
                               .first           = first };

   db_view_state view_state{ state_account, *db, *write_session, partition->contract_kv_prefix };
   view_state.kv_state.enable_write = true;
   fill_status_sing sing{ state_account, view_state, false };
   sing.set(status);
   sing.store();
}

void rodeos_db_snapshot::end_write(bool write_fill) {
   if (!undo_stack)
      throw std::runtime_error("Can only write to persistent snapshots");
   if (write_fill)
      write_fill_status();
   write_session->write_changes(*undo_stack);
}

void rodeos_db_snapshot::start_block(const get_blocks_result_base& result) {
   if (!undo_stack)
      throw std::runtime_error("Can only write to persistent snapshots");
   if (!result.this_block)
      throw std::runtime_error("get_blocks_result this_block is empty");

   if (result.this_block->block_num <= head) {
      ilog("switch forks at block ${b}; database contains revisions ${f} - ${h}",
           ("b", result.this_block->block_num)("f", undo_stack->first_revision())("h", undo_stack->revision()));
      if (undo_stack->first_revision() >= result.this_block->block_num)
         throw std::runtime_error("can't switch forks since database doesn't contain revision " +
                                  std::to_string(result.this_block->block_num - 1));
      write_session->wipe_cache();
      while (undo_stack->revision() >= result.this_block->block_num) //
         undo_stack->undo(true);
   }

   if (head_id != eosio::checksum256{} && (!result.prev_block || result.prev_block->block_id != head_id))
      throw std::runtime_error("prev_block does not match");

   if (result.this_block->block_num <= result.last_irreversible.block_num) {
      undo_stack->commit(std::min(result.last_irreversible.block_num, head));
      undo_stack->set_revision(result.this_block->block_num, false);
   } else {
      end_write(false);
      undo_stack->commit(std::min(result.last_irreversible.block_num, head));
      undo_stack->push(false);
   }
   writing_block = result.this_block->block_num;
}

void rodeos_db_snapshot::end_block(const get_blocks_result_base& result, bool force_write) {
   if (!undo_stack)
      throw std::runtime_error("Can only write to persistent snapshots");
   if (!result.this_block)
      throw std::runtime_error("get_blocks_result this_block is empty");
   if (!writing_block || result.this_block->block_num != *writing_block)
      throw std::runtime_error("call start_block first");

   bool near       = result.this_block->block_num + 4 >= result.last_irreversible.block_num;
   bool write_now  = !(result.this_block->block_num % 200) || near || force_write;
   head            = result.this_block->block_num;
   head_id         = result.this_block->block_id;
   irreversible    = result.last_irreversible.block_num;
   irreversible_id = result.last_irreversible.block_id;
   if (!first || head < first)
      first = head;
   if (write_now)
      end_write(write_now);
   if (near)
      db->flush(false, false);
}

void rodeos_db_snapshot::check_write(const ship_protocol::get_blocks_result_base& result) {
   if (!undo_stack)
      throw std::runtime_error("Can only write to persistent snapshots");
   if (!result.this_block)
      throw std::runtime_error("get_blocks_result this_block is empty");
   if (!writing_block || result.this_block->block_num != *writing_block)
      throw std::runtime_error("call start_block first");
}

void rodeos_db_snapshot::write_block_info(uint32_t block_num, const eosio::checksum256& id,
                                          const eosio::ship_protocol::signed_block_header& block) {
   db_view_state view_state{ state_account, *db, *write_session, partition->contract_kv_prefix };
   view_state.kv_state.enable_write = true;

   block_info_v0 info;
   info.num                = block_num;
   info.id                 = id;
   info.timestamp          = block.timestamp;
   info.producer           = block.producer;
   info.confirmed          = block.confirmed;
   info.previous           = block.previous;
   info.transaction_mroot  = block.transaction_mroot;
   info.action_mroot       = block.action_mroot;
   info.schedule_version   = block.schedule_version;
   info.new_producers      = block.new_producers;
   info.producer_signature = block.producer_signature;

   block_info_kv table{ kv_environment{ view_state } };
   table.put(info);
}

namespace {
   std::string to_string( const eosio::checksum256& cs ) {
      auto bytes = cs.extract_as_byte_array();
      return fc::to_hex((const char*)bytes.data(), bytes.size());
   }
}

void rodeos_db_snapshot::write_block_info(const ship_protocol::get_blocks_result_v0& result) {
   check_write(result);
   if (!result.block)
      return;

   uint32_t            block_num = result.this_block->block_num;
   eosio::input_stream bin       = *result.block;
   signed_block_header block;
   from_bin(block, bin);

   auto blk_trace = fc_create_trace_with_id( "Block", result.this_block->block_id );
   auto blk_span = fc_create_span( blk_trace, "rodeos-received" );
   fc_add_tag( blk_span, "block_id", to_string( result.this_block->block_id ) );
   fc_add_tag( blk_span, "block_num", block_num );
   fc_add_tag( blk_span, "block_time", block.timestamp.to_time_point().elapsed.count() );

   write_block_info(block_num, result.this_block->block_id, block);
}

void rodeos_db_snapshot::write_block_info(const ship_protocol::get_blocks_result_v1& result) {
   check_write(result);
   if (!result.block)
      return;

   uint32_t block_num = result.this_block->block_num;

   const signed_block_header& header =
         std::visit([](const auto& blk) { return static_cast<const signed_block_header&>(blk); }, *result.block);

   auto blk_trace = fc_create_trace_with_id( "Block", result.this_block->block_id );
   auto blk_span = fc_create_span( blk_trace, "rodeos-received" );
   fc_add_tag( blk_span, "block_id", to_string( result.this_block->block_id ) );
   fc_add_tag( blk_span, "block_num", block_num );
   fc_add_tag( blk_span, "block_time", eosio::microseconds_to_str( header.timestamp.to_time_point().elapsed.count() ) );

   write_block_info(block_num, result.this_block->block_id, header);
}

void rodeos_db_snapshot::write_deltas(uint32_t block_num, eosio::opaque<std::vector<ship_protocol::table_delta>> deltas,
                                      std::function<bool()> shutdown) {
   db_view_state view_state{ state_account, *db, *write_session, partition->contract_kv_prefix };
   view_state.kv_state.bypass_receiver_check = true; // TODO: can we enable recevier check in the future
   view_state.kv_state.enable_write          = true;
   uint32_t num                              = deltas.unpack_size();
   for (uint32_t i = 0; i < num; ++i) {
      ship_protocol::table_delta delta;
      deltas.unpack_next(delta);
      size_t num_processed = 0;
      std::visit(
         [&](auto& delta_any_v) {
         store_delta({ view_state }, delta_any_v, head == 0, [&]() {
            if (delta_any_v.rows.size() > 10000 && !(num_processed % 10000)) {
               if (shutdown())
                  throw std::runtime_error("shutting down");
               ilog("block ${b} ${t} ${n} of ${r}",
                    ("b", block_num)("t", delta_any_v.name)("n", num_processed)("r", delta_any_v.rows.size()));
               if (head == 0) {
                  end_write(false);
                  view_state.reset();
               }
            }
            ++num_processed;
         });
      }, delta);
   }
}

void rodeos_db_snapshot::write_deltas(const ship_protocol::get_blocks_result_v0& result,
                                      std::function<bool()>                      shutdown) {
   check_write(result);
   if (!result.deltas)
      return;

   uint32_t block_num = result.this_block->block_num;
   write_deltas(block_num, eosio::opaque<std::vector<ship_protocol::table_delta>>(*result.deltas), shutdown);
}

void rodeos_db_snapshot::write_deltas(const ship_protocol::get_blocks_result_v1& result,
                                      std::function<bool()>                      shutdown) {
   check_write(result);
   if (result.deltas.empty())
      return;

   uint32_t block_num = result.this_block->block_num;
   write_deltas(block_num, result.deltas, shutdown);
}

std::once_flag registered_filter_callbacks;

rodeos_filter::rodeos_filter(eosio::name name, const std::string& wasm_filename, bool profile
#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
                             ,
                             const boost::filesystem::path&       eosvmoc_path,
                             const eosio::chain::eosvmoc::config& eosvmoc_config, bool eosvmoc_enable
#endif
                             )
    : name{ name } {
   std::call_once(registered_filter_callbacks, filter::register_callbacks);

   std::ifstream wasm_file(wasm_filename, std::ios::binary);
   if (!wasm_file.is_open())
      throw std::runtime_error("can not open " + wasm_filename);
   ilog("compiling ${f}", ("f", wasm_filename));
   wasm_file.seekg(0, std::ios::end);
   int len = wasm_file.tellg();
   if (len < 0)
      throw std::runtime_error("wasm file length is -1");
   std::vector<uint8_t> code(len);
   wasm_file.seekg(0, std::ios::beg);
   wasm_file.read((char*)code.data(), code.size());
   wasm_file.close();
   backend      = std::make_unique<filter::backend_t>(code, nullptr);
   filter_state = std::make_unique<filter::filter_state>();
   filter::rhf_t::resolve(backend->get_module());
   if (profile) {
      prof = std::make_unique<eosio::vm::profile_data>(wasm_filename + ".profile", *backend);
   }
#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
   if (eosvmoc_enable) {
      try {
         filter_state->eosvmoc_tierup.emplace(
               eosvmoc_path, eosvmoc_config, code,
               eosio::chain::digest_type::hash(reinterpret_cast<const char*>(code.data()), code.size()));
      }
      FC_LOG_AND_RETHROW();
   }
#endif
}

void rodeos_filter::process(rodeos_db_snapshot& snapshot, const ship_protocol::get_blocks_result_base& result,
                            eosio::input_stream                                         bin,
                            const std::function<void(const char* data, uint64_t size)>& push_data) {
   // todo: timeout
   snapshot.check_write(result);
   chaindb_state chaindb_state;
   db_view_state view_state{ name, *snapshot.db, *snapshot.write_session, snapshot.partition->contract_kv_prefix };
   view_state.kv_state.enable_write  = true;
   filter::callbacks cb{ *filter_state, chaindb_state, view_state };
   filter_state->max_console_size = 10000;
   filter_state->console.clear();
   filter_state->input_data = bin;
   filter_state->push_data  = push_data;

#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
   if (filter_state->eosvmoc_tierup) {
      const auto* code =
            filter_state->eosvmoc_tierup->cc.get_descriptor_for_code(filter_state->eosvmoc_tierup->hash, 0);
      if (code) {
         eosio::chain::eosvmoc::timer_base timer;
         filter_state->eosvmoc_tierup->exec.execute(*code, filter_state->eosvmoc_tierup->mem, &cb, 251, 65536, &timer, 0,
                                                    0, 0);
         return;
      }
   }
#endif

   backend->set_wasm_allocator(&filter_state->wa);
   backend->initialize(&cb);
   try {
      eosio::vm::scoped_profile profile_runner(prof.get());
      (*backend)(cb, "env", "apply", uint64_t(0), uint64_t(0), uint64_t(0));

      if (!filter_state->console.empty())
         ilog("filter ${n} console output: <<<\n${c}>>>", ("n", name.to_string())("c", filter_state->console));
   } catch (...) {
      try {
         throw;
      } catch ( const std::bad_alloc& ) {
        throw;
      } catch ( const boost::interprocess::bad_alloc& ) {
        throw;
      } catch( const fc::exception& e ) {
         elog( "fc::exception processing filter wasm: ${e}", ("e", e.to_detail_string()) );
      } catch( const std::exception& e ) {
         elog( "std::exception processing filter wasm: ${e}", ("e", e.what()) );
      } catch( ... ) {
         elog( "unknown exception processing filter wasm" );
      }
      if (!filter_state->console.empty())
         ilog("filter ${n} console output before exception: <<<\n${c}>>>",
              ("n", name.to_string())("c", filter_state->console));
      throw;
   }
}

rodeos_query_handler::rodeos_query_handler(std::shared_ptr<rodeos_db_partition>         partition,
                                           std::shared_ptr<const wasm_ql::shared_state> shared_state)
    : partition{ partition }, shared_state{ std::move(shared_state) }, state_cache{
         std::make_shared<wasm_ql::thread_state_cache>(this->shared_state)
      } {}

} // namespace b1::rodeos
