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
      transaction_header() = default;
      explicit transaction_header( const transaction_header& ) = default;
      transaction_header( transaction_header&& ) = default;
      transaction_header& operator=(const transaction_header&) = delete;
      transaction_header& operator=(transaction_header&&) = default;

      time_point_sec         expiration;   ///< the time at which a transaction expires
      uint16_t               ref_block_num       = 0U; ///< specifies a block num in the last 2^16 blocks.
      uint32_t               ref_block_prefix    = 0UL; ///< specifies the lower 32 bits of the blockid at get_ref_blocknum
      fc::unsigned_int       max_net_usage_words = 0UL; /// upper limit on total network bandwidth (in 8 byte words) billed for this transaction
      uint8_t                max_cpu_usage_ms    = 0; /// upper limit on the total CPU time billed for this transaction
      fc::unsigned_int       delay_sec           = 0UL; /// number of seconds to delay this transaction for during which it may be canceled.

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
      transaction() = default;
      explicit transaction( const transaction& ) = default;
      transaction( transaction&& ) = default;
      transaction& operator=(const transaction&) = delete;
      transaction& operator=(transaction&&) = default;

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
      explicit signed_transaction( const signed_transaction& ) = default;
      signed_transaction( signed_transaction&& ) = default;
      signed_transaction& operator=(const signed_transaction&) = delete;
      signed_transaction& operator=(signed_transaction&&) = default;
      signed_transaction( transaction trx, vector<signature_type> signatures, vector<bytes> context_free_data)
      : transaction(std::move(trx))
      , signatures(std::move(signatures))
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

   struct packed_transaction_v0 : fc::reflect_init {
      enum class compression_type {
         none = 0,
         zlib = 1,
      };

      packed_transaction_v0() = default;
      packed_transaction_v0(packed_transaction_v0&&) = default;
      explicit packed_transaction_v0(const packed_transaction_v0&) = default;
      packed_transaction_v0& operator=(const packed_transaction_v0&) = delete;
      packed_transaction_v0& operator=(packed_transaction_v0&&) = default;

      explicit packed_transaction_v0(const signed_transaction& t, compression_type _compression = compression_type::none)
      :signatures(t.signatures), compression(_compression), unpacked_trx(t), trx_id(unpacked_trx.id())
      {
         local_pack_transaction();
         local_pack_context_free_data();
      }

      explicit packed_transaction_v0(signed_transaction&& t, compression_type _compression = compression_type::none)
      :signatures(t.signatures), compression(_compression), unpacked_trx(std::move(t)), trx_id(unpacked_trx.id())
      {
         local_pack_transaction();
         local_pack_context_free_data();
      }

      packed_transaction_v0(const bytes& packed_txn, const vector<signature_type>& sigs, const bytes& packed_cfd, compression_type _compression);

      // used by abi_serializer
      packed_transaction_v0( bytes&& packed_txn, vector<signature_type>&& sigs, bytes&& packed_cfd, compression_type _compression );
      packed_transaction_v0( bytes&& packed_txn, vector<signature_type>&& sigs, vector<bytes>&& cfd, compression_type _compression );
      packed_transaction_v0( transaction&& t, vector<signature_type>&& sigs, bytes&& packed_cfd, compression_type _compression );

      uint32_t get_unprunable_size()const;
      uint32_t get_prunable_size()const;

      digest_type packed_digest()const;

      const transaction_id_type& id()const { return trx_id; }

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

      friend struct fc::reflector<packed_transaction_v0>;
      friend struct fc::reflector_init_visitor<packed_transaction_v0>;
      friend struct fc::has_reflector_init<packed_transaction_v0>;
      friend struct packed_transaction;
      void reflector_init();
   private:
     vector<signature_type>                   signatures;
     fc::enum_type<uint8_t, compression_type> compression;
     bytes                                    packed_context_free_data;
     bytes                                    packed_trx;

   private:
      // cache unpacked trx, for thread safety do not modify after construction
      signed_transaction                      unpacked_trx;
      transaction_id_type                     trx_id;
   };

   using packed_transaction_v0_ptr = std::shared_ptr<const packed_transaction_v0>;

   struct packed_transaction : fc::reflect_init {

      struct prunable_data_type {
         enum class compression_type : uint8_t {
            none = 0,
            zlib = 1,
            COMPRESSION_TYPE_COUNT
         };
         // Do not exceed 127 as that will break compatibility in serialization format
         static_assert( static_cast<uint8_t>(compression_type::COMPRESSION_TYPE_COUNT) <= 127 );

         struct none {
            digest_type                     prunable_digest;
         };

         struct signatures_only {
            std::vector<signature_type>     signatures;
            digest_type                     context_free_mroot;
         };

         using segment_type = fc::static_variant<digest_type, bytes>;

         struct partial {
            std::vector<signature_type>     signatures;
            std::vector<segment_type>       context_free_segments;
         };

         struct full {
            std::vector<signature_type>     signatures;
            std::vector<bytes>              context_free_segments;
         };

         struct full_legacy {
            std::vector<signature_type>     signatures;
            bytes                           packed_context_free_data;
            vector<bytes>                   context_free_segments;
         };

         using prunable_data_t = fc::static_variant< full_legacy,
                                                     none,
                                                     signatures_only,
                                                     partial,
                                                     full >;

         prunable_data_type prune_all() const;
         digest_type digest() const;

         // Returns the maximum pack size of any prunable_data that is reachable
         // by pruning this object.
         std::size_t maximum_pruned_pack_size( compression_type segment_compression ) const;

         prunable_data_t  prunable_data;
      };

      using compression_type = packed_transaction_v0::compression_type;
      using cf_compression_type = prunable_data_type::compression_type;

      packed_transaction() = default;
      packed_transaction(packed_transaction&&) = default;
      explicit packed_transaction(const packed_transaction&) = default;
      packed_transaction& operator=(const packed_transaction&) = delete;
      packed_transaction& operator=(packed_transaction&&) = default;

      packed_transaction(const packed_transaction_v0& other, bool legacy);
      packed_transaction(packed_transaction_v0&& other, bool legacy);
      explicit packed_transaction(signed_transaction t, bool legacy, compression_type _compression = compression_type::none);

      packed_transaction_v0_ptr to_packed_transaction_v0() const;

      uint32_t get_unprunable_size()const;
      uint32_t get_prunable_size()const;
      uint32_t get_estimated_size()const { return estimated_size; }

      digest_type packed_digest()const;

      const transaction_id_type& id()const { return trx_id; }

      time_point_sec                expiration()const { return unpacked_trx.expiration; }
      const transaction&            get_transaction()const { return unpacked_trx; }
      // Returns nullptr if the signatures were pruned
      const vector<signature_type>* get_signatures()const;
      // Returns nullptr if any context_free_data segment was pruned
      const vector<bytes>*          get_context_free_data()const;
      // Returns nullptr if the context_free_data segment was pruned or segment_ordinal is out of range.
      const bytes*                  get_context_free_data(std::size_t segment_ordinal)const;
      const fc::enum_type<uint8_t,compression_type>& get_compression()const { return compression; }
      const bytes&                  get_packed_transaction()const { return packed_trx; }
      const packed_transaction::prunable_data_type& get_prunable_data() const { return prunable_data; }

      void prune_all();

      std::size_t maximum_pruned_pack_size( cf_compression_type segment_compression ) const;

   private:
      friend struct fc::reflector<packed_transaction>;
      friend struct fc::reflector_init_visitor<packed_transaction>;
      friend struct fc::has_reflector_init<packed_transaction>;
      void reflector_init();
      uint32_t calculate_estimated_size() const;
   private:
      uint32_t                                estimated_size = 0;
      fc::enum_type<uint8_t,compression_type> compression;
      prunable_data_type                      prunable_data;
      bytes                                   packed_trx; // packed and compressed (according to compression) transaction

   private:
      // cache unpacked trx, for thread safety do not modify any attributes after construction
      transaction                             unpacked_trx;
      transaction_id_type                     trx_id;
   };

   using packed_transaction_ptr = std::shared_ptr<const packed_transaction>;

   uint128_t transaction_id_to_sender_id( const transaction_id_type& tid );

} } /// namespace eosio::chain

FC_REFLECT(eosio::chain::deferred_transaction_generation_context, (sender_trx_id)(sender_id)(sender) )
FC_REFLECT( eosio::chain::transaction_header, (expiration)(ref_block_num)(ref_block_prefix)
                                              (max_net_usage_words)(max_cpu_usage_ms)(delay_sec) )
FC_REFLECT_DERIVED( eosio::chain::transaction, (eosio::chain::transaction_header), (context_free_actions)(actions)(transaction_extensions) )
FC_REFLECT_DERIVED( eosio::chain::signed_transaction, (eosio::chain::transaction), (signatures)(context_free_data) )
FC_REFLECT_ENUM( eosio::chain::packed_transaction_v0::compression_type, (none)(zlib))
// @ignore unpacked_trx trx_id
FC_REFLECT( eosio::chain::packed_transaction_v0, (signatures)(compression)(packed_context_free_data)(packed_trx) )
// @ignore estimated_size unpacked_trx trx_id
FC_REFLECT( eosio::chain::packed_transaction, (compression)(prunable_data)(packed_trx) )
FC_REFLECT( eosio::chain::packed_transaction::prunable_data_type, (prunable_data));
FC_REFLECT( eosio::chain::packed_transaction::prunable_data_type::none, (prunable_digest))
FC_REFLECT( eosio::chain::packed_transaction::prunable_data_type::signatures_only, (signatures)( context_free_mroot))
FC_REFLECT( eosio::chain::packed_transaction::prunable_data_type::partial, (signatures)( context_free_segments))
FC_REFLECT( eosio::chain::packed_transaction::prunable_data_type::full, (signatures)( context_free_segments))
// @ignore context_free_segments
FC_REFLECT( eosio::chain::packed_transaction::prunable_data_type::full_legacy, (signatures)( packed_context_free_data))
