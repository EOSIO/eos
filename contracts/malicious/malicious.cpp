/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <malicious/malicious.hpp>
#include <eoslib/memory.hpp>
#include <eoslib/account.hpp>
#include <eoslib/eos.hpp>
#include <eoslib/system.h>

#define EOS_SYMBOL  (int64_t(4) | (uint64_t('E') << 8) | (uint64_t('O') << 16) | (uint64_t('S') << 24))

namespace TOKEN_NAME {
    using namespace eosio;

    ///  When storing accounts, check for empty balance and remove account
    void store_account(account_name account_to_store, const account &a) {

    }

    // Create memory leak , try to allocate huge chunk of memory
    void apply_malicious_resource() {
        // allocate memory in an infinite loop
        char *buffer = (char *) eosio::malloc(0xFFFFFFFF);

    }

    // Infinite loop
    void apply_malicious_infinite() {
        while (1) {
            char *buffer = (char *) eosio::malloc(1064);
        }
    }

    // Divide by zero
    void apply_malicious_math() {
        uint32_t a = 1;
        uint32_t b = 0;
        uint32_t c = a / b;
        eosio::print(c); // c is 0
        int32_t arr[256];
    }

    // Fetch account details by using account API
    void apply_malicious_account()
    {
        eosio::print("Fetching account balance for inita\n");
        eosio::account::account_balance b;

        b.account = N(inita);
        eosio::account::get(b);
        eosio::print("\n\tAccount=", b.account);
        eosio::print("\n\tBalance amount=", (uint64_t) b.eos_balance.amount);
        eosio::print("\n\tBalance symbol=",b.eos_balance.symbol);
        eosio::print("\n\tStaked balance=", (uint64_t) b.staked_balance.amount);
        eosio::print("\n\tStaked balance symbol=", b.staked_balance.symbol);
        eosio::print("\n\tUnstaking balance amount=", (uint64_t) b.unstaking_balance.amount);
        eosio::print("\n\tUnstaking balance symbol=", b.unstaking_balance.symbol);
        eosio::print("\n\tLast unstaking time=", b.last_unstaking_time);
    }

    // Infinite messaging between Malicious and Dummy contract
    void apply_malicious_notify(const TOKEN_NAME::transfer &transfer_msg) {
        eosio::print("malicious::apply_malicious_notiofy\tfrom : ");
        eosio::print(name(transfer_msg.from));
        eosio::print("\tto : ");
        eosio::print(name(transfer_msg.to));
        eosio::print("\n");
        require_notice(transfer_msg.to);
    }  // namespace TOKEN_NAME

    using namespace TOKEN_NAME;

    extern "C"
    {
    void init() {
        store_account(N(malicious), account(malicious_tokens(1000ll * 1000ll * 1000ll)));
    }

    /// The apply method implements the dispatch of events to this contract
    void apply(uint64_t code, uint64_t action) {
        if (code == N(malicious)) {
            eosio::print("From malicious::apply(code=malicious)\n");
            // eosc push message malicious resource '{"from":"malicious","to":"inita","amount":50}' --scope malicious,inita
            if (action == N(resource))
                TOKEN_NAME::apply_malicious_resource();
            // eosc push message malicious infinite '{"from":"malicious","to":"inita","amount":50}' --scope malicious,inita
            else if (action == N(infinite))
                TOKEN_NAME::apply_malicious_infinite();
            // eosc push message malicious math '{}' --scope malicious,inita
            else if (action == N(math))
                TOKEN_NAME::apply_malicious_math();
            // eosc push message malicious notify  '{"from":"malicious","to":"dummy", "amount":50}' --scope malicious,dummy
            else if (action == N(notify))
                TOKEN_NAME::apply_malicious_notify(current_message<TOKEN_NAME::transfer>());
            // eosc push message malicious account '{}' --scope malicious,inita
            else if (action == N(account))
                TOKEN_NAME::apply_malicious_account();
        }
        else if (code == N(dummy)) {
            eosio::print("From malicious::apply(code=dummy)\n");
           if (action == N(notify))
                TOKEN_NAME::apply_malicious_notify(current_message<TOKEN_NAME::transfer>());
        }
    }
    }
}
