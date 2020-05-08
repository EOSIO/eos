#include <eosio/chain/block_header.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/producer_plugin/pending_snapshot.hpp>

eosio::pending_snapshot::pending_snapshot(const eosio::chain::block_id_type& block_id, eosio::pending_snapshot::next_t& next, std::string pending_path, std::string final_path)
: block_id(block_id)
, next(next)
, pending_path(pending_path)
, final_path(final_path)
{
}

eosio::producer_plugin::snapshot_information eosio::pending_snapshot::finalize( const chain::controller& chain ) const {
   auto in_chain = (bool)chain.fetch_block_by_id( block_id );
   boost::system::error_code ec;

   if (!in_chain) {
      boost::filesystem::remove(boost::filesystem::path(pending_path), ec);
      EOS_THROW(eosio::chain::snapshot_finalization_exception,
                "Snapshotted block was forked out of the chain.  ID: ${block_id}",
                ("block_id", block_id));
   }

   boost::filesystem::rename(boost::filesystem::path(pending_path), boost::filesystem::path(final_path), ec);
   EOS_ASSERT(!ec, eosio::chain::snapshot_finalization_exception,
              "Unable to finalize valid snapshot of block number ${bn}: [code: ${ec}] ${message}",
              ("bn", eosio::pending_snapshot::get_height())
              ("ec", ec.value())
              ("message", ec.message()));

   return {eosio::pending_snapshot::block_id, eosio::pending_snapshot::final_path};
}

uint32_t eosio::pending_snapshot::get_height() const {
   return eosio::chain::block_header::num_from_id(block_id);
}
