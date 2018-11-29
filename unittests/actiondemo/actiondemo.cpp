#include "actiondemo.hpp"
#include "../../contracts/eosiolib/print.hpp"
#include "../../contracts/eosiolib/types.hpp"
#include "../../contracts/eosiolib/transaction.hpp"

namespace spaceaction {

    void actiondemo::apply( account_name code, account_name act ) {

        if( code != _self )
            return;

        switch( act ) {
            case N(generate):
                generate(unpack_action_data<args>());
                return;
            case N(inlineact):
                inlineact(unpack_action_data<args_inline>());
            case N(clear):
                clear();
                return;
        }
    }

    void actiondemo::clear(){
        //require_auth(_self);
        seedobjs table(_self, _self);
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

    void actiondemo::generate(const args& t){
        for (int i = 0; i < t.loop; ++i) {
            transaction_id_type txid;
            get_transaction_id(&txid);
            std::string tx = to_hex((char*)&txid.hash, 32);

            uint64_t seq = 0;
            get_action_sequence(&seq);


            size_t szBuff = sizeof(signature);
            char buf[szBuff];
            memset(buf,0,szBuff);
            size_t size = bpsig_action_time_seed(buf, sizeof(buf));
            eosio_assert(size > 0 && size <= sizeof(buf), "buffer is too small");
            std::string seedstr = to_hex(buf,size);


            seedobjs table(_self, _self);
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
        }
    }

    void actiondemo::inlineact(const args_inline& t){
        auto& payer = t.payer;
        args gen;
        gen.loop = 1;
        gen.num = 1;

        generate(gen);

        if(t.in != 0)
        {
            INLINE_ACTION_SENDER(spaceaction::actiondemo, generate)( t.in, {payer,N(active)},
                                                               { gen});
            INLINE_ACTION_SENDER(spaceaction::actiondemo, generate)( t.in, {payer,N(active)},
                                                                     { gen});
        }

    }
}

extern "C" {
[[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    spaceaction::actiondemo obj(receiver);
    obj.apply(code, action);
    eosio_exit(0);
}
}