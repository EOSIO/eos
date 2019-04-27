#include "actiondemo.hpp"
#include <eosiolib/print.hpp>
#include <eosiolib/types.h>
#include <eosiolib/transaction.h>

namespace spaceaction {

    void actiondemo::apply( name code, name act ) {
        if( code != _self )
            return;
 
        switch( act ) {
            case "generate"_n:
                generate(unpack_action_data<args>());
                return;
            case "inlineact"_n:
                inlineact(unpack_action_data<argsinline>());
            case "clear"_n:
                clear();
                return;
            case "hascontract"_n:
                hascontract(unpack_action_data<args_name>());
                return;
        }
    }

    void actiondemo::clear(){
        //require_auth(_self);
        seedobjs table{_self, _self.value};
        auto iter = table.begin();
        while (iter != table.end())
        {
            table.erase(iter);
            iter = table.begin();
        }
    }

    std::string to_hex( const char* d, uint32_t s )
    {
        std::string r;
        const char* to_hex="0123456789abcdef";
        uint8_t* c = (uint8_t*)d;
        for( uint32_t i = 0; i < s; ++i )
            (r += to_hex[(c[i]>>4)]) += to_hex[(c[i] &0x0f)];
        return r;
    }

    void actiondemo::hascontract(const args_name& t){
        bool r = has_contract(t.name);
        print_f("% has_contract:%", name{t.name}.to_string(),r);

//        if (r) {
            checksum256 code;
            get_contract_code(t.name, &code);

            std::string s = to_hex((char*)&code, 32);
            print_f("% contract_code:%", name{t.name}.to_string(),s);
//        }

    }

    void actiondemo::generate(const args& t){
        // for (int i = 0; i < 1; ++i)
        // {
            checksum256 txid;
            get_transaction_id(&txid);
            std::string tx = to_hex((char*)&txid, 32);

            uint64_t seq = 0;
            get_action_sequence(&seq);


            size_t szBuff = sizeof(signature);
            char buf[szBuff];
            memset(buf,0,szBuff);
            size_t size = bpsig_action_time_seed(buf, sizeof(buf));
            eosio_assert(size > 0 && size <= sizeof(buf), "buffer is too small");
            std::string seedstr = to_hex(buf,size);


            seedobjs table(_self, _self.value);
            uint64_t count = 0;
            for (auto itr = table.begin(); itr != table.end(); ++itr) {
                ++count;
            }

            auto r = table.emplace(_self, [&](auto &a) {
                a.id = count + 1;
                a.create = eosio::time_point_sec(now());
                a.seedstr = seedstr;
                a.txid = tx;
                a.action = seq;
            });
            print_f("self:%, loop:%, count:%, seedstr:%", name{_self}.to_string(), t.loop, count, r->seedstr);
        // }
    }

    void actiondemo::inlineact(const argsinline& t){
        auto& payer = t.payer;
        args gen;
        gen.loop = 1;
        gen.num = 1;

        generate(gen);

        if(t.in != ""_n)
        {
            INLINE_ACTION_SENDER(spaceaction::actiondemo, generate)( t.in, {payer,"active"_n},
                                                               { gen});
            INLINE_ACTION_SENDER(spaceaction::actiondemo, generate)( t.in, {payer,"active"_n},
                                                                     { gen});
        }

    }
}

// extern "C" {
// [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
//     spaceaction::actiondemo obj(receiver);
//     obj.apply(code, action);
//     eosio_exit(0);
// }
// }

#define EOSIO_DISPATCH_CUSTOM(TYPE, MEMBERS)                                           \
    extern "C"                                                                         \
    {                                                                                  \
        void apply(uint64_t receiver, uint64_t code, uint64_t action)                  \
        {                                                                              \
                                                                       \
                switch (action)                                                        \
                {                                                                      \
                    EOSIO_DISPATCH_HELPER(TYPE, MEMBERS)                               \
                }                                                                      \
                /* does not allow destructor of thiscontract to run: eosio_exit(0); */ \
                                                                                  \
        }                                                                              \
    }

EOSIO_DISPATCH_CUSTOM(spaceaction::actiondemo, (apply)(generate)(clear)(hascontract)(inlineact))
