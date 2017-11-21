/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <tic_tac_toe.hpp>

namespace tic_tac_toe {
  using namespace eosio;
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
  bool is_valid_movement(const Movement& movement, const game& game_for_movement) {
    uint32_t movement_location = movement.row * 3 + movement.column;
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
  void apply_create(const Create& create) {
    require_auth(create.host);
    assert(create.challenger != create.host, "challenger shouldn't be the same as host");

    // Check if game already exists
    game existing_game;
    bool game_exists = Games::get(create.challenger, existing_game, create.host);
    assert(game_exists == false, "game already exists");

    game game_to_create(create.challenger, create.host);
    Games::store(game_to_create, create.host);
  }

  /**
   * @brief Apply restart action
   * @param restart - action to be applied
   */
  void apply_restart(const Restart& restart) {
    require_auth(restart.by);

    // Check if game exists
    game game_to_restart;
    bool game_exists = Games::get(restart.challenger, game_to_restart, restart.host);
    assert(game_exists == true, "game doesn't exist!");

    // Check if this game belongs to the action sender
    assert(restart.by == game_to_restart.host || restart.by == game_to_restart.challenger, "this is not your game!");

    // Reset game
    game_to_restart.reset_game();

    Games::update(game_to_restart, game_to_restart.host);
  }

  /**
   * @brief Apply close action
   * @param close - action to be applied
   */
  void apply_close(const Close& close) {
    require_auth(close.host);

    // Check if game exists
    game game_to_close;
    bool game_exists = Games::get(close.challenger, game_to_close, close.host);
    assert(game_exists == true, "game doesn't exist!");

    Games::remove(game_to_close, game_to_close.host);
  }

  /**
   * @brief Apply move action
   * @param move - action to be applied
   */
  void apply_move(const Move& move) {
    require_auth(move.by);

    // Check if game exists
    game game_to_move;
    bool game_exists = Games::get(move.challenger, game_to_move, move.host);
    assert(game_exists == true, "game doesn't exist!");

    // Check if this game hasn't ended yet
    assert(game_to_move.winner == N(none), "the game has ended!");
    // Check if this game belongs to the action sender
    assert(move.by == game_to_move.host || move.by == game_to_move.challenger, "this is not your game!");
    // Check if this is the  action sender's turn
    assert(move.by == game_to_move.turn, "it's not your turn yet!");


    // Check if user makes a valid movement
    assert(is_valid_movement(move.movement, game_to_move), "not a valid movement!");

    // Fill the cell, 1 for host, 2 for challenger
    bool is_movement_by_host = move.by == game_to_move.host;
    if (is_movement_by_host) {
      game_to_move.board[move.movement.row * 3 + move.movement.column] = 1;
      game_to_move.turn = game_to_move.challenger;
    } else {
      game_to_move.board[move.movement.row * 3 + move.movement.column] = 2;
      game_to_move.turn = game_to_move.host;
    }
    // Update winner
    game_to_move.winner = get_winner(game_to_move);
    Games::update(game_to_move, game_to_move.host);
  }


}

using namespace tic_tac_toe;

/**
*  The init() and apply() methods must have C calling convention so that the blockchain can lookup and
*  call these methods.
*/
extern "C" {

  /**
  *  This method is called once when the contract is published or updated.
  */
  void init()  {
  }

  /// The apply method implements the dispatch of events to this contract
  void apply( uint64_t code, uint64_t action ) {
    if (code == N(tic.tac.toe)) {
      if (action == N(create)) {
        tic_tac_toe::apply_create(current_action<tic_tac_toe::Create>());
      } else if (action == N(restart)) {
        tic_tac_toe::apply_restart(current_action<tic_tac_toe::Restart>());
      } else if (action == N(close)) {
        tic_tac_toe::apply_close(current_action<tic_tac_toe::Close>());
      } else if (action == N(move)) {
        tic_tac_toe::apply_move(current_action<tic_tac_toe::Move>());
      }
    }
  }

} // extern "C"
