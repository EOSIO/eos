#include <eosio/producer_plugin/pending_snapshot.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio {

producer_plugin::snapshot_information pending_snapshot::finalize( const chain::controller& chain ) const {
    auto block_ptr = chain.fetch_block_by_id( block_id );
    auto in_chain = (bool)block_ptr;
    boost::system::error_code ec;

    if (!in_chain) {
       bfs::remove(bfs::path(pending_path), ec);
       EOS_THROW(chain::snapshot_finalization_exception,
                 "Snapshotted block was forked out of the chain.  ID: ${block_id}",
                 ("block_id", block_id));
    }

    bfs::rename(bfs::path(pending_path), bfs::path(final_path), ec);
    EOS_ASSERT(!ec, chain::snapshot_finalization_exception,
               "Unable to finalize valid snapshot of block number ${bn}: [code: ${ec}] ${message}",
               ("bn", get_height())
               ("ec", ec.value())
               ("message", ec.message()));

    return {block_id, block_ptr->block_num(), block_ptr->timestamp, chain::chain_snapshot_header::current_version, final_path};
}

} // namespace eosio
