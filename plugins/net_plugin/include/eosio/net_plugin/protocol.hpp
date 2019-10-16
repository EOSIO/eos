#pragma once
#include <eosio/chain/block.hpp>
#include <eosio/chain/types.hpp>
#include <chrono>
#include <eosio/topology_plugin/messages.hpp>

namespace eosio {
   using namespace chain;
   using namespace fc;

   static_assert(sizeof(std::chrono::system_clock::duration::rep) >= 8, "system_clock is expected to be at least 64 bits");
   typedef std::chrono::system_clock::duration::rep tstamp;

   struct chain_size_message {
      uint32_t                   last_irreversible_block_num = 0;
      block_id_type              last_irreversible_block_id;
      uint32_t                   head_num = 0;
      block_id_type              head_id;
   };

   struct handshake_message {
      uint16_t                   network_version = 0; ///< incremental value above a computed base
      chain_id_type              chain_id; ///< used to identify chain
      fc::sha256                 node_id; ///< used to identify peers and prevent self-connect
      chain::public_key_type     key; ///< authentication key; may be a producer or peer key, or empty
      tstamp                     time{0};
      fc::sha256                 token; ///< digest of time to prove we own the private key of the key above
      chain::signature_type      sig; ///< signature for the digest
      string                     p2p_address;
      uint32_t                   last_irreversible_block_num = 0;
      block_id_type              last_irreversible_block_id;
      uint32_t                   head_num = 0;
      block_id_type              head_id;
      string                     os;
      string                     agent;
      int16_t                    generation = 0;
   };


  enum go_away_reason {
    no_reason, ///< no reason to go away
    self, ///< the connection is to itself
    duplicate, ///< the connection is redundant
    wrong_chain, ///< the peer's chain id doesn't match
    wrong_version, ///< the peer's network version doesn't match
    forked, ///< the peer's irreversible blocks are different
    unlinkable, ///< the peer sent a block we couldn't use
    bad_transaction, ///< the peer sent a transaction that failed verification
    validation, ///< the peer sent a block that failed validation
    benign_other, ///< reasons such as a timeout. not fatal but warrant resetting
    fatal_other, ///< a catch-all for errors we don't have discriminated
    authentication ///< peer failed authenication
  };

  constexpr auto reason_str( go_away_reason rsn ) {
    switch (rsn ) {
    case no_reason : return "no reason";
    case self : return "self connect";
    case duplicate : return "duplicate";
    case wrong_chain : return "wrong chain";
    case wrong_version : return "wrong version";
    case forked : return "chain is forked";
    case unlinkable : return "unlinkable block received";
    case bad_transaction : return "bad transaction";
    case validation : return "invalid block";
    case authentication : return "authentication failure";
    case fatal_other : return "some other failure";
    case benign_other : return "some other non-fatal condition, possibly unknown block";
    default : return "some crazy reason";
    }
  }

  struct go_away_message {
    go_away_message (go_away_reason r = no_reason) : reason(r), node_id() {}
    go_away_reason reason;
    fc::sha256 node_id; ///< for duplicate notification
  };

  struct time_message {
            tstamp  org;       //!< origin timestamp
            tstamp  rec;       //!< receive timestamp
            tstamp  xmt;       //!< transmit timestamp
    mutable tstamp  dst;       //!< destination timestamp
  };

  enum id_list_modes {
    none,
    catch_up,
    last_irr_catch_up,
    normal
  };

  constexpr auto modes_str( id_list_modes m ) {
    switch( m ) {
    case none : return "none";
    case catch_up : return "catch up";
    case last_irr_catch_up : return "last irreversible";
    case normal : return "normal";
    default: return "undefined mode";
    }
  }

  template<typename T>
  struct select_ids {
    select_ids () : mode(none),pending(0),ids() {}
    id_list_modes  mode;
    uint32_t       pending;
    vector<T>      ids;
    bool           empty () const { return (mode == none || ids.empty()); }
  };

  using ordered_txn_ids = select_ids<transaction_id_type>;
  using ordered_blk_ids = select_ids<block_id_type>;

  struct notice_message {
    notice_message () : known_trx(), known_blocks() {}
    ordered_txn_ids known_trx;
    ordered_blk_ids known_blocks;
  };

  struct request_message {
    request_message () : req_trx(), req_blocks() {}
    ordered_txn_ids req_trx;
    ordered_blk_ids req_blocks;
  };

   struct sync_request_message {
      uint32_t start_block;
      uint32_t end_block;
   };

   using net_message = static_variant<handshake_message,
                                      chain_size_message,
                                      go_away_message,
                                      time_message,
                                      notice_message,
                                      request_message,
                                      sync_request_message,
                                      signed_block,         // which = 7
                                      packed_transaction,   // which = 8
                                      topology_message>;

} // namespace eosio

FC_REFLECT( eosio::select_ids<fc::sha256>, (mode)(pending)(ids) )
FC_REFLECT( eosio::chain_size_message,
            (last_irreversible_block_num)(last_irreversible_block_id)
            (head_num)(head_id))
FC_REFLECT( eosio::handshake_message,
            (network_version)(chain_id)(node_id)(key)
            (time)(token)(sig)(p2p_address)
            (last_irreversible_block_num)(last_irreversible_block_id)
            (head_num)(head_id)
            (os)(agent)(generation) )
FC_REFLECT( eosio::go_away_message, (reason)(node_id) )
FC_REFLECT( eosio::time_message, (org)(rec)(xmt)(dst) )
FC_REFLECT( eosio::notice_message, (known_trx)(known_blocks) )
FC_REFLECT( eosio::request_message, (req_trx)(req_blocks) )
FC_REFLECT( eosio::sync_request_message, (start_block)(end_block) )

/**
 *
Goals of Network Code
1. low latency to minimize missed blocks and potentially reduce block interval
2. minimize redundant data between blocks and transactions.
3. enable rapid sync of a new node
4. update to new boost / fc



State:
   All nodes know which blocks and transactions they have
   All nodes know which blocks and transactions their peers have
   A node knows which blocks and transactions it has requested
   All nodes know when they learned of a transaction

   send hello message
   write loop (true)
      if peer knows the last irreversible block {
         if peer does not know you know a block or transactions
            send the ids you know (so they don't send it to you)
            yield continue
         if peer does not know about a block
            send transactions in block peer doesn't know then send block summary
            yield continue
         if peer does not know about new public endpoints that you have verified
            relay new endpoints to peer
            yield continue
         if peer does not know about transactions
            sends the oldest transactions that is not known by the remote peer
            yield continue
         wait for new validated block, transaction, or peer signal from network fiber
      } else {
         we assume peer is in sync mode in which case it is operating on a
         request / response basis

         wait for notice of sync from the read loop
      }


    read loop
      if hello message
         verify that peers Last Ir Block is in our state or disconnect, they are on fork
         verify peer network protocol

      if notice message update list of transactions known by remote peer
      if trx message then insert into global state as unvalidated
      if blk summary message then insert into global state *if* we know of all dependent transactions
         else close connection


    if my head block < the LIB of a peer and my head block age > block interval * round_size/2 then
    enter sync mode...
        divide the block numbers you need to fetch among peers and send fetch request
        if peer does not respond to request in a timely manner then make request to another peer
        ensure that there is a constant queue of requests in flight and everytime a request is filled
        send of another request.

     Once you have caught up to all peers, notify all peers of your head block so they know that you
     know the LIB and will start sending you real time transactions

parallel fetches, request in groups


only relay transactions to peers if we don't already know about it.

send a notification rather than a transaction if the txn is > 3mtu size.





*/
