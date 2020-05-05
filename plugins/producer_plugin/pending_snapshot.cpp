#include <eosio/producer_plugin/pending_snapshot.hpp>

pending_snapshot::pending_snapshot(const block_id_type& block_id, next_t& next, std::string pending_path, std::string final_path)
   : block_id(block_id)
   , next(next)
   , pending_path(pending_path)
   , final_path(final_path)
   {}

uint32_t pending_snapshot::get_height() const {
   return block_header::num_from_id(block_id);
}

static bfs::path pending_snapshot::get_final_path(const block_id_type& block_id, const bfs::path& snapshots_dir) {
   return snapshots_dir / fc::format_string("snapshot-${id}.bin", fc::mutable_variant_object()("id", block_id));
}

static bfs::path pending_snapshot::get_pending_path(const block_id_type& block_id, const bfs::path& snapshots_dir) {
   return snapshots_dir / fc::format_string(".pending-snapshot-${id}.bin", fc::mutable_variant_object()("id", block_id));
}

static bfs::path pending_snapshot::get_temp_path(const block_id_type& block_id, const bfs::path& snapshots_dir) {
   return snapshots_dir / fc::format_string(".incomplete-snapshot-${id}.bin", fc::mutable_variant_object()("id", block_id));
}

producer_plugin::snapshot_information finalize( const chain::controller& chain ) const {
   auto in_chain = (bool)chain.fetch_block_by_id( block_id );
   boost::system::error_code ec;

   if (!in_chain) {
      bfs::remove(bfs::path(pending_path), ec);
      EOS_THROW(snapshot_finalization_exception,
                "Snapshotted block was forked out of the chain.  ID: ${block_id}",
                ("block_id", block_id));
   }

   bfs::rename(bfs::path(pending_path), bfs::path(final_path), ec);
   EOS_ASSERT(!ec, snapshot_finalization_exception,
              "Unable to finalize valid snapshot of block number ${bn}: [code: ${ec}] ${message}",
              ("bn", get_height())
              ("ec", ec.value())
              ("message", ec.message()));

   return {block_id, final_path};
}
