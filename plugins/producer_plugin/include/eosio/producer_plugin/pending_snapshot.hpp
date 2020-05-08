#pragma once

#include <boost/filesystem/path.hpp>

#include <eosio/chain/types.hpp>
#include <eosio/producer_plugin/common.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>

#include <string>

namespace eosio {

class pending_snapshot {
public:
   using next_t = eosio::producer_plugin::next_function<eosio::producer_plugin::snapshot_information>;
   
   pending_snapshot( const eosio::chain::block_id_type& block_id, next_t& next, std::string pending_path, std::string final_path );

   eosio::producer_plugin::snapshot_information finalize( const eosio::chain::controller& chain ) const;
   
   uint32_t get_height() const;
   
   static boost::filesystem::path get_final_path( const eosio::chain::block_id_type& block_id, const boost::filesystem::path& snapshots_dir ) {
      return snapshots_dir / fc::format_string("snapshot-${id}.bin", fc::mutable_variant_object()("id", block_id));
   }

   static boost::filesystem::path get_pending_path( const eosio::chain::block_id_type& block_id, const boost::filesystem::path& snapshots_dir ) {
      return snapshots_dir / fc::format_string(".pending-snapshot-${id}.bin", fc::mutable_variant_object()("id", block_id));
   }

   static boost::filesystem::path get_temp_path( const eosio::chain::block_id_type& block_id, const boost::filesystem::path& snapshots_dir ) {
      return snapshots_dir / fc::format_string(".incomplete-snapshot-${id}.bin", fc::mutable_variant_object()("id", block_id));
   }
   
   eosio::chain::block_id_type block_id;
   next_t                      next;
   std::string                 pending_path;
   std::string                 final_path;
};

using pending_snapshot_index = boost::multi_index::multi_index_container<
   pending_snapshot,
      boost::multi_index::indexed_by<
      boost::multi_index::hashed_unique<tag<eosio::by_id>, BOOST_MULTI_INDEX_MEMBER( pending_snapshot, eosio::chain::block_id_type, pending_snapshot::block_id )>,
      boost::multi_index::ordered_non_unique<tag<eosio::by_height>, BOOST_MULTI_INDEX_CONST_MEM_FUN( pending_snapshot, uint32_t, pending_snapshot::get_height )>
   >
>;

}
