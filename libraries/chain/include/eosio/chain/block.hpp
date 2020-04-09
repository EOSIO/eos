#pragma once
#include <eosio/chain/block_header.hpp>
#include <eosio/chain/transaction.hpp>

namespace eosio { namespace chain {

   /**
    * When a transaction is referenced by a block it could imply one of several outcomes which
    * describe the state-transition undertaken by the block producer.
    */

   struct transaction_receipt_header {
      enum status_enum {
         executed  = 0, ///< succeed, no error handler executed
         soft_fail = 1, ///< objectively failed (not executed), error handler executed
         hard_fail = 2, ///< objectively failed and error handler objectively failed thus no state change
         delayed   = 3, ///< transaction delayed/deferred/scheduled for future execution
         expired   = 4  ///< transaction expired and storage space refuned to user
      };

      transaction_receipt_header():status(hard_fail){}
      explicit transaction_receipt_header( status_enum s ):status(s){}

      friend inline bool operator ==( const transaction_receipt_header& lhs, const transaction_receipt_header& rhs ) {
         return std::tie(lhs.status, lhs.cpu_usage_us, lhs.net_usage_words) == std::tie(rhs.status, rhs.cpu_usage_us, rhs.net_usage_words);
      }

      fc::enum_type<uint8_t,status_enum>   status;
      uint32_t                             cpu_usage_us = 0; ///< total billed CPU usage (microseconds)
      fc::unsigned_int                     net_usage_words; ///<  total billed NET usage, so we can reconstruct resource state when skipping context free data... hard failures...
   };

   struct transaction_receipt : public transaction_receipt_header {
      using trx_type = fc::static_variant<transaction_id_type, packed_transaction>;
      transaction_receipt() : transaction_receipt_header() {}
      transaction_receipt(const transaction_receipt_header& header, trx_type&& t): transaction_receipt_header(header), trx(std::move(t)){}
      explicit transaction_receipt( const transaction_id_type& tid ):transaction_receipt_header(executed),trx(tid){}
      explicit transaction_receipt( const packed_transaction& ptrx ):transaction_receipt_header(executed),trx(ptrx){}

      trx_type trx;

      digest_type digest()const {
         digest_type::encoder enc;
         fc::raw::pack( enc, status );
         fc::raw::pack( enc, cpu_usage_us );
         fc::raw::pack( enc, net_usage_words );
         if( trx.contains<transaction_id_type>() )
            fc::raw::pack( enc, trx.get<transaction_id_type>() );
         else
            fc::raw::pack( enc, trx.get<packed_transaction>().packed_digest() );
         return enc.result();
      }
   };

   struct additional_block_signatures_extension : fc::reflect_init {
      static constexpr uint16_t extension_id() { return 2; }
      static constexpr bool     enforce_unique() { return true; }

      additional_block_signatures_extension() = default;

      additional_block_signatures_extension( const vector<signature_type>& signatures )
      :signatures( signatures )
      {}

      additional_block_signatures_extension( vector<signature_type>&& signatures )
      :signatures( std::move(signatures) )
      {}

      void reflector_init();

      vector<signature_type> signatures;
   };

   namespace detail {
      template<typename... Ts>
      struct block_extension_types {
         using block_extension_t = fc::static_variant< Ts... >;
         using decompose_t = decompose< Ts... >;
      };
   }

   using block_extension_types = detail::block_extension_types<
         additional_block_signatures_extension
   >;

   using block_extension = block_extension_types::block_extension_t;

   /**
    */
   struct signed_block : public signed_block_header{
   private:
      signed_block( const signed_block& ) = default;
   public:
      signed_block() = default;
      explicit signed_block( const signed_block_header& h ):signed_block_header(h){}
      signed_block( signed_block&& ) = default;
      signed_block& operator=(const signed_block&) = delete;
      signed_block clone() const { return *this; }

      deque<transaction_receipt>    transactions; /// new or generated transactions
      extensions_type               block_extensions;

      flat_multimap<uint16_t, block_extension> validate_and_extract_extensions()const;
   };
   using signed_block_ptr = std::shared_ptr<signed_block>;

   struct pruned_transaction_receipt : public transaction_receipt_header {

      pruned_transaction_receipt():transaction_receipt_header(){}
      pruned_transaction_receipt(const transaction_receipt&, bool legacy);
      explicit pruned_transaction_receipt( const transaction_id_type& tid ):transaction_receipt_header(executed),trx(tid){}
      explicit pruned_transaction_receipt( const pruned_transaction& ptrx ):transaction_receipt_header(executed),trx(ptrx){}

      fc::static_variant<transaction_id_type, pruned_transaction> trx;

      std::size_t maximum_pruned_pack_size( pruned_transaction::cf_compression_type segment_compression ) const;

      digest_type digest()const {
         digest_type::encoder enc;
         fc::raw::pack( enc, status );
         fc::raw::pack( enc, cpu_usage_us );
         fc::raw::pack( enc, net_usage_words );
         if( trx.contains<transaction_id_type>() )
            fc::raw::pack( enc, trx.get<transaction_id_type>() );
         else
            fc::raw::pack( enc, trx.get<pruned_transaction>().packed_digest() );
         return enc.result();
      }
   };

   struct pruned_block : public signed_block_header{
   private:
      pruned_block( const pruned_block& ) = default;
   public:
      enum class prune_state_type : uint8_t { incomplete, complete, complete_legacy };

      pruned_block() = default;
      explicit pruned_block( const signed_block_header& h ):signed_block_header(h){}
      pruned_block( const signed_block&, bool legacy );
      pruned_block( pruned_block&& ) = default;
      pruned_block& operator=(const pruned_block&) = delete;
      pruned_block clone() const { return *this; }
      signed_block_ptr to_signed_block() const;

      fc::enum_type<uint8_t,prune_state_type> prune_state{prune_state_type::complete_legacy};
      deque<pruned_transaction_receipt>       transactions; /// new or generated transactions
      extensions_type                         block_extensions;

      std::size_t maximum_pruned_pack_size( pruned_transaction::cf_compression_type segment_compression ) const;

      // Returns the maximum_pruned_padded_size.  It is the caller's responsibility to
      // reserve enough space after the end if in-place pruning is desired.
      template<typename Stream>
      std::size_t pack(Stream& stream, pruned_transaction::cf_compression_type segment_compression) {
         std::size_t padded_size = maximum_pruned_pack_size( segment_compression );
         // TODO: This only handles legacy transactions.
         fc::raw::pack(stream, *this);
         return padded_size;
      }
      template<typename Stream>
      void unpack(Stream& stream, pruned_transaction::cf_compression_type segment_compression) {
         fc::raw::unpack(stream, *this);
      }

      flat_multimap<uint16_t, block_extension> validate_and_extract_extensions()const;
   };
   using pruned_block_ptr = std::shared_ptr<pruned_block>;

   struct producer_confirmation {
      block_id_type   block_id;
      digest_type     block_digest;
      account_name    producer;
      signature_type  sig;
   };

} } /// eosio::chain

FC_REFLECT_ENUM( eosio::chain::transaction_receipt_header::status_enum,
                 (executed)(soft_fail)(hard_fail)(delayed)(expired) )

FC_REFLECT(eosio::chain::transaction_receipt_header, (status)(cpu_usage_us)(net_usage_words) )
FC_REFLECT_DERIVED(eosio::chain::transaction_receipt, (eosio::chain::transaction_receipt_header), (trx) )
FC_REFLECT(eosio::chain::additional_block_signatures_extension, (signatures));
FC_REFLECT_DERIVED(eosio::chain::signed_block, (eosio::chain::signed_block_header), (transactions)(block_extensions) )

FC_REFLECT_DERIVED(eosio::chain::pruned_transaction_receipt, (eosio::chain::transaction_receipt_header), (trx) )
FC_REFLECT_DERIVED(eosio::chain::pruned_block, (eosio::chain::signed_block_header), (prune_state)(transactions)(block_extensions) )
