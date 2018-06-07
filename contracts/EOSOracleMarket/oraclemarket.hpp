/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/db.h>
#include <eosiolib/asset.hpp>
#include <eosiolib/serialize.hpp>

#include "../EOSDACToken/eosdactoken.hpp"

#include "./tool.hpp"


#define STATUS_MORTGAGE_PAIR_CANNOT_FREEZE 0
#define STATUS_MORTGAGE_PAIR_CAN_FREEZE 1

#define STATUS_NOT_VOTED  0
#define STATUS_VOTED_GOOD 1
#define STATUS_VOTED_EVIL 2
#define STATUS_VOTED_ALREADY 3
#define STATUS_APPEALED 4
#define STATUS_APPEALED_CHECKED_GOOD 5
#define STATUS_APPEALED_CHECKED_EVIL 6
#define STATUS_APPEALED_CHECKED_UNKNOWN 7
#define STATUS_DEALED 8

struct mortgagepair{
    mortgagepair(){
    }

    mortgagepair(
    account_name serverpar,
    asset       quantitypar,
    uint64_t     createtimepar){
        this->createtime = createtimepar;
        this->server = serverpar;
        this->quantity = quantitypar;
        this->status = STATUS_MORTGAGE_PAIR_CANNOT_FREEZE;
        this->votedcount = 0;
    }

    account_name server;
    asset       quantity;
    uint64_t    createtime;
    uint64_t    status;
    uint64_t    votedcount;

    EOSLIB_SERIALIZE( mortgagepair, (server)(quantity)(createtime)(status)(votedcount))
};

//@abi table mortgaged
struct mortgaged{
    account_name from;
    std::vector<mortgagepair> mortgegelist;

    mortgaged(){}

    mortgaged(account_name frompar,
    std::vector<mortgagepair> mortgegelistpar){
        this->from = frompar;
        this->mortgegelist = mortgegelistpar;
    }

    account_name primary_key()const { return from;}
    EOSLIB_SERIALIZE( mortgaged, (from)(mortgegelist))
};

typedef eosio::multi_index<N(mortgaged), mortgaged> Mortgaged;

#define STATUS_UNKNOWN_HEHAVIOR 0
#define STATUS_BAD_BEHAVIOR 1
#define STATUS_GOOD_BEHAVIOR 2

//@abi table userscores
struct scores{
    scores(){}

    scores(account_name ownerpar,
    int64_t scorescntpar){
        this->owner = ownerpar;
        this->scorescnt = scorescntpar;
    }

    account_name owner;
    int64_t scorescnt;
    account_name primary_key()const { return owner;}
    EOSLIB_SERIALIZE( scores, (owner)(scorescnt));
};

#define SCORES_INDEX 0
//@abi table contractinfo i64
struct contractinfo{
    uint64_t serverindex;
    uint64_t assfrosec;//asset frozen seconds
    uint64_t scores;
    asset fee;

    contractinfo(){}

    contractinfo(uint64_t serverindexPar, uint64_t assfrosecPar, uint64_t scoresPar, asset feePar){
        this->serverindex = serverindexPar;
        this->assfrosec = assfrosecPar;
        this->scores = scoresPar;
        this->fee = feePar;
    }

    account_name primary_key()const { return serverindex;}
    EOSLIB_SERIALIZE( contractinfo, (serverindex)(assfrosec)(scores)(fee));
};

#define STATUS_BEHAVIOR_EVIL  0
#define STATUS_BEHAVIOR_GOOD  1
#define STATUS_BEHAVIOR_UNKNOWN 2


//@abi table behsco
struct behsco{
    uint64_t id;
    account_name server;
    std::string  memo;
    account_name user;

    uint64_t    status;
    std::string appealmemo;
    std::string justicememo;

    uint64_t primary_key()const { return id;}
    account_name get_secondary()const { return server; }

    EOSLIB_SERIALIZE( behsco, (id)(server)(memo)(user)(status)(appealmemo)(justicememo));
};


typedef eosio::multi_index<N(userscores), scores> UserScores;
typedef eosio::multi_index<N(contractinfo), contractinfo> ContractInfo;//Invoking the contract, the minimum required score,default is zero

typedef multi_index<N(behsco), behsco,//behsco
   indexed_by< N(bysecondary), const_mem_fun<behsco, account_name, &behsco::get_secondary> >
> BehaviorScores;

class OracleMarket : public eosio::contract{

public:
    OracleMarket( account_name self)
         :contract(self){
        balanceAdmin = currentAdmin;
        dataAdmin = currentAdmin;
    }

    account_name balanceAdmin;
    account_name dataAdmin;


    //@abi action
    void mortgage(account_name from, account_name server, const asset &quantity);

    //@abi action
    void unfrosse(account_name server, account_name from, const asset & quantity);

    //@abi action
    void withdrawfro(account_name from);

    //weight=balance(oct)*(now()-lastvotetime)
    //voter account is server account
    bool vote(account_name voted, account_name voter, int64_t weight, uint64_t status);

    uint64_t getPunishMentAmount(account_name name);

    //@abi action
    void votebehavior(account_name server, account_name user, uint64_t status, std::string memo);

    //@abi action
    void appealgood(account_name user, uint64_t idevilbeha, std::string memo);

    //@abi action
    void admincheck(account_name admin, uint64_t idevilbeha, std::string memo, uint8_t status);


    //@abi action
    void setconscolim(account_name conadm, uint64_t assfrosec,  uint64_t scores, asset fee);

    //@abi action
    void clear(account_name scope, uint64_t id);

    const uint64_t normalServerScoresRate = 1;//Provide a normal service and get extra points
    const uint64_t appealAsGoodScoresExtraRate = 1;

    const uint64_t evilbehscoRate = -10;//Evil offensive score, It is evil to abuse others.
    const uint64_t evilBehaviorOCTPunishment = 10000;

    const uint64_t minEvilVoteTimeIntervar = 24*60*60;//The minimum time interval for bad votes
    const uint64_t finaltimeSecFrozen = 2*24*60*60;//mortgage freeze time in seconds

    symbol_type octsymbol = (string_to_symbol(4, "OCT"));
};
/*
OI server need frozen time interface
*/

EOSIO_ABI( OracleMarket, (mortgage)(unfrosse)(withdrawfro)(votebehavior)(appealgood)(admincheck)(setconscolim)(clear))


