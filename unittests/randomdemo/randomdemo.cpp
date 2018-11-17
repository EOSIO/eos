#include "randomdemo.hpp"
#include "../../contracts/eosiolib/random.hpp"
#include "../../contracts/eosiolib/print.hpp"
#include "../../contracts/eosiolib/random.h"
#include "../../contracts/eosiolib/types.hpp"

namespace spacerandom {

    void randomdemo::apply( account_name code, account_name act ) {

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

    void randomdemo::clear(){
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

    void randomdemo::generate(const args& t){
        for (int i = 0; i < t.loop; ++i) {

            std::unique_ptr<std::seed_seq> seed = seed_timestamp_txid_signed();

            std::mt19937 e(*seed);
            std::uniform_int_distribution <uint32_t> normal_dist(100, 2);

            std::vector <uint32_t> list;
            for (int n = 0; n < t.num; ++n) {
                list.push_back(std::round(normal_dist(e)));
            }

            //seed
            vector <uint32_t> seed_vec(seed->size());
            seed->param(seed_vec.begin());
            std::string seedstr = to_hex(reinterpret_cast<char*>(seed_vec.data()), seed->size()* sizeof(uint32_t));

            seedobjs table(_self, _self);


            uint64_t count = 0;
            for (auto itr = table.begin(); itr != table.end(); ++itr) {
                ++count;
            }

            auto r = table.emplace(_self, [&](auto &a) {
                a.id = count + 1;
                a.create = eosio::time_point_sec(now());
                a.seedstr = seedstr;
                a.randoms = list;
            });
            print_f("self:%, loop:%, count:%, seedstr:%", name{_self}.to_string(), t.loop, count, r->seedstr);
        }
    }

    void randomdemo::inlineact(const args_inline& t){
        auto& payer = t.payer;
        args gen;
        gen.loop = 1;
        gen.num = 1;

        generate(gen);

        if(t.in != 0)
        {
            INLINE_ACTION_SENDER(spacerandom::randomdemo, generate)( t.in, {payer,N(active)},
                                                               { gen});
            INLINE_ACTION_SENDER(spacerandom::randomdemo, generate)( t.in, {payer,N(active)},
                                                                     { gen});
        }

    }
}

extern "C" {
[[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
    spacerandom::randomdemo obj(receiver);
    obj.apply(code, action);
    eosio_exit(0);
}
}