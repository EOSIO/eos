#include "eosiolib/eosio.hpp"

#include "oraclemarket.hpp"

using namespace eosio;

//@abi action
void OracleMarket::mortgage(account_name from, account_name server, const asset &quantity){
    require_auth(from);

    transferfromact tf(from, currentAdmin, quantity);
    transferFromInline(tf);

    ContractInfo userScores(_self, server);
    auto serverIte = userScores.find(SCORES_INDEX);
    eosio_assert(serverIte != userScores.end(), CONTRACT_NOT_REGISTER_STILL);

    Mortgaged mt(_self, from);
    if(mt.find(from) == mt.end()){

        std::vector<mortgagepair> mortgegelist;
        mortgagepair mp(server, quantity, now());
        mortgegelist.push_back(mp);

        mortgaged m(from, mortgegelist);
        mt.emplace(from, [&]( auto& s ) {
            s.from = from;
            s.mortgegelist = mortgegelist;
        });
    }else{
        auto itefrom = mt.find(from);
        mortgagepair mp(server, quantity, now());
        mt.modify(itefrom, from, [&](auto &s){
            s.mortgegelist.push_back(mp);
        });
    }
    eosio::print("mortgage!");
}

//@abi action
void OracleMarket::unfrosse(account_name server, account_name from, const asset & quantity){
    require_auth(server);

    Mortgaged mt(_self, from);
    auto mortIte = mt.find(from);
    eosio_assert(mortIte!=mt.end(), USER_NOT_MORTGAGED_CANNOT_RELEASE);


    std::vector<mortgagepair> mortgegelist = mortIte->mortgegelist;
    //find first
    for(auto ite = mortgegelist.begin();ite != mortgegelist.end(); ite++){

          if(quantity == ite->quantity && ite->server == server){
              ite->status = STATUS_MORTGAGE_PAIR_CAN_FREEZE;
              break;
          }
    }

    mt.modify(*mortIte, server, [&](auto & s){
        s.mortgegelist = mortgegelist;
        s.from = from;
    });
}


//@abi action
void OracleMarket::withdrawfro(account_name from){//withdrawfrozened
    require_auth(from);

    Mortgaged mt(_self, from);
    eosio_assert(mt.find(from)!=mt.end(), USER_NOT_MORTGAGED_CANNOT_RELEASE);

    auto mortIte = mt.get(from);

    int32_t toPunishMent = getPunishMentAmount(from);

    asset as;
    for(auto ite = mortIte.mortgegelist.begin();ite != mortIte.mortgegelist.end(); ite++){
          as = ite->quantity;

          ContractInfo conInfo(_self, ite->server);
          auto serverIte = conInfo.find(SCORES_INDEX);

          if(ite->createtime+serverIte->assfrosec<now() || ite->status == STATUS_MORTGAGE_PAIR_CAN_FREEZE){
               ite = mortIte.mortgegelist.erase(ite);
               toPunishMent -=ite->quantity.amount;

               if(ite==mortIte.mortgegelist.end()){
                    break;
               }
          }
    }

    eosio_assert(toPunishMent<0, AMOUNT_CAN_WITHDRAW_NOW_IS_ZERO);
    as.amount = (-toPunishMent);
    transferInline(balanceAdmin, from, as, "");

    auto toMofify = mt.find(from);
    if(mortIte.mortgegelist.size() ==0 ){
        mt.erase(toMofify);
    }else{
        mt.modify(toMofify, from, [&](auto & s){
            s.mortgegelist = mortIte.mortgegelist;
        });
    }
}

//weight=balance(oct)*(now()-lastvotetime)
//voter account is server account
//@abi action
bool OracleMarket::vote(account_name voted, account_name voter, int64_t weight, uint64_t status){
    require_auth(voter);

    ContractInfo conInfo(_self, voter);
    auto serverIte = conInfo.find(SCORES_INDEX);
    eosio_assert(serverIte != conInfo.end(), CONTRACT_NOT_REGISTER_STILL);

    Mortgaged mt(_self, voted);
    eosio_assert(mt.find(voted) != mt.end(), CANNOT_VOTE_SOMEONE_NOT_JOIN_YOU_SERVER);

    auto iteM = mt.get(voted);
    auto iteMToModify = mt.find(voted);
    bool didvoted = false;
    for(auto ite = iteM.mortgegelist.begin(); ite != iteM.mortgegelist.end(); ite++){

          if(ite->server == voter && ite->votedcount==0){

               didvoted = true;
               ite->votedcount = 1;
               mt.modify(iteMToModify, voter, [&](auto &s){
                   s.mortgegelist = iteM.mortgegelist;
               });

               UserScores userScores(_self, voted);
               auto votedUser = userScores.find(voted);
               if(votedUser == userScores.end()){
                   userScores.emplace(voter, [&](auto &s){
                        s.owner = voted;
                        s.scorescnt = weight;
                   });
               }else{
                   userScores.modify(votedUser, voter, [&](auto &s){
                        s.scorescnt = votedUser->scorescnt + weight;
                   });
               }
          }
    }
    eosio_assert(didvoted, NO_ACTIONS_CAN_BE_VOTED_NOW_ABOUNT_THIS_USER);
    return didvoted;
}

uint64_t OracleMarket::getPunishMentAmount(account_name name){
    BehaviorScores bs(_self, dataAdmin);
    if(bs.begin()==bs.end()){
        return 0;
    }

    auto secondary_index = bs.get_index<N(bysecondary)>();
    auto ite = secondary_index.begin();

    uint32_t countToPunishMent = 0;
    while(ite!=secondary_index.end()){
        if(ite->status == STATUS_VOTED_EVIL || ite->status == STATUS_APPEALED || ite->status ==STATUS_APPEALED_CHECKED_EVIL){

            ContractInfo conInfo(_self, ite->server);
            auto serverIte = conInfo.find(SCORES_INDEX);
            countToPunishMent += serverIte->fee.amount;

            bs.modify(*ite, 0, [&](auto &s){
                s.status = STATUS_DEALED;
            });
        }
        ite++;
    }

    return countToPunishMent;
}
//@abi action
void OracleMarket::votebehavior(account_name server, account_name user, uint64_t status, std::string memo){
    require_auth(server);

    eosio_assert(status==STATUS_VOTED_EVIL||status==STATUS_VOTED_GOOD, ONLY_CAN_VOTED_AD_GOOD_OR_EVIL);

    BehaviorScores bs(_self, dataAdmin);
    uint64_t idFrom = 0;
    if(bs.rbegin()!=bs.rend()){
        idFrom = bs.rbegin()->id+1;
    }

    int64_t weight = status==STATUS_VOTED_EVIL?evilbehscoRate:-evilbehscoRate;
    eosio_assert(vote(user, server, weight, status), YOU_VOTED_REPEAT);

    bs.emplace(server, [&](auto &s){
        s.id = idFrom;
        s.server = server;
        s.user = user;
        s.memo = memo;
        s.status = status;
        s.appealmemo = "";
        s.justicememo = "";
    });
}

//@abi action
void OracleMarket::appealgood(account_name user, uint64_t idevilbeha, std::string memo){
       require_auth(user);
       BehaviorScores bs(_self, dataAdmin);
       auto bsIte = bs.find(idevilbeha);

       eosio_assert(bsIte != bs.end(), APPEALED_BEHAVIOR_NOT_EXIST);
       eosio_assert(bsIte->status < STATUS_APPEALED, APPEALED_OR_CHECKED_BY_ADMIN);

       bs.modify(*bsIte, user, [&](auto &s){
           s.status = STATUS_APPEALED;
           s.appealmemo = memo;
       });
}

//@abi action
void OracleMarket::admincheck(account_name admin, uint64_t idevilbeha, std::string memo, uint8_t status){
     require_auth(admin);

     BehaviorScores bs(_self, dataAdmin);
     auto bsIte = bs.find(idevilbeha);

     eosio_assert(bsIte != bs.end(), APPEALED_BEHAVIOR_NOT_EXIST);
     eosio_assert(status== STATUS_APPEALED_CHECKED_GOOD || status == STATUS_APPEALED_CHECKED_EVIL || status == STATUS_APPEALED_CHECKED_UNKNOWN,  NOT_INT_CHECKED_STATUS);

     if(bsIte->status == STATUS_DEALED
             && (status== STATUS_APPEALED_CHECKED_GOOD || status == STATUS_APPEALED_CHECKED_UNKNOWN)){

         ContractInfo conInfo(_self, bsIte->server);
         auto serverIte = conInfo.find(SCORES_INDEX);

         asset as;
         as.symbol = octsymbol;
         as.amount = serverIte->fee.amount;

         transferInline(currentAdmin, bsIte->user, as, "oracle market admin check");
     }

     bs.modify(*bsIte, admin, [&](auto &s){
         s.status = status;
         s.justicememo = memo;
     });
}


//@abi action
void OracleMarket::setconscolim(account_name conadm, uint64_t assfrosec,  uint64_t scores, asset fee){//set contract call, minimum scores required
    require_auth(conadm);

    ContractInfo conInfo(_self, conadm);
    eosio_assert(fee.amount>=0, ASSFROSEC_SCORES_FEE);

    auto ciItem = conInfo.find(SCORES_INDEX);
    if(ciItem != conInfo.end()){
        conInfo.modify(ciItem, conadm, [&](auto &s){
            s.assfrosec = assfrosec;
            s.serverindex = 0;
            s.scores = scores;
            s.fee = fee;
        });
    }else{
        conInfo.emplace(conadm, [&](auto &s){
           s.assfrosec = assfrosec;
           s.serverindex = 0;
           s.scores = scores;
           s.fee = fee;
        });
    }
}

void OracleMarket::clear(account_name scope, uint64_t id){
    //int32_t db_end_i64(account_name code, account_name scope, table_name table);
    require_auth(currentAdmin);
    int32_t ite = db_find_i64(N(eosoramar), N(eosoramar), N(behsco), id);
    eosio_assert(ite >= 0, "primary_i64_general - db_find_i64");
    db_remove_i64(ite);
}

