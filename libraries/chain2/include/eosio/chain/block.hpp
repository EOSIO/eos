#pragma once
#include <eosio/chain/block_header.hpp>
#include <eosio/chain/transaction.hpp>

namespace eosio { namespace chain {

   struct shard_lock {
      account_name   account;
      scope_name     scope;

      friend bool operator <  ( const shard_lock& a, const shard_lock& b ) { return std::tie(a.account, a.scope) <  std::tie(b.account, b.scope); }
      friend bool operator <= ( const shard_lock& a, const shard_lock& b ) { return std::tie(a.account, a.scope) <= std::tie(b.account, b.scope); }
      friend bool operator >  ( const shard_lock& a, const shard_lock& b ) { return std::tie(a.account, a.scope) >  std::tie(b.account, b.scope); }
      friend bool operator >= ( const shard_lock& a, const shard_lock& b ) { return std::tie(a.account, a.scope) >= std::tie(b.account, b.scope); }
      friend bool operator == ( const shard_lock& a, const shard_lock& b ) { return std::tie(a.account, a.scope) == std::tie(b.account, b.scope); }
      friend bool operator != ( const shard_lock& a, const shard_lock& b ) { return std::tie(a.account, a.scope) != std::tie(b.account, b.scope); }
   };

   struct shard_summary {
      vector<shard_lock>            read_locks;
      vector<shard_lock>            write_locks;
      vector<transaction_receipt>   transactions; /// new or generated transactions

      bool empty() const {
         return read_locks.empty() && write_locks.empty() && transactions.empty();
      }
   };

   typedef vector<shard_summary>    cycle;

   struct region_summary {
      uint16_t region = 0;
      vector<cycle>    cycles_summary;
   };

   /**
    *  The block_summary defines the set of transactions that were successfully applied as they
    *  are organized into cycles and shards. A shard contains the set of transactions IDs which
    *  are either user generated transactions or code-generated transactions.
    *
    *
    *  The primary purpose of a block is to define the order in which messages are processed.
    *
    *  The secodnary purpose of a block is certify that the messages are valid according to
    *  a group of 3rd party validators (producers).
    *
    *  The next purpose of a block is to enable light-weight proofs that a transaction occured
    *  and was considered valid.
    *
    *  The next purpose is to enable code to generate messages that are certified by the
    *  producers to be authorized.
    *
    *  A block is therefore defined by the ordered set of executed and generated transactions,
    *  and the merkle proof is over set of messages delivered as a result of executing the
    *  transactions.
    *
    *  A message is defined by { receiver, code, function, permission, data }, the merkle
    *  tree of a block should be generated over a set of message IDs rather than a set of
    *  transaction ids.
    */
   struct signed_block_summary : public signed_block_header {
      vector<region_summary>    regions;
   };

   /**
    * This structure contains the set of signed transactions referenced by the
    * block summary. This inherits from block_summary/signed_block_header and is
    * what would be logged to disk to enable the regeneration of blockchain state.
    *
    * The transactions are grouped to mirror the cycles in block_summary, generated
    * transactions are not included.
    */
   struct signed_block : public signed_block_summary {
      signed_block () = default;
      signed_block (const signed_block& ) = default;
      signed_block (const signed_block_summary& base)
         :signed_block_summary (base),
          input_transactions()
      {}

      /// this is loaded and indexed into map<id,trx> that is referenced by summary; order doesn't matter
      vector<packed_transaction>   input_transactions;
   };

   typedef std::shared_ptr<signed_block> signed_block_ptr;


} } /// eosio::chain

FC_REFLECT( eosio::chain::shard_lock, (account)(scope))
FC_REFLECT( eosio::chain::shard_summary, (read_locks)(write_locks)(transactions))
FC_REFLECT( eosio::chain::region_summary, (region)(cycles_summary) )
FC_REFLECT_DERIVED(eosio::chain::signed_block_summary, (eosio::chain::signed_block_header), (regions))
FC_REFLECT_DERIVED(eosio::chain::signed_block, (eosio::chain::signed_block_summary), (input_transactions))
