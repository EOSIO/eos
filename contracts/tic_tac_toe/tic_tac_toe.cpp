/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <tic_tac_toe.hpp>

using namespace eosio;
namespace tic_tac_toe {
  /**
   * @brief Check if cell is empty
   * @param cell - value of the cell (should be either 0, 1, or 2)
   * @return true if cell is empty
   */
  bool is_empty_cell(const uint8_t& cell) {
    return cell == 0;
  }

  /**
   * @brief Check for valid movement
   * @detail Movement is considered valid if it is inside the board and done on empty cell
   * @param movement - the movement made by the player
   * @param game - the game on which the movement is being made
   * @return true if movement is valid
   */
  bool is_valid_movement(const movement& mvt, const game& game_for_movement) {
    uint32_t movement_location = mvt.row * 3 + mvt.column;
    bool is_valid = movement_location < game_for_movement.board_len && is_empty_cell(game_for_movement.board[movement_location]);
    return is_valid;
  }


  /**
   * @brief Get winner of the game
   * @detail Winner of the game is the first player who made three consecutive aligned movement
   * @param game - the game which we want to determine the winner of
   * @return winner of the game (can be either none/ draw/ account name of host/ account name of challenger)
   */
  account_name get_winner(const game& current_game) {
    if((current_game.board[0] == current_game.board[4] && current_game.board[4] == current_game.board[8]) ||
       (current_game.board[1] == current_game.board[4] && current_game.board[4] == current_game.board[7]) ||
       (current_game.board[2] == current_game.board[4] && current_game.board[4] == current_game.board[6]) ||
       (current_game.board[3] == current_game.board[4] && current_game.board[4] == current_game.board[5])) {
      //  - | - | x    x | - | -    - | - | -    - | x | -
      //  - | x | -    - | x | -    x | x | x    - | x | -
      //  x | - | -    - | - | x    - | - | -    - | x | -
      if (current_game.board[4] == 1) {
        return current_game.host;
      } else if (current_game.board[4] == 2) {
        return current_game.challenger;
      }
    } else if ((current_game.board[0] == current_game.board[1] && current_game.board[1] == current_game.board[2]) ||
               (current_game.board[0] == current_game.board[3] && current_game.board[3] == current_game.board[6])) {
      //  x | x | x       x | - | -
      //  - | - | -       x | - | -
      //  - | - | -       x | - | -
      if (current_game.board[0] == 1) {
        return current_game.host;
      } else if (current_game.board[0] == 2) {
        return current_game.challenger;
      }
    } else if ((current_game.board[2] == current_game.board[5] && current_game.board[5] == current_game.board[8]) ||
               (current_game.board[6] == current_game.board[7] && current_game.board[7] == current_game.board[8])) {
      //  - | - | -       - | - | x
      //  - | - | -       - | - | x
      //  x | x | x       - | - | x
      if (current_game.board[8] == 1) {
        return current_game.host;
      } else if (current_game.board[8] == 2) {
        return current_game.challenger;
      }
    } else {
      bool is_board_full = true;
      for (uint8_t i = 0; i < current_game.board_len; i++) {
        if (is_empty_cell(current_game.board[i])) {
          is_board_full = false;
          break;
        }
      }
      if (is_board_full) {
        return N(draw);
      }
    }
    return N(none);
  }

  /**
   * @brief Apply create action
   * @param create - action to be applied
   */
  void apply_create(const create& c) {
    require_auth(c.host);
    eosio_assert(c.challenger != c.host, "challenger shouldn't be the same as host");

    // Check if game already exists
    game existing_game;
    bool game_does_not_exist = Games::get(c.challenger, existing_game, c.host);
    eosio_assert(game_does_not_exist == false, "game already exists");

    game game_to_create(c.challenger, c.host);
    Games::store(game_to_create, c.host);
  }

  /**
   * @brief Apply restart action
   * @param restart - action to be applied
   */
  void apply_restart(const restart& r) {
    require_auth(r.by);

    // Check if game exists
    game game_to_restart;
    bool game_does_not_exist = Games::get(r.challenger, game_to_restart, r.host);
    eosio_assert(game_does_not_exist, "game doesn't exist!");

    // Check if this game belongs to the action sender
    eosio_assert(r.by == game_to_restart.host || r.by == game_to_restart.challenger, "this is not your game!");

    // Reset game
    game_to_restart.reset_game();

    Games::update(game_to_restart, game_to_restart.host);
  }

  /**
   * @brief Apply close action
   * @param close - action to be applied
   */
  void apply_close(const close& c) {
    require_auth(c.host);

    // Check if game exists
    game game_to_close;
    bool game_does_not_exist = Games::get(c.challenger, game_to_close, c.host);
    eosio_assert(game_does_not_exist, "game doesn't exist!");

    Games::remove(game_to_close, game_to_close.host);
  }

  /**
   * @brief Apply move action
   * @param move - action to be applied
   */
  void apply_move(const move& m) {
    require_auth(m.by);

    // Check if game exists
    game game_to_move;
    bool game_does_not_exist = Games::get(m.challenger, game_to_move, m.host);
    eosio_assert(game_does_not_exist, "game doesn't exist!");

    // Check if this game hasn't ended yet
    eosio_assert(game_to_move.winner == N(none), "the game has ended!");
    // Check if this game belongs to the action sender
    eosio_assert(m.by == game_to_move.host || m.by == game_to_move.challenger, "this is not your game!");
    // Check if this is the  action sender's turn
    eosio_assert(m.by == game_to_move.turn, "it's not your turn yet!");


    // Check if user makes a valid movement
    eosio_assert(is_valid_movement(m.mvt, game_to_move), "not a valid movement!");

    // Fill the cell, 1 for host, 2 for challenger
    bool is_movement_by_host = m.by == game_to_move.host;
    if (is_movement_by_host) {
      game_to_move.board[m.mvt.row * 3 + m.mvt.column] = 1;
      game_to_move.turn = game_to_move.challenger;
    } else {
      game_to_move.board[m.mvt.row * 3 + m.mvt.column] = 2;
      game_to_move.turn = game_to_move.host;
    }
    // Update winner
    game_to_move.winner = get_winner(game_to_move);
    Games::update(game_to_move, game_to_move.host);
  }


}

/**
*  The init() and apply() methods must have C calling convention so that the blockchain can lookup and
*  call these methods.
*/
extern "C" {

  /// The apply method implements the dispatch of events to this contract
  void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
    if (code == N(tic.tac.toe)) {
      if (action == N(create)) {
        tic_tac_toe::apply_create(current_action<tic_tac_toe::create>());
      } else if (action == N(restart)) {
        tic_tac_toe::apply_restart(current_action<tic_tac_toe::restart>());
      } else if (action == N(close)) {
        tic_tac_toe::apply_close(current_action<tic_tac_toe::close>());
      } else if (action == N(move)) {
        tic_tac_toe::apply_move(current_action<tic_tac_toe::move>());
      }
    }
  }

} // extern "C"
