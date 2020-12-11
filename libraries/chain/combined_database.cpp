#include <eosio/chain/backing_store/kv_context_rocksdb.hpp>
#include <eosio/chain/backing_store/kv_context_chainbase.hpp>
#include <eosio/chain/backing_store/db_combined.hpp>
#include <eosio/chain/combined_database.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/backing_store/db_context.hpp>
#include <eosio/chain/backing_store/db_key_value_format.hpp>

namespace eosio { namespace chain {
   combined_session::combined_session(chainbase::database& cb_database, eosio::session::undo_stack<rocks_db_type>* undo_stack)
       : kv_undo_stack{ undo_stack } {
      cb_session = std::make_unique<chainbase::database::session>(cb_database.start_undo_session(true));
      try {
        try {
            if (kv_undo_stack) {
              kv_undo_stack->push();
            }
        }
        FC_LOG_AND_RETHROW()
      }
      CATCH_AND_EXIT_DB_FAILURE()
   }

   combined_session::combined_session(combined_session&& src) noexcept
       : cb_session(std::move(src.cb_session)), kv_undo_stack(src.kv_undo_stack) {
      src.kv_undo_stack = nullptr;
   }

   void combined_session::push() {
      if (cb_session) {
         cb_session->push();
         cb_session = nullptr;

         if (kv_undo_stack) {
            kv_undo_stack = nullptr;
         }
      }
   }

   void combined_session::squash() {
      if (cb_session) {
         cb_session->squash();
         cb_session = nullptr;

         if (kv_undo_stack) {
            try {
               try {
                  kv_undo_stack->squash();
                  kv_undo_stack = nullptr;
               }
               FC_LOG_AND_RETHROW()
            }
            CATCH_AND_EXIT_DB_FAILURE()
         }
      }
   }

   void combined_session::undo() {
      if (cb_session) {
         cb_session->undo();
         cb_session = nullptr;

         if (kv_undo_stack) {
            try {
               try {
                  kv_undo_stack->undo();
                  kv_undo_stack = nullptr;
               }
               FC_LOG_AND_RETHROW()
            }
            CATCH_AND_EXIT_DB_FAILURE()
         }
      }
   }

   template <typename Util, typename F>
   void walk_index(const Util& utils, const chainbase::database& db, F&& function) {
      utils.walk(db, std::forward<F>(function));
   }

   template <typename F>
   void walk_index(const index_utils<table_id_multi_index>& utils, const chainbase::database& db, F&& function) {}

   template <typename F>
   void walk_index(const index_utils<database_header_multi_index>& utils, const chainbase::database& db, F&& function) {}

   template <typename F>
   void walk_index(const index_utils<kv_db_config_index>& utils, const chainbase::database& db, F&& function) {}

   void add_kv_table_to_snapshot(const snapshot_writer_ptr& snapshot, const chainbase::database& db, const kv_undo_stack_ptr& kv_undo_stack) {
      snapshot->write_section<kv_object>([&db,&kv_undo_stack](auto& section) {
         if (kv_undo_stack && db.get<kv_db_config_object>().backing_store == backing_store_type::ROCKSDB) {
            using add_database_section_receiver = backing_store::add_database_receiver<std::decay_t < decltype(section)>>;
            add_database_section_receiver add_db_receiver(section, db);
            backing_store::rocksdb_contract_kv_table_writer<add_database_section_receiver> writer(add_db_receiver);
            const auto begin_key = eosio::session::shared_bytes(&backing_store::rocksdb_contract_kv_prefix, 1);
            const auto end_key = begin_key.next();
            backing_store::walk_rocksdb_entries_with_prefix(kv_undo_stack, begin_key, end_key, writer);
         } else {
            index_utils<kv_index> utils;
            utils.walk<by_kv_key>(db, [&db, &section](const auto& row) { section.add_row(row, db); });
         }
      });
   }

   void read_kv_table_from_snapshot(const snapshot_reader_ptr& snapshot, chainbase::database& db,
                                    const std::unique_ptr<rocks_db_type>& kv_database, uint32_t version, backing_store_type backing_store ) {
      if (version < kv_object::minimum_snapshot_version)
         return;
      if (backing_store == backing_store_type::ROCKSDB) {
         auto key_values = std::vector<std::pair<eosio::session::shared_bytes, eosio::session::shared_bytes>>{};
         constexpr std::size_t batch_size = 500;
         key_values.reserve(batch_size);
         snapshot->read_section<kv_object>([&key_values, &db, &kv_database](auto& section) {
            const std::string_view prefix_key {&backing_store::rocksdb_contract_kv_prefix, 1};
            bool more = !section.empty();
            while (more) {
               kv_object_view move_to_rocks;
               more = section.read_row(move_to_rocks, db);
               b1::chain_kv::bytes contract_as_bytes;
               b1::chain_kv::append_key(contract_as_bytes, move_to_rocks.contract.to_uint64_t());
               auto full_key =
                     eosio::session::make_shared_bytes<std::string_view, 3>({prefix_key,
                                                                             std::string_view{contract_as_bytes.data(),
                                                                                              contract_as_bytes.size()},
                                                                             std::string_view{move_to_rocks.kv_key.data.data(),
                                                                                              move_to_rocks.kv_key.data.size()}});

               // Pack payer and actual key value
               auto final_kv_value = backing_store::payer_payload(move_to_rocks.payer,
                                                                  move_to_rocks.kv_value.data.data(),
                                                                  move_to_rocks.kv_value.data.size());

               key_values.emplace_back(full_key,
                                       final_kv_value.as_payload());

               if (key_values.size() >= batch_size) {
                  kv_database->write(key_values);
                  key_values.clear();
               }
            }
         });
         // write out any remaining key-values
         kv_database->write(key_values);
      }
      else {
         snapshot->read_section<kv_object>([&db](auto& section) {
            bool more = !section.empty();
            while (more) {
               index_utils<kv_index>::create(db, [&db, &section, &more](auto &row) { more = section.read_row(row, db); });
            }
         });
      }
   }

   combined_database::combined_database(chainbase::database& chain_db,
                                        uint32_t snapshot_batch_threashold)
       : backing_store(backing_store_type::CHAINBASE), db(chain_db), kv_snapshot_batch_threashold(snapshot_batch_threashold * 1024 * 1024) {}

   combined_database::combined_database(chainbase::database& chain_db,
                                        const controller::config& cfg)
       : backing_store(backing_store_type::ROCKSDB), db(chain_db), kv_database{ [&]() {
            rocksdb::Options options;

            options.create_if_missing = true; // Creates a database if it is missing
            options.level_compaction_dynamic_level_bytes = true;
            options.bytes_per_sync = cfg.persistent_storage_bytes_per_sync; // used to control the write rate of flushes and compactions.
            options.use_adaptive_mutex = true;

            // Number of threads used for flush and compaction.
            options.IncreaseParallelism(cfg.persistent_storage_num_threads);

            options.OptimizeLevelStyleCompaction(512ull << 20); // optimizes level style compaction

            // Number of open files that can be used by the DB.  
            // Setting it to -1 means files opened are always kept open.
            options.max_open_files = cfg.persistent_storage_max_num_files;

            // Use this option to increase the number of threads
            // used to open the files.
            options.max_file_opening_threads = cfg.persistent_storage_num_threads;

            // Write Buffer Size - Sets the size of a single
            // memtable. Once memtable exceeds this size, it is
            // marked immutable and a new one is created.
            // Default should be 128MB
            options.write_buffer_size = cfg.persistent_storage_write_buffer_size;
            options.max_write_buffer_number = 10; // maximum number of memtables, both active and immutable
            options.min_write_buffer_number_to_merge = 2; // minimum number of memtables to be merged before flushing to storage

            // Once level 0 reaches this number of files, L0->L1 compaction is triggered.
            options.level0_file_num_compaction_trigger = 2;

            // Size of L0 = write_buffer_size * min_write_buffer_number_to_merge * level0_file_num_compaction_trigger
            // For optimal performance make this equal to L0
            options.max_bytes_for_level_base = cfg.persistent_storage_write_buffer_size * options.min_write_buffer_number_to_merge * options.level0_file_num_compaction_trigger;

            // Files in level 1 will have target_file_size_base
            // bytes. It’s recommended setting target_file_size_base
            // to be max_bytes_for_level_base / 10.
            options.target_file_size_base = options.max_bytes_for_level_base / 10;

            // This value represents the maximum number of threads
            // that will concurrently perform a compaction job by
            // breaking it into multiple,
            // smaller ones that are run simultaneously.
            options.max_subcompactions = cfg.persistent_storage_num_threads;

            // Full and partitioned filters in the block-based table
            // use an improved Bloom filter implementation, enabled
            // with format_version 5 (or above) because previous
            // releases cannot read this filter. This replacement is
            // faster and more accurate, especially for high bits
            // per key or millions of keys in a single (full) filter.
            rocksdb::BlockBasedTableOptions table_options;
            table_options.format_version               = 5;
            table_options.index_block_restart_interval = 16;

            // Sets the bloom filter - Given an arbitrary key, 
            // this bit array may be used to determine if the key 
            // may exist or definitely does not exist in the key set.
	          table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(15, false));
	          table_options.index_type = rocksdb::BlockBasedTableOptions::kBinarySearch;

            // Incorporates the Table options into options
            options.table_factory.reset(NewBlockBasedTableFactory(table_options));

            rocksdb::DB* p;
            auto         status = rocksdb::DB::Open(options, (cfg.state_dir / "chain-kv").string(), &p);
            if (!status.ok())
               throw std::runtime_error(std::string{ "database::database: rocksdb::DB::Open: " } + status.ToString());
            auto rdb        = std::shared_ptr<rocksdb::DB>{ p };
            return std::make_unique<rocks_db_type>(eosio::session::make_session(std::move(rdb), 1024));
         }() },
         kv_undo_stack(std::make_unique<eosio::session::undo_stack<rocks_db_type>>(*kv_database, cfg.state_dir)),
         kv_snapshot_batch_threashold(cfg.persistent_storage_mbytes_batch * 1024 * 1024)  {}

   void combined_database::check_backing_store_setting(bool clean_startup) {
      if (backing_store != db.get<kv_db_config_object>().backing_store) {   
         EOS_ASSERT(clean_startup, database_move_kv_disk_exception,
                   "Existing state indicates a different backing store is in use; use resync, replay, or restore from snapshot to switch backing store");
         db.modify(db.get<kv_db_config_object>(), [this](auto& cfg) { cfg.backing_store = backing_store; });
      }

      if (backing_store == backing_store_type::ROCKSDB)
         ilog("using rocksdb for backing store");
      else
         ilog("using chainbase for backing store");
   }

   void combined_database::destroy(const fc::path& p) {
      if( !fc::is_directory( p ) )
          return;

      fc::remove( p / "shared_memory.bin" );
      fc::remove( p / "shared_memory.meta" );

      rocks_db_type::destroy((p / "chain-kv").string());
   }

   void combined_database::set_revision(uint64_t revision) {
      db.set_revision(revision);

      if (backing_store == backing_store_type::ROCKSDB) {
         try {
            try {
                kv_undo_stack->revision(revision);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }
   }

   int64_t combined_database::revision() {
      if (backing_store == backing_store_type::ROCKSDB) {
         try {
            try {
                return kv_undo_stack->revision();
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      } else {
        return db.revision();
      }
   }

   void combined_database::undo() {
      db.undo();

      if (backing_store == backing_store_type::ROCKSDB) {
         try {
            try {
              kv_undo_stack->undo();
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }
   }

   void combined_database::commit(int64_t revision) {
      db.commit(revision);

      if (backing_store == backing_store_type::ROCKSDB) {
         try {
            try {
               kv_undo_stack->commit(revision);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }
   }

   void combined_database::flush() {
      if (backing_store == backing_store_type::ROCKSDB) {
         try {
            try {
               kv_database->flush();
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }
   }

   std::unique_ptr<kv_context> combined_database::create_kv_context(name receiver, kv_resource_manager resource_manager,
                                                                    const kv_database_config& limits) const {
      switch (backing_store) {
         case backing_store_type::ROCKSDB:
            return std::visit([&](auto* session){
              return create_kv_rocksdb_context<std::remove_pointer_t<decltype(session)>, kv_resource_manager>(*session, receiver,
                                                                                  resource_manager, limits);
            }, kv_undo_stack->top().holder());
         case backing_store_type::CHAINBASE:
            return create_kv_chainbase_context<kv_resource_manager>(db, receiver, resource_manager, limits);
      }
      EOS_ASSERT(false, action_validate_exception, "Unknown backing store.");
   }

   std::unique_ptr<db_context> combined_database::create_db_context(apply_context& context, name receiver) {
      switch (backing_store) {
         case backing_store_type::ROCKSDB:
            return backing_store::create_db_rocksdb_context(context, receiver, kv_undo_stack->top());
         case backing_store_type::CHAINBASE:
            return backing_store::create_db_chainbase_context(context, receiver);
         default:
            EOS_ASSERT(false, action_validate_exception, "Unknown backing store.");
      }
   }

   void combined_database::add_to_snapshot(
         const eosio::chain::snapshot_writer_ptr& snapshot, const eosio::chain::block_state& head,
         const eosio::chain::authorization_manager&                    authorization,
         const eosio::chain::resource_limits::resource_limits_manager& resource_limits) const {
      snapshot->write_section<chain_snapshot_header>(
            [this](auto& section) { section.add_row(chain_snapshot_header(), db); });

      snapshot->write_section<block_state>(
            [this, &head](auto& section) { section.template add_row<block_header_state>(head, db); });

      eosio::chain::controller_index_set::walk_indices([this, &snapshot](auto utils) {
         using value_t = typename decltype(utils)::index_t::value_type;

         snapshot->write_section<value_t>([utils, this](auto& section) {
            walk_index(utils, db, [this, &section](const auto& row) { section.add_row(row, db); });
         });
      });

      add_kv_table_to_snapshot(snapshot, db, kv_undo_stack);
      add_contract_tables_to_snapshot(snapshot);

      authorization.add_to_snapshot(snapshot);
      resource_limits.add_to_snapshot(snapshot);
   }

   void combined_database::read_from_snapshot(const snapshot_reader_ptr& snapshot,
                                              uint32_t blog_start,
                                              uint32_t blog_end,
                                              eosio::chain::authorization_manager& authorization,
                                              eosio::chain::resource_limits::resource_limits_manager& resource_limits,
                                              eosio::chain::fork_database& fork_db, eosio::chain::block_state_ptr& head,
                                              uint32_t&                          snapshot_head_block,
                                              const eosio::chain::chain_id_type& chain_id) {
      chain_snapshot_header header;
      snapshot->read_section<chain_snapshot_header>([this, &header](auto& section) {
         section.read_row(header, db);
         header.validate();
      });

      db.create<kv_db_config_object>([](auto&) {});
      check_backing_store_setting(true);

      { /// load and upgrade the block header state
         block_header_state head_header_state;
         using v2 = legacy::snapshot_block_header_state_v2;

         if (std::clamp(header.version, v2::minimum_version, v2::maximum_version) == header.version) {
            snapshot->read_section<block_state>([this, &head_header_state](auto& section) {
               legacy::snapshot_block_header_state_v2 legacy_header_state;
               section.read_row(legacy_header_state, db);
               head_header_state = block_header_state(std::move(legacy_header_state));
            });
         } else {
            snapshot->read_section<block_state>(
                  [this, &head_header_state](auto& section) { section.read_row(head_header_state, db); });
         }

         snapshot_head_block = head_header_state.block_num;
         EOS_ASSERT(blog_start <= (snapshot_head_block + 1) && snapshot_head_block <= blog_end, block_log_exception,
                    "Block log is provided with snapshot but does not contain the head block from the snapshot nor a "
                    "block right after it",
                    ("snapshot_head_block", snapshot_head_block)("block_log_first_num",
                                                                 blog_start)("block_log_last_num", blog_end));

         fork_db.reset(head_header_state);
         head                = fork_db.head();
         snapshot_head_block = head->block_num;
      }

      controller_index_set::walk_indices([this, &snapshot, &header](auto utils) {
         using value_t = typename decltype(utils)::index_t::value_type;

         // skip the table_id_object as its inlined with contract tables section
         if (std::is_same<value_t, table_id_object>::value) {
            return;
         }

         // skip the database_header as it is only relevant to in-memory database
         if (std::is_same<value_t, database_header_object>::value) {
            return;
         }

         // skip the kv_db_config as it only determines where the kv-database is stored
         if (std::is_same_v<value_t, kv_db_config_object>) {
            return;
         }

         // special case for in-place upgrade of global_property_object
         if (std::is_same<value_t, global_property_object>::value) {
            using v2 = legacy::snapshot_global_property_object_v2;
            using v3 = legacy::snapshot_global_property_object_v3;

            if (std::clamp(header.version, v2::minimum_version, v2::maximum_version) == header.version) {
               std::optional<genesis_state> genesis = extract_legacy_genesis_state(*snapshot, header.version);
               EOS_ASSERT(genesis, snapshot_exception,
                          "Snapshot indicates chain_snapshot_header version 2, but does not contain a genesis_state. "
                          "It must be corrupted.");
               snapshot->read_section<global_property_object>(
                     [&db = this->db, gs_chain_id = genesis->compute_chain_id()](auto& section) {
                        v2 legacy_global_properties;
                        section.read_row(legacy_global_properties, db);

                        db.create<global_property_object>([&legacy_global_properties, &gs_chain_id](auto& gpo) {
                           gpo.initalize_from(legacy_global_properties, gs_chain_id, kv_database_config{},
                                              genesis_state::default_initial_wasm_configuration);
                        });
                     });
               return; // early out to avoid default processing
            }

            if (std::clamp(header.version, v3::minimum_version, v3::maximum_version) == header.version) {
               snapshot->read_section<global_property_object>([&db = this->db](auto& section) {
                  v3 legacy_global_properties;
                  section.read_row(legacy_global_properties, db);

                  db.create<global_property_object>([&legacy_global_properties](auto& gpo) {
                     gpo.initalize_from(legacy_global_properties, kv_database_config{},
                                        genesis_state::default_initial_wasm_configuration);
                  });
               });
               return; // early out to avoid default processing
            }
         }

         snapshot->read_section<value_t>([this](auto& section) {
            bool more = !section.empty();
            while (more) {
               decltype(utils)::create(db, [this, &section, &more](auto& row) { more = section.read_row(row, db); });
            }
         });
      });

      read_kv_table_from_snapshot(snapshot, db, kv_database, header.version, backing_store);
      read_contract_tables_from_snapshot(snapshot);

      authorization.read_from_snapshot(snapshot);
      resource_limits.read_from_snapshot(snapshot, header.version);

      set_revision(head->block_num);
      db.create<database_header_object>([](const auto& header) {
         // nothing to do
      });

      const auto& gpo = db.get<global_property_object>();
      EOS_ASSERT(gpo.chain_id == chain_id, chain_id_type_exception,
                 "chain ID in snapshot (${snapshot_chain_id}) does not match the chain ID that controller was "
                 "constructed with (${controller_chain_id})",
                 ("snapshot_chain_id", gpo.chain_id)("controller_chain_id", chain_id));
   }

   template <typename Section>
   void chainbase_add_contract_tables_to_snapshot(const chainbase::database& db, Section& section) {
      index_utils<table_id_multi_index>::walk(db, [&db, &section](const table_id_object& table_row) {
         // add a row for the table
         section.add_row(table_row, db);

         // followed by a size row and then N data rows for each type of table
         contract_database_index_set::walk_indices([&db, &section, &table_row](auto utils) {
            using utils_t     = decltype(utils);
            using value_t     = typename utils_t::index_t::value_type;
            using by_table_id = object_to_table_id_tag_t<value_t>;

            auto tid_key      = std::make_tuple(table_row.id);
            auto next_tid_key = std::make_tuple(table_id_object::id_type(table_row.id._id + 1));

            unsigned_int size = utils_t::template size_range<by_table_id>(db, tid_key, next_tid_key);
            section.add_row(size, db);

            utils_t::template walk_range<by_table_id>(db, tid_key, next_tid_key, [&db, &section](const auto& row) {
               section.add_row(row, db);
            });
         });
      });
   }

   void combined_database::add_contract_tables_to_snapshot(const snapshot_writer_ptr& snapshot) const {
      snapshot->write_section("contract_tables", [this](auto& section) {
         if (kv_undo_stack && db.get<kv_db_config_object>().backing_store == backing_store_type::ROCKSDB) {
            using add_database_section_receiver = backing_store::add_database_receiver<std::decay_t < decltype(section)>>;
            using table_collector = backing_store::rocksdb_whole_db_table_collector<add_database_section_receiver>;

            add_database_section_receiver add_db_receiver(section, db);
            table_collector table_collector_receiver(add_db_receiver);
            backing_store::rocksdb_contract_db_table_writer<table_collector> writer(table_collector_receiver);
            const auto begin_key = eosio::session::shared_bytes(&backing_store::rocksdb_contract_db_prefix, 1);
            const auto end_key = begin_key.next();
            backing_store::walk_rocksdb_entries_with_prefix(kv_undo_stack, begin_key, end_key, writer);
         }
         else {
            chainbase_add_contract_tables_to_snapshot(db, section);
         }
      });
   }

   template <typename Section>
   void chainbase_read_contract_tables_from_snapshot(chainbase::database& db, Section& section) {
      bool more = !section.empty();
      while (more) {
         // read the row for the table
         table_id_object::id_type t_id;

         index_utils<table_id_multi_index>::create(db, [&db, &section, &t_id](auto& row) {
            section.read_row(row, db);
            t_id = row.id;
         });

         // read the size and data rows for each type of table
         contract_database_index_set::walk_indices([&db, &section, &t_id, &more](auto utils) {
            using utils_t = decltype(utils);

            unsigned_int size;
            more = section.read_row(size, db);

            for (size_t idx = 0; idx < size.value; ++idx) {
               utils_t::create(db, [&db, &section, &more, &t_id](auto& row) {
                  row.t_id = t_id;
                  more     = section.read_row(row, db);
               });
            }
         });
      }
   }

   template <typename Section>
   void rocksdb_read_contract_tables_from_snapshot(rocks_db_type& kv_database, chainbase::database& db,
                                                   Section& section, uint64_t snapshot_batch_threashold) {
      std::vector<std::pair<eosio::session::shared_bytes, eosio::session::shared_bytes>> batch;
      bool                more     = !section.empty();
      auto                read_row = [&section, &more, &db](auto& row) { more = section.read_row(row, db); };
      uint64_t            batch_mem_size = 0;

      while (more) {
         // read the row for the table
         backing_store::table_id_object_view table_obj;
         read_row(table_obj);
         auto put = [&batch, &table_obj, &batch_mem_size, &kv_database, snapshot_batch_threashold]
               (auto&& value, auto create_fun, auto&&... args) {
            auto composite_key = create_fun(table_obj.scope, table_obj.table, std::forward<decltype(args)>(args)...);
            batch.emplace_back(backing_store::db_key_value_format::create_full_key(composite_key, table_obj.code),
                               std::forward<decltype(value)>(value));

            const auto& back = batch.back();
            const auto size = back.first.size() + back.second.size();
            if (size >= snapshot_batch_threashold || snapshot_batch_threashold - size < batch_mem_size) {
               kv_database.write(batch);
               batch_mem_size = 0;
               batch.clear();
            }
            else {
               batch_mem_size += size;
            }
         };

         // handle the primary key index
         unsigned_int size;
         read_row(size);
         for (size_t i = 0; i < size.value; ++i) {
            backing_store::primary_index_view row;
            read_row(row);
            backing_store::payer_payload pp{row.payer, row.value.data(), row.value.size()};
            put(pp.as_payload(), backing_store::db_key_value_format::create_primary_key, row.primary_key);
         }

         auto write_secondary_index = [&put, &read_row](auto index) {
            using index_t = decltype(index);
            static const eosio::session::shared_bytes  empty_payload;
            unsigned_int       size;
            read_row(size);
            for (uint32_t i = 0; i < size.value; ++i) {
               backing_store::secondary_index_view<index_t> row;
               read_row(row);
               backing_store::payer_payload pp{row.payer, nullptr, 0};
               put(pp.as_payload(), &backing_store::db_key_value_format::create_secondary_key<index_t>,
                   row.secondary_key, row.primary_key);

               put(empty_payload, &backing_store::db_key_value_format::create_primary_to_secondary_key<index_t>,
                   row.primary_key, row.secondary_key);
            }
         };

         // handle secondary key indices
         std::tuple<uint64_t, uint128_t, key256_t, float64_t, float128_t> indices;
         std::apply([&write_secondary_index](auto... index) { (write_secondary_index(index), ...); }, indices);

         backing_store::payer_payload pp{table_obj.payer, nullptr, 0};
         b1::chain_kv::bytes (*create_table_key)(name scope, name table) = backing_store::db_key_value_format::create_table_key;
         put(pp.as_payload(), create_table_key);

      }
      kv_database.write(batch);
   }

   void combined_database::read_contract_tables_from_snapshot(const snapshot_reader_ptr& snapshot) {
      snapshot->read_section("contract_tables", [this](auto& section) {
         if (kv_undo_stack && db.get<kv_db_config_object>().backing_store == backing_store_type::ROCKSDB)
            rocksdb_read_contract_tables_from_snapshot(*kv_database, db, section, kv_snapshot_batch_threashold);
         else
            chainbase_read_contract_tables_from_snapshot(db, section);
      });
   }

   std::optional<eosio::chain::genesis_state> extract_legacy_genesis_state(snapshot_reader& snapshot,
                                                                           uint32_t         version) {
      std::optional<eosio::chain::genesis_state> genesis;
      using v2 = legacy::snapshot_global_property_object_v2;

      if (std::clamp(version, v2::minimum_version, v2::maximum_version) == version) {
         genesis.emplace();
         snapshot.read_section<eosio::chain::genesis_state>(
               [&genesis = *genesis](auto& section) { section.read_row(genesis); });
      }
      return genesis;
   }

   // TODO : need to change this method to just return a char
   std::vector<char> make_rocksdb_contract_kv_prefix() { return std::vector<char> { backing_store::rocksdb_contract_kv_prefix }; }
   char make_rocksdb_contract_db_prefix() { return backing_store::rocksdb_contract_db_prefix; }

}} // namespace eosio::chain


