/*
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <rps.hpp>
#include <eoslib/crypto.h>
#include <eoslib/memory.hpp>
#include <eoslib/transaction.hpp>
#include <rps_eos_ext.hpp>

using namespace eosio;

namespace rps {
    /*
     * Store game to database
     */
    int32_t store_game(const game& g) {
        bytes b = value_to_bytes(g);
        return store_i64(contract_name, N(games), b.data, b.len);
    }

    /*
     * Update game object in database
     */
     int32_t update_game(const game& g) {
        bytes b = value_to_bytes(g);
        return update_i64(N(rps), N(games), b.data, b.len);
    }
    /*
     * Load game from database
     */
    int32_t load_game(game& g, const uint64_t game_id) {
        uint32_t max_buffer_size = 1000;
        uint8_t buffer[max_buffer_size];
        memcpy(buffer, &game_id, sizeof(uint64_t));
        int32_t num_b_read = load_i64(contract_name, contract_name, N(games), buffer, max_buffer_size);

        if (num_b_read != -1) {
            bytes b;
            b.len = (uint32_t)num_b_read;
            b.data = buffer;
            g = bytes_to_value<game>(b);
        } 
        return num_b_read;
    }

    /*
     * Get next game id
     */
    uint64_t get_next_game_id() {
        uint32_t max_buffer_size = sizeof(uint64_t);
        uint8_t buffer[max_buffer_size];
        int32_t num_bytes_read = back_i64(contract_name, contract_name, N(games), buffer, max_buffer_size);

        if (num_bytes_read != -1) {
          datastream<char *> ds((char *)buffer, max_buffer_size);
          uint64_t prev_game_id;
          raw::unpack(ds, prev_game_id);
          return prev_game_id + 1;
        } 
        return 0;
    }

    /*
     * Get winner of the game
     */
    account_name get_game_winner(const game& g) {
        // Calculate score
        uint8_t host_score = 0;
        uint8_t foe_score = 0;
        for (int i = 0; i < g.round; i++) {
            if (g.round_winner[i] == g.host) {
                host_score++;
            } else if (g.round_winner[i] == g.foe) {
                foe_score++;
            }
        }
        // Determine winner
        if (host_score >= 2 ) {
            return g.host;
        } else if (foe_score >= 2) {
            return g.foe;
        } else if (g.round > 2 && host_score == foe_score) {
            return N(draw);
        } else {
            return N(none);
        }

    }

    /*
     * Get winner of a round
     */
    account_name get_round_winner(const game& g) {
        const string& host_move = g.host_moves.moves_val[g.round];
        const string& foe_move = g.foe_moves.moves_val[g.round];
        if ((host_move == "rock" && foe_move == "scissor") || 
            (host_move == "scissor" && foe_move == "paper")  || 
            (host_move == "paper" && foe_move == "rock")) {
            return g.host; 
        } else if ((host_move == "rock" && foe_move == "paper") || 
                    (host_move == "scissor" && foe_move == "rock")  || 
                    (host_move == "paper" && foe_move == "scissor")) {
            return g.foe;
        } else {
            return N(draw);
        }
    }

    /*
     * Distribute stake to the winner
     */
    void distribute_stake(game& g) {
        // Fetch host and foe balance
        balance host_bal;
        Balance::get(host_bal, g.host);
        balance foe_bal;
        Balance::get(foe_bal, g.foe);
        if (g.game_winner == N(draw) || g.game_winner == N(none)) {
            // Draw, returns money back
            host_bal.locked_amount -= g.host_stake;
            host_bal.avail_amount += g.host_stake;
            foe_bal.locked_amount -= g.foe_stake;
            foe_bal.avail_amount += g.foe_stake;
        } else if (g.game_winner == g.host) {
            // Host win, gives all money to host
            host_bal.locked_amount -= g.host_stake;
            foe_bal.locked_amount -= g.foe_stake;
            host_bal.avail_amount += (g.host_stake + g.foe_stake);
        } else if (g.game_winner == g.foe) {
            // Host win, gives all money to foe
            host_bal.locked_amount -= g.host_stake;
            foe_bal.locked_amount -= g.foe_stake;
            foe_bal.avail_amount += (g.host_stake + g.foe_stake);
        }
        // Update foe and host balance
        Balance::update(host_bal, g.host);
        Balance::update(foe_bal, g.foe);
        // Reduce stakes in game
        g.host_stake = 0;
        g.foe_stake = 0;
    }

    /*
     * Apply create game action
     */
    void apply_create(const create& c) {
        require_auth(c.host);
        // Create a new game and set it to active
        game g(get_next_game_id(), c.foe, c.host);
        g.is_active = 1;
        store_game(g);
    }

    /*
     * Apply submit move action
     */
    void apply_submit(const submit& s) {
        require_auth(s.by);

        // Check if the game exists and is active
        game g;
        bool game_exists = load_game(g, s.game_id) != -1;
        assert(game_exists == true, "game doesn't exist!");
        assert(g.is_active == 1, "game is inactive!");

        // Check if this g belongs to the message sender
        assert(s.by == g.host || s.by == g.foe, "this is not your game!");

        bool is_by_host = s.by == g.host;
        moves* moves_pointer = is_by_host ? &g.host_moves : &g.foe_moves;

        assert(moves_pointer->submit_turn == g.round, "you have submitted a move!");

        // Assign the move and nonce
        moves_pointer->hashed_moves[g.round] = s.hashed_move;
        moves_pointer->submit_turn++;

        // Update the data
        update_game(g);
    }   

    /*
     * Apply reveal move action
     */
    void apply_reveal(const reveal& r) {
        require_auth(r.by);

        // Check if the game exists and is active
        game g;
        bool game_exists = load_game(g, r.game_id) != -1;
        assert(game_exists == true, "game doesn't exist!");
        assert(g.is_active == 1, "game is inactive!");

        // Check if this game belongs to the message sender
        assert(r.by == g.host || r.by == g.foe, "this is not your game!");
        // Check for valid move
        assert(r.move_val == "rock" || r.move_val == "paper" || r.move_val == "scissor", "invalid move!");

        bool is_by_host = r.by == g.host;
        moves* moves_pointer = is_by_host ? &g.host_moves : &g.foe_moves;
        assert(moves_pointer->submit_turn > moves_pointer->reveal_turn, "you haven't submitted a move!");
        assert(moves_pointer->reveal_turn == g.round, "you have revealed the move!");

        // reveal the move
        string move_and_nonce = r.move_val + r.nonce;
        // Ensure it is the right move and nonce
        assert_sha256((char *)move_and_nonce.get_data(), move_and_nonce.get_size(), &moves_pointer->hashed_moves[g.round]);
        // Update the information in the game object
        moves_pointer->moves_val[g.round] = r.move_val; 
        moves_pointer->nonces[g.round] = r.nonce; 
        moves_pointer->reveal_turn++;

        // If both player has revealed, decide the winner of this round
        if (g.host_moves.reveal_turn > g.round && g.foe_moves.reveal_turn > g.round){
            g.round_winner[g.round] = get_round_winner(g);
            // Increment the round
            g.round++;
        }

        // Update game winner
        g.game_winner = get_game_winner(g);

        bool is_winner_determined = g.game_winner != N(none);
        if (is_winner_determined) {
            // Distribute stake if the winner is determined
            distribute_stake(g);
            // Set game to be inactive
            g.is_active = 0;
        }

        // Update the game
        update_game(g);
    }

    /*
     * Apply stake action
     */
    void apply_stake(const stake& s) { 
        // Check authorization
        require_auth(s.by);

        // Check if the game exists and is active
        game g;
        bool game_exists = load_game(g, s.game_id) != -1;
        assert(game_exists == true, "game doesn't exist!");
        assert(g.is_active == 1, "game is inactive!");

        // Check if this game belongs to the message sender
        assert(s.by == g.host || s.by == g.foe, "this is not your game!");

        // Check if the account has enough balance
        balance bal;
        bool balance_exists = Balance::get(bal, s.by);
        assert(balance_exists && bal.avail_amount >= s.amount, "not enough balance! deposit some money first!");
        bal.avail_amount -= s.amount;
        bal.locked_amount += s.amount;
        // Update balance
        Balance::update(bal, s.by);

        // Update game table for host/ foe deposit
        bool is_by_host = s.by == g.host;
        if (is_by_host) {
            g.host_stake += s.amount;
        } else {
            g.foe_stake += s.amount;
        }
        update_game(g);

    }

    /*
     * Message handler for native eos transfer
     * - If it is incoming transfer, add the balance for the sender
     * - If it is outgoing transfer, deduct the balance from the receiver
     */
    void apply_eos_transfer(const transfer& t) {
        if (t.from == contract_name) {
            // Prevent transfer to account that doesn't have enough available money deposited to rps
            balance bal;
            bool balance_exists = Balance::get(bal, t.to);
            assert(balance_exists && bal.avail_amount >= t.amount, "not enough balance to withdraw!");

            // Update balance table
            bal.avail_amount -= t.amount;
            Balance::update(bal, t.to);
        } else if (t.to == contract_name) {
            // When another account deposits money to this contract, note the amount of money he sends
            balance bal;
            bool balance_exists =  Balance::get(bal, t.from);
            bal.avail_amount += t.amount;

            // Update balance if it exists otherwise create new one
            if (balance_exists) {
                Balance::update(bal, t.from);
            } else {
                Balance::store(bal, t.from);
            }
        }
       
    }

    /*
     * Apply Withdraw Action
     * This generates a native eosio transfer with auth from the code
     */
    void apply_withdraw(const withdraw& w) {
        require_auth(w.by);
        // Initiate eos transfer from this contract to the withdrawer
        transfer tx;
        tx.from = contract_name;
        tx.to = w.by;
        tx.amount = w.amount;
        message msg(N(eos), N(transfer), tx);
        msg.add_permissions(contract_name, N(code));
        msg.send();
    }
    
    /*
     * Apply Cancel Game Action
     * Only enable cancel game after the game has been running for at least 24 hours
     */
    void apply_cancel(const cancel& c) {
        require_auth(c.by);

        // Check if game exists
        game g;
        bool game_exists = load_game(g, c.game_id) != -1;
        assert(game_exists == true, "game doesn't exist!");
        assert(g.is_active == 1, "game is inactive!");

        // Check if this g belongs to the message sender
        assert(c.by == g.host || c.by == g.foe, "this is not your game!");
        assert(now() - g.created_time > 24 * 60 * 60, "you can't cancel game that hasn't been active for at least a day");

        // Release stake
        distribute_stake(g);
        // Set game to be inactive
        g.is_active = 0;

        update_game(g);
    }
}


/*
 *  The init() and apply() methods must have C calling convention so that the blockchain can lookup and
 *  call these methods.
 */
extern "C" {

    /*
     * This method is called once when the contract is published or updated.
     */
    void init()  {
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action_name ) {
        if (code == rps::contract_name) {
            if (action_name == N(create)) {
                rps::apply_create(current_message<rps::create>());
            } else if (action_name == N(submit)) {
                rps::apply_submit(current_message<rps::submit>());
            } else if (action_name == N(reveal)) {
                rps::apply_reveal(current_message<rps::reveal>());
            } else if (action_name == N(cancel)) {
                rps::apply_cancel(current_message<rps::cancel>());
            } else if (action_name == N(stake)) {
                rps::apply_stake(current_message<rps::stake>());
            } else if (action_name == N(withdraw)) {
                rps::apply_withdraw(current_message<rps::withdraw>());
            }
        } else if (code == N(eos)) {
            if (action_name == N(transfer)) {
                rps::apply_eos_transfer(current_message<eosio::transfer>());
            }
        }
      
    }

} // extern "C"
