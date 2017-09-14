#pragma once
#include <eos/chain/block.hpp>
#include <eos/chain/types.hpp>

namespace eos {
   using namespace chain;
   using namespace fc;

  struct handshake_message {
      int16_t         network_version = 0;
      chain_id_type   chain_id; ///< used to identify chain
      fc::sha256      node_id; ///< used to identify peers and prevent self-connect
      string          p2p_address;
      uint32_t        last_irreversible_block_num = 0;
      block_id_type   last_irreversible_block_id;
      uint32_t        head_num = 0;
      block_id_type   head_id;
      string          os;
      string          agent;
   };

   struct notice_message {
      vector<transaction_id_type> known_trx;
      vector<block_id_type>       known_blocks;
   };


   struct request_message {
      vector<transaction_id_type> req_trx;
      vector<block_id_type>       req_blocks;
   };

   struct block_summary_message {
      signed_block                block;
      vector<transaction_id_type> trx_ids;
   };

   struct sync_request_message {
      uint32_t start_block;
      uint32_t end_block;
   };

   using net_message = static_variant<handshake_message,
                                      notice_message,
                                      request_message,
                                      sync_request_message,
                                      block_summary_message,
                                      SignedTransaction,
                                      signed_block>;

} // namespace eos


FC_REFLECT( eos::handshake_message,
            (network_version)(chain_id)(node_id)
            (p2p_address)
            (last_irreversible_block_num)(last_irreversible_block_id)
            (head_num)(head_id)
            (os)(agent) )

FC_REFLECT( eos::block_summary_message, (block)(trx_ids) )
FC_REFLECT( eos::notice_message, (known_trx)(known_blocks) )
FC_REFLECT( eos::request_message, (req_trx)(req_blocks) )
FC_REFLECT( eos::sync_request_message, (start_block)(end_block) )

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
     know the LIB and will start sending you real time tranasctions

parallel fetches, request in groups


only relay transactions to peers if we don't already know about it.

send a notification rather than a transaaction if the txn is > 3mtu size.





*/
