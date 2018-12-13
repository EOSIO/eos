#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>

namespace spaceaction {

    using namespace eosio;
    class actiondemo : public contract {
        typedef std::chrono::milliseconds duration;
    public:
        actiondemo( account_name self ):contract(self){}

        void apply( account_name contract, account_name act );

        struct args{
            uint64_t loop;
            uint64_t num;
        };
        //@abi action
        void generate(const args& t);

        //@abi action
        void clear();


        struct args_inline{
            account_name payer;
            account_name in;
        };
        //@abi action
        void inlineact(const args_inline& t);

    public:
        // @abi table seedobjs i64
        struct seedobj {
            uint64_t    id;
            time_point  create;
            std::string seedstr;
            std::string txid;
            uint64_t    action;

            uint64_t primary_key()const { return id; }
            EOSLIB_SERIALIZE(seedobj,(id)(create)(seedstr)(txid)(action))
        };
        typedef eosio::multi_index< N(seedobjs), seedobj> seedobjs;


    };

} /// namespace eosio
