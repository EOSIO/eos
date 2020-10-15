#include <eosio/chain/backing_store/kv_context_rocksdb.hpp>
#include <eosio/chain/backing_store/kv_context_chainbase.hpp>
#include <eosio/chain/combined_database.hpp>
#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/backing_store/db_context.hpp>
#include <eosio/chain/backing_store/db_key_value_format.hpp>

namespace eosio { namespace chain {
   combined_session::combined_session(chainbase::database& cb_database, eosio::session::undo_stack<rocks_db_type>* undo_stack)
       : kv_undo_stack{ undo_stack } {
      try {
        try {
            cb_session = std::make_unique<chainbase::database::session>(cb_database.start_undo_session(true));
            if (kv_undo_stack) {
              kv_undo_stack->push();
            }
        }
        FC_LOG_AND_RETHROW()
      }
      CATCH_AND_EXIT_DB_FAILURE()
   }

   combined_session::combined_session(combined_session&& src) noexcept
       : cb_session(std::move(src.cb_session)), kv_undo_stack(std::move(src.kv_undo_stack)) {
      kv_undo_stack = nullptr;
   }

   void combined_session::push() {
      if (cb_session) {
         if (!kv_undo_stack) {
            cb_session->push();
            cb_session = nullptr;
         } else {
            try {
               try {
                  cb_session->push();
                  cb_session    = nullptr;
                  kv_undo_stack = nullptr;
               }
               FC_LOG_AND_RETHROW()
            }
            CATCH_AND_EXIT_DB_FAILURE()
         }
      }
   }

   void combined_session::squash() {
      if (cb_session) {
         if (!kv_undo_stack) {
            cb_session->squash();
            cb_session = nullptr;
         } else {
            try {
               try {
                  cb_session->squash();
                  kv_undo_stack->squash();
                  cb_session    = nullptr;
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
         if (!kv_undo_stack) {
            cb_session->undo();
            cb_session = nullptr;
         } else {
            try {
               try {
                  cb_session->undo();
                  kv_undo_stack->undo();
                  cb_session    = nullptr;
                  kv_undo_stack = nullptr;
               }
               FC_LOG_AND_RETHROW()
            }
            CATCH_AND_EXIT_DB_FAILURE()
         }
      }
   }

   // chainlib reserves prefixes 0x10 - 0x2F.
   static const std::vector<char> rocksdb_undo_prefix{ 0x10 };
   static const std::vector<char> rocksdb_contract_kv_prefix{ 0x11 }; // for KV API
   static const std::vector<char> rocksdb_contract_db_prefix{ 0x12 }; // for DB API

   template <typename Util, typename F>
   void walk_index(const Util& utils, eosio::session::undo_stack<rocks_db_type>* kv_undo_stack, const chainbase::database& db, F function) {
      utils.walk(db, function);
   }

   template <typename F>
   void walk_index(const index_utils<table_id_multi_index>& utils, eosio::session::undo_stack<rocks_db_type>* kv_undo_stack, const chainbase::database& db, F function) {}

   template <typename F>
   void walk_index(const index_utils<database_header_multi_index>& utils, eosio::session::undo_stack<rocks_db_type>* kv_undo_stack, const chainbase::database& db, F function) {}

   template <typename F>
   void walk_index(const index_utils<kv_db_config_index>& utils, eosio::session::undo_stack<rocks_db_type>* kv_undo_stack, const chainbase::database& db, F function) {}

   template <typename F>
   void for_each_rocksdb_index_with_prefix(const eosio::session::undo_stack<rocks_db_type>& kv_undo_stack, const std::vector<char>& prefix, F&& function) {
      const auto prefix_key = eosio::session::shared_bytes(prefix.data(), prefix.size());
      const auto next_prefix = prefix_key.next();
      auto begin_it = kv_undo_stack.top().lower_bound(prefix_key);
      auto end_it = kv_undo_stack.top().lower_bound(next_prefix);
      for (auto it = begin_it; it != end_it; ++it) {
         auto key = (*it).first;

         uint64_t    contract;
         std::size_t key_prefix_size = prefix_key.size() + sizeof(contract);
         EOS_ASSERT(key.size() >= key_prefix_size, database_exception, "Unexpected key in rocksdb");

         auto key_buffer = std::vector<char>{ key.data(), key.data() + key.size() };
         auto begin = std::begin(key_buffer) + prefix_key.size();
         auto end = std::begin(key_buffer) + key_prefix_size;
         b1::chain_kv::extract_key(begin, end, contract);

         // In KV RocksDB, payer and actual data are packed together.
         // Extract them.
         auto value = (*it).second;

         const char* post_contract = key.data() + key_prefix_size;
         const std::size_t remaining = key.size() - key_prefix_size;
         function(contract, post_contract, remaining, value->data(), value->size());
      }
   }

   template <typename F>
   void walk_index(const index_utils<kv_index>& utils, eosio::session::undo_stack<rocks_db_type>* kv_undo_stack,
                   const chainbase::database& db, F&& function) {
      if (kv_undo_stack && db.get<kv_db_config_object>().backing_store == backing_store_type::ROCKSDB) {
         auto undo = false;
         if (kv_undo_stack->empty()) {
            // Get a session to iterate over.
            kv_undo_stack->push();
            undo = true;
         }
         for_each_rocksdb_index_with_prefix(
               *kv_undo_stack, rocksdb_contract_kv_prefix,
               [&function](uint64_t contract, const char* key, std::size_t key_size, const char* value,
                           std::size_t value_size) {
                  backing_store::payer_payload pp(value, value_size);
                  kv_object_view row{name(contract),
                                     {{key, key + key_size}},
                                      {{pp.value, pp.value + pp.value_size}},
                                     pp.payer};
                  function(row);
               });
         if (undo) {
            kv_undo_stack->undo();
         }

         utils.walk<by_kv_key>(db, function);
      }
      else  {
         utils.walk(db, function);
      }
   }

   combined_database::combined_database(chainbase::database& chain_db)
       : backing_store(backing_store_type::CHAINBASE), db(chain_db) {}

   combined_database::combined_database(chainbase::database& chain_db,
                                        const controller::config& cfg)
       : backing_store(backing_store_type::ROCKSDB), db(chain_db), kv_database{ [&]() {
            rocksdb::Options options;

            options.create_if_missing = !cfg.read_only; // Creates a database if it is missing
            options.level_compaction_dynamic_level_bytes = true;
            options.bytes_per_sync = 1048576; // used to control the write rate of flushes and compactions.

            // By default, RocksDB uses only one background thread
            // for flush and compaction.
            // Good value for `total_threads` is the number of cores
            options.IncreaseParallelism(cfg.rocksdb_threads);

            options.OptimizeLevelStyleCompaction(512ull << 20); // optimizes level style compaction

            // Number of open files that can be used by the DB.  
            // Setting it to -1 means files opened are always kept open.
            options.max_open_files = cfg.rocksdb_max_open_files;

            // Use this option to increase the number of threads
            // used to open the files.
            options.max_file_opening_threads = cfg.rocksdb_threads; // Default should be the # of Cores

            // Write Buffer Size - Sets the size of a single
            // memtable. Once memtable exceeds this size, it is
            // marked immutable and a new one is created.
            // Default should be 128MB
            options.write_buffer_size = cfg.rocksdb_write_buffer_size;
            options.max_write_buffer_number = 10; // maximum number of memtables, both active and immutable
            options.min_write_buffer_number_to_merge = 2; // minimum number of memtables to be merged before flushing to storage

            // Once level 0 reaches this number of files, L0->L1 compaction is triggered.
            options.level0_file_num_compaction_trigger = 2;

            // Size of L0 = write_buffer_size * min_write_buffer_number_to_merge * level0_file_num_compaction_trigger
            // For optimal performance make this equal to L0
            options.max_bytes_for_level_base = cfg.rocksdb_write_buffer_size * options.min_write_buffer_number_to_merge * options.level0_file_num_compaction_trigger; 

            // Files in level 1 will have target_file_size_base
            // bytes. It’s recommended setting target_file_size_base
            // to be max_bytes_for_level_base / 10.
            options.target_file_size_base = options.max_bytes_for_level_base / 10;

            // This value represents the maximum number of threads
            // that will concurrently perform a compaction job by
            // breaking it into multiple,
            // smaller ones that are run simultaneously.
            options.max_subcompactions = cfg.rocksdb_threads;	// Default should be the # of CPUs

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
         kv_undo_stack(std::make_unique<eosio::session::undo_stack<rocks_db_type>>(*kv_database)) {}

   void combined_database::check_backing_store_setting() {
      switch (backing_store) {
      case backing_store_type::CHAINBASE:
         EOS_ASSERT(db.get<kv_db_config_object>().backing_store == backing_store_type::CHAINBASE, database_move_kv_disk_exception,
                    "Chainbase indicates that RocksDB is in use; resync, replay, or restore from snapshot to switch back to chainbase");
         break;
      case backing_store_type::ROCKSDB:
         if (db.get<kv_db_config_object>().backing_store != backing_store_type::ROCKSDB) {
            auto& idx = db.get_index<kv_index, by_kv_key>();
            auto  it  = idx.lower_bound(boost::make_tuple(name{}, std::string_view{}));
            EOS_ASSERT(it == idx.end(), database_move_kv_disk_exception,
                     "Chainbase already contains KV entries; use resync, replay, or snapshot to move these to "
                     "rocksdb");
            db.modify(db.get<kv_db_config_object>(), [](auto& cfg) { cfg.backing_store = backing_store_type::ROCKSDB; });
         }
      }
      if (backing_store == backing_store_type::ROCKSDB)
         ilog("using rocksdb for backing store");
      else
         ilog("using chainbase for backing store");
   }

   void combined_database::set_revision(uint64_t revision) {
      switch (backing_store) {
      case backing_store_type::CHAINBASE:
         db.set_revision(revision);
         break;
      case backing_store_type::ROCKSDB:
         try {
            try {
                db.set_revision(revision);
                kv_undo_stack->revision(revision);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }
   }

   void combined_database::undo() {
      switch (backing_store) {
      case backing_store_type::CHAINBASE:
         db.undo();
         break;
      case backing_store_type::ROCKSDB:
         try {
            try {
              db.undo();
              kv_undo_stack->undo();
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }
   }

   void combined_database::commit(int64_t revision) {
      switch (backing_store) {
      case backing_store_type::CHAINBASE:
         db.commit(revision);
         break;
      case backing_store_type::ROCKSDB:
         try {
            try {
               db.commit(revision);
               kv_undo_stack->commit(revision);
            }
            FC_LOG_AND_RETHROW()
         }
         CATCH_AND_EXIT_DB_FAILURE()
      }
   }

   void combined_database::flush() {
      switch (backing_store) {
      case backing_store_type::CHAINBASE:
         break;
      case backing_store_type::ROCKSDB:
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
                                                                    const kv_database_config& limits) {
      switch (backing_store) {
         case backing_store_type::ROCKSDB:
            return create_kv_rocksdb_context<session_type, kv_resource_manager>(kv_undo_stack->top(), receiver,
                                                                                resource_manager, limits);
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
            walk_index(utils, this->kv_undo_stack.get(), db, [this, &section](const auto& row) { section.add_row(row, db); });
         });
      });

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
      check_backing_store_setting();

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

         // skip the kv index if the snapshot doesn't contain it
         if constexpr (std::is_same_v<value_t, kv_object>) {
            if (header.version < kv_object::minimum_snapshot_version)
               return;
            if (backing_store == backing_store_type::ROCKSDB) {
               auto prefix_key = eosio::session::shared_bytes(rocksdb_contract_kv_prefix.data(),
                                                                   rocksdb_contract_kv_prefix.size());
               auto key_values = std::vector<std::pair<eosio::session::shared_bytes, eosio::session::shared_bytes>>{};
               snapshot->read_section<value_t>([this, &key_values, &prefix_key](auto& section) {
                  bool more = !section.empty();
                  while (more) {
                     kv_object* move_to_rocks = nullptr;
                     decltype(utils)::create(db, [this, &section, &more, &move_to_rocks](auto& row) {
                        more = section.read_row(row, db);
                        move_to_rocks = &row;
                     });
                     if (move_to_rocks) {
                        auto buffer = std::vector<char>{};
                        buffer.reserve(sizeof(uint64_t) + prefix_key.size() + move_to_rocks->kv_key.size());
                        buffer.insert(std::end(buffer), prefix_key.data(), prefix_key.data() + prefix_key.size());
                        b1::chain_kv::append_key(buffer, move_to_rocks->contract.to_uint64_t());
                        buffer.insert(std::end(buffer), move_to_rocks->kv_key.data(),
                                      move_to_rocks->kv_key.data() + move_to_rocks->kv_key.size());

                        // Pack payer and actual key value
                        auto final_kv_value = build_value(move_to_rocks->kv_value.data(), move_to_rocks->kv_value.size(), move_to_rocks->payer);

                        key_values.emplace_back(eosio::session::shared_bytes(buffer.data(), buffer.size()),
                                                std::move(final_kv_value));
db.remove(*move_to_rocks);
                     }
                  }
               });
               kv_database->write(key_values);
               return;
            }
         }

         snapshot->read_section<value_t>([this](auto& section) {
            bool more = !section.empty();
            while (more) {
               decltype(utils)::create(db, [this, &section, &more](auto& row) { more = section.read_row(row, db); });
            }
         });
      });

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

            auto tid_key      = boost::make_tuple(table_row.id);
            auto next_tid_key = boost::make_tuple(table_id_object::id_type(table_row.id._id + 1));

            unsigned_int size = utils_t::template size_range<by_table_id>(db, tid_key, next_tid_key);
            section.add_row(size, db);

            utils_t::template walk_range<by_table_id>(db, tid_key, next_tid_key, [&db, &section](const auto& row) {
               static int count = 0;
               if (row.t_id == 7 && row.primary_key == 1)
                  ++count;
               section.add_row(row, db);
            });
         });
      });
   }

   struct table_id_object_view {
      account_name   code;  //< code should not be changed within a chainbase modifier lambda
      scope_name     scope; //< scope should not be changed within a chainbase modifier lambda
      table_name     table; //< table should not be changed within a chainbase modifier lambda
      account_name   payer;
      uint32_t       count = 0; /// the number of elements in the table
   };

   struct blob {
      std::string data;
   };

   template<typename DataStream>
   inline DataStream& operator << ( DataStream& ds, const blob& b ) {
      fc::raw::pack(ds, b.data);
      return ds;
   }

   template<typename DataStream>
   inline DataStream& operator >> ( DataStream& ds, blob& b ) {
      fc::raw::unpack(ds, b.data);
      return ds;
   }

   struct primary_index_view {
      uint64_t     primary_key;
      account_name payer;
      blob     value;
   };

   template <typename SecondaryKeyType>
   struct secondary_index_view {
      using secondary_key_type = SecondaryKeyType;
      uint64_t primary_key;
      account_name payer;
      SecondaryKeyType secondary_key;
   };

   template <typename Section>
   struct rocksdb_contract_table_writer {
      using key_type = eosio::chain::backing_store::db_key_value_format::key_type;

      Section&                                   section_;
      const chainbase::database&                 db_;
      eosio::session::undo_stack<rocks_db_type>& kv_undo_stack_;
      std::vector<primary_index_view>            primary_indices;
      std::tuple<std::vector<secondary_index_view<uint64_t>>,
                 std::vector<secondary_index_view<uint128_t>>,
                 std::vector<secondary_index_view<key256_t>>,
                 std::vector<secondary_index_view<float64_t>>,
                 std::vector<secondary_index_view<float128_t>> >
          all_secondary_indices;

      using extract_index_member_fun_t = void (rocksdb_contract_table_writer::*)(b1::chain_kv::bytes::const_iterator, b1::chain_kv::bytes::const_iterator, const char*, std::size_t);
      const static extract_index_member_fun_t extract_index_member_fun[7];

      rocksdb_contract_table_writer(Section& s, const chainbase::database& db, eosio::session::undo_stack<rocks_db_type>& kus)
         : section_(s), db_(db), kv_undo_stack_(kus){}

      void extract_primary_index(b1::chain_kv::bytes::const_iterator remaining,
                                 b1::chain_kv::bytes::const_iterator key_end, const char* value,
                                 std::size_t value_size) {
         uint64_t primary_key;
         EOS_ASSERT(b1::chain_kv::extract_key(remaining, key_end, primary_key), bad_composite_key_exception,
                    "DB intrinsic key-value store composite key is malformed");
         backing_store::payer_payload pp(value, value_size);
         primary_indices.emplace_back(primary_index_view{primary_key, pp.payer, {std::string{pp.value, value + value_size}}});
      }

      template <typename IndexType>
      auto& secondary_indices() {
         return std::get<std::vector<secondary_index_view<IndexType>>>(all_secondary_indices);
      }

      template <typename IndexType>
      void extract_secondary_index(b1::chain_kv::bytes::const_iterator remaining,
                                   b1::chain_kv::bytes::const_iterator key_end, const char* value,
                                   std::size_t value_size) {
         auto&     indices = secondary_indices<IndexType>();
         IndexType secondary_key;
         uint64_t primary_key;
         EOS_ASSERT( b1::chain_kv::extract_key(remaining, key_end, secondary_key), bad_composite_key_exception, "DB intrinsic key-value store composite key is malformed" );
         EOS_ASSERT( b1::chain_kv::extract_key(remaining, key_end, primary_key), bad_composite_key_exception , "DB intrinsic key-value store composite key is malformed" );
         backing_store::payer_payload pp(value, value_size);
         indices.emplace_back(secondary_index_view<IndexType>{primary_key, pp.payer, secondary_key});
      }

      template <typename IndexType>
      void write_secondary_indices(IndexType) {
         auto&     indices = secondary_indices<IndexType>();
         auto add_row = [this](const auto& row) { section_.add_row(row, db_); };
         add_row(fc::unsigned_int(indices.size()));
         std::for_each (indices.begin(), indices.end(), add_row);
         indices.clear();
      }

      void operator() (uint64_t contract, const char* key, std::size_t key_size,
                     const char* value, std::size_t value_size){

         b1::chain_kv::bytes composite_key(key, key + key_size);
         auto [scope, table, remaining, type] = backing_store::db_key_value_format::get_prefix_thru_key_type(composite_key);

         if (type == key_type::table) {
            backing_store::payer_payload pp(value, value_size);

            auto add_row = [this](const auto& row) { section_.add_row(row, db_); };

            std::tuple<uint64_t, uint128_t, key256_t, float64_t, float128_t> secondary_key_types;
            uint32_t total_count = primary_indices.size();
            std::apply([this, &total_count](auto... index) {
               total_count += ( this->secondary_indices<decltype(index)>().size() + ...); }, secondary_key_types);

            add_row(table_id_object_view{account_name{contract}, scope, table, pp.payer, total_count});

            add_row(fc::unsigned_int(primary_indices.size()));
            std::for_each (primary_indices.begin(), primary_indices.end(), add_row);
            primary_indices.clear();
            std::apply([this](auto... index) { (this->write_secondary_indices(index),...); }, secondary_key_types);
         } else if (type != key_type::primary_to_sec) {
            std::invoke(extract_index_member_fun[ static_cast<int>(type) ], this, remaining, composite_key.end(), value, value_size);
         }
      }
   };

   template <typename Section>
   const typename rocksdb_contract_table_writer<Section>::extract_index_member_fun_t rocksdb_contract_table_writer<Section>::extract_index_member_fun[] = {
      &rocksdb_contract_table_writer<Section>::extract_primary_index,
      nullptr,
      &rocksdb_contract_table_writer<Section>::extract_secondary_index<uint64_t>,
      &rocksdb_contract_table_writer<Section>::extract_secondary_index<uint128_t>,
      &rocksdb_contract_table_writer<Section>::extract_secondary_index<key256_t>,
      &rocksdb_contract_table_writer<Section>::extract_secondary_index<float64_t>,
      &rocksdb_contract_table_writer<Section>::extract_secondary_index<float128_t>,
   };

   void combined_database::add_contract_tables_to_snapshot(const snapshot_writer_ptr& snapshot) const {
      snapshot->write_section("contract_tables", [this](auto& section) {
         if (kv_undo_stack && db.get<kv_db_config_object>().backing_store == backing_store_type::ROCKSDB) {
            rocksdb_contract_table_writer<std::decay_t < decltype(section)>> writer(section, db, *kv_undo_stack);
            auto undo = false;
            if (kv_undo_stack->empty()) {
               // Get a session to iterate over.
               kv_undo_stack->push();
               undo = true;
            }
            for_each_rocksdb_index_with_prefix(*kv_undo_stack, rocksdb_contract_db_prefix, writer);
            if (undo) {
               kv_undo_stack->undo();
            }
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
                                                   Section& section) {
      std::vector<std::pair<eosio::session::shared_bytes, eosio::session::shared_bytes>> batch;
      bool                more     = !section.empty();
      auto                read_row = [&section, &more, &db](auto& row) { more = section.read_row(row, db); };

      while (more) {
         // read the row for the table
         table_id_object_view table_obj;
         read_row(table_obj);
         auto put = [&batch, &table_obj](auto&& value, auto create_fun, auto&&... args) {
            auto composite_key = create_fun(table_obj.scope, table_obj.table, std::forward<decltype(args)>(args)...);
            batch.emplace_back(backing_store::db_key_value_format::create_full_key(composite_key, table_obj.code),
                               std::forward<decltype(value)>(value));
         };

         // handle the primary key index
         unsigned_int size;
         read_row(size);
         for (size_t i = 0; i < size.value; ++i) {
            primary_index_view row;
            read_row(row);
            backing_store::payer_payload pp{row.payer, row.value.data.data(), row.value.data.size()};
            put(pp.as_payload(), backing_store::db_key_value_format::create_primary_key, row.primary_key);
         }

         auto write_secondary_index = [&put, &read_row](auto index) {
            using index_t = decltype(index);
            static const eosio::session::shared_bytes  empty_payload;
            unsigned_int       size;
            read_row(size);
            // std::cout << " " << size;
            for (int i = 0; i < size.value; ++i) {
               secondary_index_view<index_t> row;
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
            rocksdb_read_contract_tables_from_snapshot(*kv_database, db, section);
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

   std::vector<char> make_rocksdb_undo_prefix() { return rocksdb_undo_prefix; }
   std::vector<char> make_rocksdb_contract_kv_prefix() { return rocksdb_contract_kv_prefix; }
   std::vector<char> make_rocksdb_contract_db_prefix() { return rocksdb_contract_db_prefix; }

}} // namespace eosio::chain

FC_REFLECT(eosio::chain::table_id_object_view, (code)(scope)(table)(payer)(count) )
FC_REFLECT(eosio::chain::primary_index_view, (primary_key)(payer)(value) )
REFLECT_SECONDARY(eosio::chain::secondary_index_view<uint64_t>)
REFLECT_SECONDARY(eosio::chain::secondary_index_view<eosio::chain::uint128_t>)
REFLECT_SECONDARY(eosio::chain::secondary_index_view<eosio::chain::key256_t>)
REFLECT_SECONDARY(eosio::chain::secondary_index_view<float64_t>)
REFLECT_SECONDARY(eosio::chain::secondary_index_view<float128_t>)

namespace fc {
   inline
   void to_variant( const eosio::chain::blob& b, variant& v ) {
      v = variant(base64_encode(b.data.data(), b.data.size()));
   }

   inline
   void from_variant( const variant& v, eosio::chain::blob& b ) {
      b.data = base64_decode(v.as_string());
   }
}
