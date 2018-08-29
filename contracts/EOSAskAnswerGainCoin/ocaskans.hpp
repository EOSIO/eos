/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/db.h>
#include <eosiolib/asset.hpp>
#include<eosiolib/serialize.hpp>
#include"tool.hpp"


const static uint32_t lable_not_release = 0;
const static uint32_t lable_released = 1;


struct transferfromact {
    account_name from;
    account_name to;
    eosio::asset quantity;

    EOSLIB_SERIALIZE(transferfromact, (from)(to)(quantity))
};


struct transfer
{
  account_name from;
  account_name to;
  eosio::asset        quantity;
  std::string       memo;

  EOSLIB_SERIALIZE( transfer, (from)(to)(quantity)(memo) )
};


/**
    *  @abi table
*/
struct ask{
    uint64_t       id;
    uint32_t       endtime;
    account_name   from;
    eosio::asset   quantity;
    uint32_t       releasedLable;
    uint32_t       createtime;
    uint64_t       optionanswerscnt;
    std::string    asktitle;
    std::string    optionanswers;

    uint64_t primary_key()const { return id; }

    EOSLIB_SERIALIZE( ask, (id)(endtime)(from)(quantity)(releasedLable)(createtime)(optionanswerscnt)(asktitle)(optionanswers))
};

/**
    *  @abi action ask
*/
struct actask{
    uint64_t       id;
    uint32_t       endtime;
    account_name   from;
    eosio::asset   quantity;
    uint32_t       createtime;
    uint64_t       optionanswerscnt;
    std::string    asktitle;
    std::string    optionanswers;

    uint64_t primary_key()const { return id; }
    EOSLIB_SERIALIZE( actask, (id)(endtime)(from)(quantity)(createtime)(optionanswerscnt)(asktitle)(optionanswers))
};

/*
@abi action answer
*/
struct answer{
    uint64_t  askid;
    account_name  from;
    uint32_t  choosedanswer;
    uint64_t primary_key()const { return askid; }
    uint64_t get_secondary()const { return from; }

    EOSLIB_SERIALIZE(answer, (askid)(from)(choosedanswer))
};

/**
*  @abi table
*/
struct answers{
    uint64_t  askid;
    std::vector<answer> answerlist;
    uint64_t primary_key()const { return askid; }
    EOSLIB_SERIALIZE(answers, (askid)(answerlist))
};


/**
*  @abi action
*/
struct releasemog{
    uint64_t       askid;
    EOSLIB_SERIALIZE(releasemog, (askid))
};


/*
@abi action
@abi table configa
*/
struct config{
    account_name admin;
    eosio::asset ansreqoct;
    account_name primary_key()const { return admin; }
    EOSLIB_SERIALIZE(config, (admin)(ansreqoct))
};
typedef eosio::multi_index<N(configa), config> Config;



/*
@abi action
@abi table configaskid
*/
struct configaskid{
    uint64_t key;
    uint64_t value;
    uint64_t primary_key()const { return key; }
    EOSLIB_SERIALIZE(configaskid, (key)(value))
};
typedef eosio::multi_index<N(configaskid), configaskid> ConfigAskId;
#define INDEX_ASKID 0
/*
    @abi action
*/
struct rmask{
    uint64_t  askid;
    EOSLIB_SERIALIZE(rmask, (askid))
};

typedef eosio::multi_index<N(ask), ask> askIndex;

typedef eosio::multi_index<N(answers), answers> AnswerIndex;

class ocaskans:public eosio::contract{

public:
    ocaskans(account_name self):contract(self){
        ansreqoct.amount = 0;

        eosio::symbol_name sn = eosio::string_to_symbol(4, globalsymbolname.c_str());
        ansreqoct.symbol = eosio::symbol_type(sn);

        Config c(self, aksansadmin);
        auto ite = c.find(aksansadmin);
        if(ite != c.end()){
            ansreqoct = ite->ansreqoct;
        }

        ConfigAskId cai(currentAdmin, aksansadmin);
        latestaskid = 1000;
        auto askIndexIdItem = cai.find(INDEX_ASKID);
        if(askIndexIdItem!=cai.end()){
            latestaskid = askIndexIdItem->value;
        }

    }

    uint32_t getAnswerCount(uint64_t askid);
    void removeAsk(const rmask & ra);
    void releaseMortgage( const releasemog& rm );
    void store_answer(const answer &a);
    void store_ask(const actask &c);
    void transferInline(const transfer &trs);
    void transferFromInline(const transferfromact &tf);

    void configinfo(const config &ansreqoct);

    static const uint64_t aksansadmin = N(ocaskans);
    static const uint64_t tokenContract = N(octoneos);
    static const uint64_t currentAdmin = N(ocaskans);
    eosio::asset ansreqoct;
    uint64_t latestaskid;

private:
    std::string globalsymbolname = "OCT";
    void send_deferred_transferfrom_transaction(transferfromact tf);
};







