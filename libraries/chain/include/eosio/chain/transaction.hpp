#pragma once

#include <eosio/chain/action.hpp>
#include <numeric>

namespace eosio { namespace chain {

   struct deferred_transaction_generation_context : fc::reflect_init {
      static constexpr uint16_t extension_id() { return 0; }
      static constexpr bool     enforce_unique() { return true; }

      deferred_transaction_generation_context() = default;

      deferred_transaction_generation_context( const transaction_id_type& sender_trx_id, uint128_t sender_id, account_name sender )
      :sender_trx_id( sender_trx_id )
      ,sender_id( sender_id )
      ,sender( sender )
      {}

      void reflector_init();

      transaction_id_type sender_trx_id;
      uint128_t           sender_id;
      account_name        sender;
   };

   namespace detail {
      template<typename... Ts>
      struct transaction_extension_types {
         using transaction_extension_t = fc::static_variant< Ts... >;
         using decompose_t = decompose< Ts... >;
      };
   }

   using transaction_extension_types = detail::transaction_extension_types<
      deferred_transaction_generation_context
   >;

   using transaction_extension = transaction_extension_types::transaction_extension_t;

   /**
    *  The transaction header contains the fixed-sized data
    *  associated with each transaction. It is separated from
    *  the transaction body to facilitate partial parsing of
    *  transactions without requiring dynamic memory allocation.
    *
    *  All transactions have an expiration time after which they
    *  may no longer be included in the blockchain. Once a block
    *  with a block_header::timestamp greater than expiration is
    *  deemed irreversible, then a user can safely trust the transaction
    *  will never be included.
    *

    *  Each region is an independent blockchain, it is included as routing
    *  information for inter-blockchain communication. A contract in this
    *  region might generate or authorize a transaction intended for a foreign
    *  region.
    */
   struct transaction_header {
      time_point_sec         expiration;   ///< the time at which a transaction expires
      uint16_t               ref_block_num       = 0U; ///< specifies a block num in the last 2^16 blocks.
      uint32_t               ref_block_prefix    = 0UL; ///< specifies the lower 32 bits of the blockid at get_ref_blocknum
      fc::unsigned_int       max_net_usage_words = 0UL; /// upper limit on total network bandwidth (in 8 byte words) billed for this transaction
      uint8_t                max_cpu_usage_ms    = 0; /// upper limit on the total CPU time billed for this transaction
      fc::unsigned_int       delay_sec           = 0UL; /// number of seconds to delay this transaction for during which it may be canceled.

      /**
       * @return the absolute block number given the relative ref_block_num
       */
      block_num_type get_ref_blocknum( block_num_type head_blocknum )const {
         return ((head_blocknum/0xffff)*0xffff) + head_blocknum%0xffff;
      }
      void set_reference_block( const block_id_type& reference_block );
      bool verify_reference_block( const block_id_type& reference_block )const;
      void validate()const;
   };

   /**
    *  A transaction consits of a set of messages which must all be applied or
    *  all are rejected. These messages have access to data within the given
    *  read and write scopes.
    */
   struct transaction : public transaction_header {
      vector<action>         context_free_actions;
      vector<action>         actions;
      extensions_type        transaction_extensions;

      transaction_id_type        id()const;
      digest_type                sig_digest( const chain_id_type& chain_id, const vector<bytes>& cfd = vector<bytes>() )const;
      fc::microseconds           get_signature_keys( const vector<signature_type>& signatures,
                                                     const chain_id_type& chain_id,
                                                     fc::time_point deadline,
                                                     const vector<bytes>& cfd,
                                                     flat_set<public_key_type>& recovered_pub_keys,
                                                     bool allow_duplicate_keys = false) const;

      uint32_t total_actions()const { return context_free_actions.size() + actions.size(); }

      account_name first_authorizer()const {
         for( const auto& a : actions ) {
            for( const auto& u : a.authorization )
               return u.actor;
         }
         return account_name();
      }

      flat_multimap<uint16_t, transaction_extension> validate_and_extract_extensions()const;
   };

   struct signed_transaction : public transaction
   {
      signed_transaction() = default;
//      signed_transaction( const signed_transaction& ) = default;
//      signed_transaction( signed_transaction&& ) = default;
      signed_transaction( transaction&& trx, const vector<signature_type>& signatures, const vector<bytes>& context_free_data)
      : transaction(std::move(trx))
      , signatures(signatures)
      , context_free_data(context_free_data)
      {}
      signed_transaction( transaction&& trx, const vector<signature_type>& signatures, vector<bytes>&& context_free_data)
      : transaction(std::move(trx))
      , signatures(signatures)
      , context_free_data(std::move(context_free_data))
      {}

      vector<signature_type>    signatures;
      vector<bytes>             context_free_data; ///< for each context-free action, there is an entry here

      const signature_type&     sign(const private_key_type& key, const chain_id_type& chain_id);
      signature_type            sign(const private_key_type& key, const chain_id_type& chain_id)const;
      fc::microseconds          get_signature_keys( const chain_id_type& chain_id, fc::time_point deadline,
                                                    flat_set<public_key_type>& recovered_pub_keys,
                                                    bool allow_duplicate_keys = false )const;
   };

   struct packed_transaction : fc::reflect_init {
      enum class compression_type {
         none = 0,
         zlib = 1,
      };

      packed_transaction() = default;
      packed_transaction(packed_transaction&&) = default;
      explicit packed_transaction(const packed_transaction&) = default;
      packed_transaction& operator=(const packed_transaction&) = delete;
      packed_transaction& operator=(packed_transaction&&) = default;

      explicit packed_transaction(const signed_transaction& t, compression_type _compression = compression_type::none)
      :signatures(t.signatures), compression(_compression), unpacked_trx(t), trx_id(unpacked_trx.id())
      {
         local_pack_transaction();
         local_pack_context_free_data();
      }

      explicit packed_transaction(signed_transaction&& t, compression_type _compression = compression_type::none)
      :signatures(t.signatures), compression(_compression), unpacked_trx(std::move(t)), trx_id(unpacked_trx.id())
      {
         local_pack_transaction();
         local_pack_context_free_data();
      }

      // used by abi_serializer
      packed_transaction( bytes&& packed_txn, vector<signature_type>&& sigs, bytes&& packed_cfd, compression_type _compression );
      packed_transaction( bytes&& packed_txn, vector<signature_type>&& sigs, vector<bytes>&& cfd, compression_type _compression );
      packed_transaction( transaction&& t, vector<signature_type>&& sigs, bytes&& packed_cfd, compression_type _compression );

      uint32_t get_unprunable_size()const;
      uint32_t get_prunable_size()const;

      digest_type packed_digest()const;

      const transaction_id_type& id()const { return trx_id; }
      bytes               get_raw_transaction()const;

      time_point_sec                expiration()const { return unpacked_trx.expiration; }
      const vector<bytes>&          get_context_free_data()const { return unpacked_trx.context_free_data; }
      const transaction&            get_transaction()const { return unpacked_trx; }
      const signed_transaction&     get_signed_transaction()const { return unpacked_trx; }
      const vector<signature_type>& get_signatures()const { return signatures; }
      const fc::enum_type<uint8_t,compression_type>& get_compression()const { return compression; }
      const bytes&                  get_packed_context_free_data()const { return packed_context_free_data; }
      const bytes&                  get_packed_transaction()const { return packed_trx; }

   private:
      void local_unpack_transaction(vector<bytes>&& context_free_data);
      void local_unpack_context_free_data();
      void local_pack_transaction();
      void local_pack_context_free_data();

      friend struct fc::reflector<packed_transaction>;
      friend struct fc::reflector_init_visitor<packed_transaction>;
      friend struct fc::has_reflector_init<packed_transaction>;
      void reflector_init();
   private:
      vector<signature_type>                  signatures;
      fc::enum_type<uint8_t,compression_type> compression;
      bytes                                   packed_context_free_data;
      bytes                                   packed_trx;

   private:
      // cache unpacked trx, for thread safety do not modify after construction
      signed_transaction                      unpacked_trx;
      transaction_id_type                     trx_id;
   };

   struct prunable_transaction_data {
      enum class compression_type {
         none = 0,
         zlib = 1,
      };

      struct none {
         digest_type                     prunable_digest;
      };

      struct signatures_only {
         std::vector<signature_type>     signatures;
         digest_type                     context_free_mroot;
      };

      using segment_type = fc::static_variant<digest_type, std::vector<char>>;

      struct partial {
         std::vector<signature_type>     signatures;
         std::vector<segment_type>       context_free_segments;
      };

      struct full {
         std::vector<signature_type>     signatures;
         std::vector<std::vector<char>>  context_free_segments;
      };

      struct full_legacy {
         std::vector<signature_type>     signatures;
         std::vector<char>               packed_context_free_data;
      };

      using prunable_data_type = fc::static_variant< full_legacy,
                                                     none,
                                                     signatures_only,
                                                     partial,
                                                     full >;

      prunable_transaction_data prune_all() const;
      digest_type digest() const;

      // Returns the maximum pack size of any prunable_transaction_data that is reachable
      // by pruning this object.
      std::size_t maximum_pruned_pack_size( compression_type segment_compression ) const;

      prunable_data_type  prunable_data;
   };

   struct pruned_transaction : fc::reflect_init {
      using compression_type = packed_transaction::compression_type;
      using cf_compression_type = prunable_transaction_data::compression_type;

      pruned_transaction() = default;
      pruned_transaction(pruned_transaction&&) = default;
      explicit pruned_transaction(const pruned_transaction&) = default;
      pruned_transaction& operator=(const pruned_transaction&) = delete;
      pruned_transaction& operator=(pruned_transaction&&) = default;

      pruned_transaction(const packed_transaction& other, bool legacy) : pruned_transaction(other.get_signed_transaction(), legacy, other.get_compression()) {}
      explicit pruned_transaction(const signed_transaction& t, bool legacy, compression_type _compression = compression_type::none);
      explicit pruned_transaction(signed_transaction&& t, bool legacy, compression_type _compression = compression_type::none);

#if 0
      // used by abi_serializer
      packed_transaction( bytes&& packed_txn, vector<signature_type>&& sigs, bytes&& packed_cfd, compression_type _compression );
      packed_transaction( bytes&& packed_txn, vector<signature_type>&& sigs, vector<bytes>&& cfd, compression_type _compression );
      packed_transaction( transaction&& t, vector<signature_type>&& sigs, bytes&& packed_cfd, compression_type _compression );
#endif

      uint32_t get_unprunable_size()const;
      uint32_t get_prunable_size()const;

      digest_type packed_digest()const;

      const transaction_id_type& id()const { return trx_id; }

      time_point_sec                expiration()const { return unpacked_trx.expiration; }
      const transaction&            get_transaction()const { return unpacked_trx; }
      const signed_transaction&     get_signed_transaction()const { return unpacked_trx; }
      // Returns nullptr if the signatures were pruned
      const vector<signature_type>* get_signatures()const;
      // Returns nullptr if any context_free_data segment was pruned
      const vector<bytes>*          get_context_free_data()const;
      // Returns nullptr if the context_free_data segment was pruned or segment_ordinal is out of range.
      const bytes*                  get_context_free_data(std::size_t segment_ordinal);
      const fc::enum_type<uint8_t,compression_type>& get_compression()const { return compression; }
      const bytes&                  get_packed_transaction()const { return packed_trx; }

      void prune_all();

      std::size_t maximum_pruned_pack_size( cf_compression_type segment_compression ) const;

   private:

      friend struct fc::reflector<pruned_transaction>;
      friend struct fc::reflector_init_visitor<pruned_transaction>;
      friend struct fc::has_reflector_init<pruned_transaction>;
      void reflector_init();
   private:
      fc::enum_type<uint8_t,compression_type> compression;
      prunable_transaction_data               prunable_data;
      bytes                                   packed_trx;

   private:
      // cache unpacked trx, for thread safety do not modify after construction
      signed_transaction                      unpacked_trx;
      transaction_id_type                     trx_id;
   };

   using packed_transaction_ptr = std::shared_ptr<packed_transaction>;

   uint128_t transaction_id_to_sender_id( const transaction_id_type& tid );

} } /// namespace eosio::chain

FC_REFLECT(eosio::chain::deferred_transaction_generation_context, (sender_trx_id)(sender_id)(sender) )
FC_REFLECT( eosio::chain::transaction_header, (expiration)(ref_block_num)(ref_block_prefix)
                                              (max_net_usage_words)(max_cpu_usage_ms)(delay_sec) )
FC_REFLECT_DERIVED( eosio::chain::transaction, (eosio::chain::transaction_header), (context_free_actions)(actions)(transaction_extensions) )
FC_REFLECT_DERIVED( eosio::chain::signed_transaction, (eosio::chain::transaction), (signatures)(context_free_data) )
FC_REFLECT_ENUM( eosio::chain::packed_transaction::compression_type, (none)(zlib))
// @ignore unpacked_trx
FC_REFLECT( eosio::chain::packed_transaction, (signatures)(compression)(packed_context_free_data)(packed_trx) )
FC_REFLECT( eosio::chain::pruned_transaction, (compression)(prunable_data)(packed_trx) )
FC_REFLECT( eosio::chain::prunable_transaction_data, (prunable_data));
FC_REFLECT( eosio::chain::prunable_transaction_data::none, (prunable_digest))
FC_REFLECT( eosio::chain::prunable_transaction_data::signatures_only, (signatures)(context_free_mroot))
FC_REFLECT( eosio::chain::prunable_transaction_data::partial, (signatures)(context_free_segments))
FC_REFLECT( eosio::chain::prunable_transaction_data::full, (signatures)(context_free_segments))
FC_REFLECT( eosio::chain::prunable_transaction_data::full_legacy, (signatures)(packed_context_free_data))
