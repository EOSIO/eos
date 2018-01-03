/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 * 
 * Example of Player vs Player Rock Paper Scissors Smart Contract
 * This example contract demonstrates:
 * 1. How two players can submit a move without being known by other player
 * 2. How to stake money on a game and rewards the winner with the total stake
 * 
 * Rules:
 * - 1 game has 3 round
 * - The first player who gets 2 win out of 3 round is the winner
 * 
 * In order for first player to be able to submit a move into a public blockchain
 * without being known by other player, and when the move is revealed the other player is unable 
 * to change his move, the game will follow the following flows:
 * 1. Player 1 submits hash of his move + nonce (i.e. hash(rock213890123892))
 * 2. At this point, the public information is just the hash of Player 1 move and not the move itself
 * 3. Player 2 submits hash of his move + nonce (i.e. hash(scissor123947128193))
 * 4. Player 1 reveals his move by submitting his move and nonce to the blockchain
 * 5. The blockchain hashes the submitted move and nonce, if it matches with the previously submitted hash(move + nonce)
 * then the submitted move is valid.
 * 6. At this point, the move of Player 1 is known, however Player 2 won't be able to change his move
 * because if he did, the hash of the new move and nonce will not match the previously submitted hash(move + nonce)
 * 7. Player 2 reveals his move and nonce, and similarly the blockchain will compare it with the previously submitted hash(move + nonce)
 * 8. Both player has revealed his move, the winner can be determined
 * 
 * In order for the player to be able to put stake on the game, the following flows is followed:
 * 1. Player do eosio native transfer to rps contract
 * 2. rps contract listens to any eosio native transfer sent to it, it will mark down how much money is deposited
 * 3. When a game is created, player can put stake up to the amount of money it has deposited to rps contract,
 * this money will be locked until the game winner is declared
 * 4. When a game winner is declared, rps contract will mark that the staked money belongs to the winner and has the
 * winner deposited amount increased
 * 5. Winner can withdraw money by sending Withdraw action to rps contract. Internally, this will trigger eosio native
 * transfer from rps contract to the withdrawer
 * 
 * Available Actions:
 * - Create = Create New Game
 * - Submit = Submit hashed move + nonce
 * - Reveal = Reveal move and nonce
 * - Stake = Put stake on a game
 * - Withdraw = Withdraw deposited money
 * 
 * Available Tables:
 * - Games = List of Games
 * - Balance = Balance of deposited money for each account
 * 
 * To be improved:
 * - Create deferred transaction to progress the game if a player hasn't made any move for the next 5 mins,
 * this prevents the losing player to not submitting a move to the winning player to become the winner and claim the money
 * - Handle integer overflow and underflow (to be done when the token api is updated)
 */
#pragma once
#include <eoslib/eos.hpp>
#include <eoslib/system.h>
#include <eoslib/db.hpp>
#include <eoslib/string.hpp>

using namespace eosio;
namespace rps {
  
  const static uint64_t contract_name = N(rps);

  /*
   * Moves
   */
  struct PACKED(moves) {
    uint8_t moves_val_len = 3;
    string moves_val[3]; 

    uint8_t nonces_len = 3;
    string nonces[3]; // Any random string, the larger it is, the better

    uint8_t hashed_moves_len = 3;
    checksum hashed_moves[3]; 

    uint8_t submit_turn = 0;
    uint8_t reveal_turn = 0;

    /*
     * Calculate the size of this object to be used by pack function
     */
    const uint32_t get_pack_size() const {
      uint32_t size = 0;
      size += sizeof(moves_val_len);
      for (int i = 0; i < moves_val_len; i++) {
        size += moves_val[i].get_size() + sizeof(moves_val[i].get_size());
      }
      size += sizeof(nonces_len);
      for (int i = 0; i < nonces_len; i++) {
        size += nonces[i].get_size() + sizeof(nonces[i].get_size());
      }
      size += sizeof(hashed_moves_len);
      for (int i = 0; i < hashed_moves_len; i++) {
        size += sizeof(hashed_moves[i]);
      }
      size += sizeof(submit_turn);
      size += sizeof(reveal_turn);
      return size;
    }
  };

  /*
   * Game
   */
  struct PACKED(game) {
    game() {
    };
    game(uint64_t id, account_name foe, account_name host):id(id), foe(foe), host(host) {
      created_time = now();
      for (int i = 0; i < round_winner_len; i++) {
        round_winner[i] = N(none);
      }
    };
    uint64_t id;
    account_name foe;
    account_name host;
    moves host_moves;
    moves foe_moves;
    uint8_t round = 0; // 0, 1, 2
    uint8_t round_winner_len = 3;
    account_name round_winner[3]; // none, draw, foe account name, host account name
    account_name game_winner = N(none); // none, draw, foe account name, host account name
    uint64_t host_stake = 0;
    uint64_t foe_stake = 0;
    time created_time;
    uint8_t is_active = 0;
    
    /*
     * Calculate the size of this object to be used by pack function
     */
    const uint32_t get_pack_size() const {
      uint32_t size=0;
      size += sizeof(foe);
      size += sizeof(host);
      size += host_moves.get_pack_size();
      size += foe_moves.get_pack_size();
      size += sizeof(round);
      size += sizeof(round_winner_len);
      size += sizeof(round_winner);
      size += sizeof(game_winner);
      size += sizeof(host_stake);
      size += sizeof(foe_stake);
      size += sizeof(created_time);
      size += sizeof(is_active);
      return size;
    }

  };

  /*
   * Balance
   */
  struct PACKED(balance) {
    const uint64_t key = N(account); // constant key
    uint64_t locked_amount = 0;
    uint64_t avail_amount = 0;
  };

  /*
   * Create New Game Action
   */
  struct PACKED(create) {
    account_name host;
    account_name foe;
  };


  /*
   * Cancel Active Game Action
   */
  struct PACKED(cancel) {
    uint64_t game_id;
    account_name by;
  };


  /*
   * Submit Move Action
   */
  struct PACKED(submit) {
    uint64_t game_id;
    account_name by;
    checksum   hashed_move;
  };


  /*
   * Reveal Move Action
   */
  struct PACKED(reveal) {
    uint64_t game_id;
    account_name by;
    string move_val;
    string nonce;
  };


  /*
   * Stake Money Action
   */
  struct PACKED(stake) {
    uint64_t game_id;
    uint64_t amount;
    account_name by;
  };

  /*
   * Withdraw Money Action
   */
  struct PACKED(withdraw) {
    account_name by;
    uint64_t amount;
  };

  
  /*
   * Games Table
   * Store all games information
   */
  using Games = table<contract_name,contract_name,N(games),game,uint64_t>;
  /*
   * Balance Table
   * Store all deposited money information of an account
   */
  using Balance = table<contract_name,contract_name,N(balance),balance,uint64_t>;
}

