#pragma once

// #include <producer_plugin/common.hpp>
#include <eosio/producer_plugin/producer_plugin.hpp>

class pending_snapshot {
public:
   using next_t = eosio::producer_plugin::next_function<eosio::producer_plugin::snapshot_information>;

   pending_snapshot( const block_id_type& block_id, next_t& next, std::string pending_path, std::string final_path );
   
   uint32_t get_height() const;
   
   static boost::filesystem::path get_final_path  ( const block_id_type& block_id, const boost::filesystem::path& snapshots_dir );
   static boost::filesystem::path get_pending_path( const block_id_type& block_id, const boost::filesystem::path& snapshots_dir );
   static boost::filesystem::path get_temp_path   ( const block_id_type& block_id, const boost::filesystem::path& snapshots_dir );
   
   eosio::producer_plugin::snapshot_information finalize( const eosio::chain::controller& chain ) const;

   block_id_type     block_id;
   next_t            next;
   std::string       pending_path;
   std::string       final_path;
};

using pending_snapshot_index = multi_index_container<
   pending_snapshot,
   indexed_by<
      hashed_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(pending_snapshot, block_id_type, block_id)>,
      ordered_non_unique<tag<by_height>, BOOST_MULTI_INDEX_CONST_MEM_FUN( pending_snapshot, uint32_t, get_height)>
   >
>;
