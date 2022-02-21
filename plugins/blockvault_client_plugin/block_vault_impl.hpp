#pragma once

#include "backend.hpp"
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <fc/io/datastream.hpp>
#include <fc/scoped_exit.hpp>
#include <memory>
#include <thread>

namespace eosio {
namespace blockvault {

template <typename Compressor>
struct transform_callback : backend::sync_callback {
   Compressor&                       compressor;
   eosio::blockvault::sync_callback& target;

   transform_callback(Compressor& comp, eosio::blockvault::sync_callback& t)
       : compressor(comp)
       , target(t) {}

   void on_snapshot(const char* filename) override {
      auto uncompressed_file = compressor.decompress(filename);

      if (uncompressed_file.size())
         target.on_snapshot(uncompressed_file.c_str());
      else
         FC_THROW_EXCEPTION(chain::snapshot_decompress_exception, "Unable to decompress snapshot received from block vault");
   }

   void on_block(std::string_view block) override {
      chain::signed_block_ptr     b = std::make_shared<chain::signed_block>();
      fc::datastream<const char*> ds(block.cbegin(), block.size());
      fc::raw::unpack(ds, *b);
      target.on_block(b);
   }
};

template <typename Compressor>
class block_vault_impl : public block_vault_interface {
   Compressor                           compressor;
   std::unique_ptr<blockvault::backend> backend;

   boost::asio::io_context                                                  ioc;
   std::thread                                                              thr;
   boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;
   std::atomic<bool>                                                        syncing;
   fc::logger                                                               log;

 public:
   block_vault_impl(std::unique_ptr<blockvault::backend>&& be)
       : backend(std::move(be))
       , work(boost::asio::make_work_guard(ioc)) {}

   ~block_vault_impl() {
      if (thr.joinable()) {
         thr.join();
      }
   }

   void async_propose_constructed_block(uint32_t lib, chain::signed_block_ptr block,
                                        std::function<void(bool)> handler) override {
      boost::asio::post(ioc, [this, handler, lib, block]() {
         try {
            fc::datastream<std::vector<char>> stream;
            fc::raw::pack(stream, *block);
            eosio::chain::block_id_type block_id = block->calculate_id();

            bool r = backend->propose_constructed_block({block->block_num(), block->timestamp.slot}, lib,
                                                        stream.storage(), {block_id.data(), block_id.data_size()},
                                                        {block->previous.data(), block->previous.data_size()});

            // Notice : 
            //   This following logging line is used for checking double production in 'tests/blockvault_tests.py'.
            //   Make sure the corresponding code in 'tests/blockvault_tests.py' is changed if the format is changed.
            fc_dlog(log, "propose_constructed_block(watermark={${bn}, ${ts}}, lib=${lib}) returns ${r}",
                 ("bn", block->block_num())("ts", block->timestamp.slot)("lib", lib)("r", r));
            handler(r);
         } catch (std::exception& ex) {
            fc_elog(log, ex.what());
            handler(false);
         }
      });
   }
   void async_append_external_block(uint32_t lib, chain::signed_block_ptr block,
                                    std::function<void(bool)> handler) override {
      if (syncing.load())
         return;

      boost::asio::post(ioc, [this, handler, lib, block]() {
         try {
            fc::datastream<std::vector<char>> stream;
            fc::raw::pack(stream, *block);
            eosio::chain::block_id_type block_id = block->calculate_id();

            bool r = backend->append_external_block(block->block_num(), lib, stream.storage(),
                                                    {block_id.data(), block_id.data_size()},
                                                    {block->previous.data(), block->previous.data_size()});
            fc_dlog(log, "append_external_block(block_num=${bn}, lib=${lib}) returns ${r}",
                 ("bn", block->block_num())("lib", lib)("r", r));
            handler(r);
         } catch (std::exception& ex) {
            fc_elog(log, ex.what());
            handler(false);
         }
      });
   }

   bool propose_snapshot(watermark_t watermark, const char* snapshot_filename) override {
      fc_dlog(log, "propose_snapshot(watermark={${wf}, ${ws}), ${fn})",
                 ("wf", watermark.first)("ws", watermark.second.slot)("fn", snapshot_filename));

      std::string compressed_file = compressor.compress(snapshot_filename);
      if (compressed_file.size()) {
         auto on_exit = fc::make_scoped_exit([&compressed_file]() { boost::filesystem::remove(compressed_file); });     
         return backend->propose_snapshot({watermark.first, watermark.second.slot}, compressed_file.c_str());
      } else {
         fc_elog(log, "snapshot compress failed");
         return false;
      }
   }
   void sync(const eosio::chain::block_id_type* ptr_block_id, sync_callback& callback) override {
      transform_callback<Compressor> cb{compressor, callback};
      std::string_view bid = ptr_block_id ? std::string_view{ptr_block_id->data(), ptr_block_id->data_size()}
                                          : std::string_view{nullptr, 0};
      syncing.store(true);
      auto on_exit = fc::make_scoped_exit([this]() { syncing.store(false); });
      backend->sync(bid, cb);
   }

   void start() {
      thr = std::thread([&log = log, & ioc = ioc]() {
         fc_ilog(log, "block vault thread started");
         ioc.run();
      });
   }

   void stop() { work.reset(); }

   void set_logger_name(const char* logger_name) {
      fc::logger::update( logger_name, log );
   }
};
} // namespace blockvault
} // namespace eosio