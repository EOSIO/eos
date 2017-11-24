/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <dummy/dummy.hpp>
#include <eoslib/memory.hpp>



namespace TOKEN_NAME {
    using namespace eosio;

    ///  When storing accounts, check for empty balance and remove account
    void store_account(account_name account_to_store, const account &a) {

    }

    // When receive the transfer msg from malicous contract, send a message back to malicious and to itself
    void apply_dummy_notify(const TOKEN_NAME::transfer &transfer_msg) {
        require_notice(transfer_msg.from);
        require_notice(transfer_msg.to);
    }  // namespace TOKEN_NAME

    using namespace TOKEN_NAME;

    extern "C"
    {
    void init() {
        store_account(N(dummy), account(dummy_tokens(1000ll * 1000ll * 1000ll)));
    }

    /// The apply method implements the dispatch of events to this contract
    void apply(uint64_t code, uint64_t action) {

        if (code == N(malicious)) {
            eosio::print("From dummy::apply(code=malicious)\n");
            if (action == N(notify))
                TOKEN_NAME::apply_dummy_notify(current_message<TOKEN_NAME::transfer>());
        }
    }
    }
}
