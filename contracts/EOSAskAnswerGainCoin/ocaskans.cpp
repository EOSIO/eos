/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/db.h>
#include <eosiolib/types.hpp>
#include <eosiolib/transaction.hpp>
#include "../EOSDACToken/eosdactoken.hpp"


#include "tool.hpp"
#include "ocaskans.hpp"


/**
 *  The init() and apply() methods must have C calling convention so that the blockchain can lookup and
 *  call these methods.
 */

void ocaskans::configinfo(const config &cp){
    require_auth(aksansadmin);

    eosio_assert(cp.ansreqoct.symbol == ansreqoct.symbol, ONLY_ORACLECHAIN_TOKEN_OCT_SUPPORTED);

    Config c(_self, aksansadmin);
    auto ite = c.find(aksansadmin);
    if(ite == c.end()){
        c.emplace(aksansadmin, [&](auto &s){
           s.admin = cp.admin;
           s.ansreqoct = cp.ansreqoct;
        });
    }else{
        c.modify(*ite, aksansadmin, [&](auto &s){
            s.admin = cp.admin;
            s.ansreqoct = cp.ansreqoct;
        });
    }

    this->ansreqoct = cp.ansreqoct;
}

void ocaskans::transferInline(const transfer &trs)
{
    require_auth(trs.from);
    INLINE_ACTION_SENDER(eosdactoken, transfer)(tokenContract, {trs.from,N(active)},
    { trs.from, trs.to, trs.quantity, std::string("") } );
}


void ocaskans::transferFromInline(const transferfromact &tf){
    require_auth(tf.from);

    eosio::asset fromBalance = eosdactoken(tokenContract).get_balance(tf.from, tf.quantity.symbol.name());
    eosio_assert(fromBalance.amount >= tf.quantity.amount,NOT_ENOUGH_OCT_TO_DO_IT);

    uint64_t allowed = eosdactoken(tokenContract).allowanceOf(tf.from, tf.to, tf.quantity.symbol.name());
    eosio_assert(allowed>= tf.quantity.amount, NOT_ENOUGH_ALLOWED_OCT_TO_DO_IT);

    INLINE_ACTION_SENDER(eosdactoken, transferfrom)(tokenContract, {currentAdmin, N(active)},
    { tf.from, tf.to, tf.quantity} );
}

/*
you can modify you self asks, or else will add a new ask
*/
void ocaskans::store_ask(const actask &askItemPar){
    actask c = askItemPar;
    require_auth(c.from);
    eosio_assert(askItemPar.quantity.symbol == ansreqoct.symbol, ONLY_ORACLECHAIN_TOKEN_OCT_SUPPORTED);
    eosio_assert(c.optionanswerscnt>=2 && c.optionanswerscnt<10000, OPTIONS_ANSWERS_COUNT_SHOULE_BIGGER_THAN_ONE);



    transferfromact transAct;
    transAct.from = askItemPar.from;
    transAct.to = aksansadmin;
    transAct.quantity = askItemPar.quantity;
    transferFromInline(transAct);

    askIndex askItem(currentAdmin, aksansadmin);
    auto to = askItem.find( c.id );

    ConfigAskId cai(currentAdmin, aksansadmin);
    auto askIndexIdItem = cai.find(INDEX_ASKID);
    if(askIndexIdItem!=cai.end()){
        latestaskid = askIndexIdItem->value;
    }
    uint64_t idincrease = latestaskid;

    if(to != askItem.end())
    {
        to = askItem.find(idincrease);
        while(to != askItem.end())
        {
             idincrease++;
             to = askItem.find(idincrease);
        }
    }
    c.id = idincrease;


    askItem.emplace(c.from, [&]( auto& s ) {
        s.id = c.id;
        s.createtime = now();
        s.endtime    = s.createtime+c.endtime;;
        s.from        = c.from;
        s.quantity = c.quantity;
        s.releasedLable = lable_not_release;
        s.optionanswerscnt = c.optionanswerscnt;
        s.asktitle = c.asktitle;
        s.optionanswers = c.optionanswers;
     });


    latestaskid+=1;
    if(askIndexIdItem == cai.end()){
        cai.emplace(c.from, [&](auto &s){
             s.key = INDEX_ASKID;
             s.value = latestaskid;
        });
    }else{
        cai.modify(askIndexIdItem, c.from, [&](auto &s){
             s.key = INDEX_ASKID;
             s.value = latestaskid;
        });
    }

     eosio::print("Answersid:", c.id);
}

void ocaskans::store_answer(const answer &a){

    require_auth(a.from);

    askIndex askItem(currentAdmin, aksansadmin);
    auto to = askItem.find( a.askid);
    if(to == askItem.end()){
        eosio_assert(false, ASK_NOT_EXISTS);
    }

    eosio_assert(to->releasedLable==lable_not_release, CANNOT_RELEASE_ASK_REPEAT);
    eosio_assert((to->optionanswerscnt>=a.choosedanswer)&&(a.choosedanswer>0), ILLEGAL_ANSWER);


    if(ansreqoct.amount>0){
        transferfromact transAct;
        transAct.from = a.from;
        transAct.to = aksansadmin;
        transAct.quantity = ansreqoct;
        transferFromInline(transAct);
    }



    AnswerIndex ai(currentAdmin, aksansadmin);
    auto ite = ai.find(a.askid);
    if (ite != ai.end()) {
        std::vector<answer> answersList = ite->answerlist;
        auto answerItem = answersList.begin();
        while(answerItem != answersList.end()){
            if(answerItem->from == a.from){
                eosio_assert(false, YOU_ANSWERED_THIS_QUESTION_EVER);
            }
            ++answerItem;
        }
        answersList.push_back(a);
        ai.modify(ite, 0, [&](auto &s){
            s.askid = a.askid;
            s.answerlist = answersList;
        });
    }
    else{
        ai.emplace(aksansadmin, [&](auto &s){
            s.askid = a.askid;
            std::vector<answer> ans;
            ans.push_back(a);
            s.answerlist = ans;
        });
    }
}


void ocaskans::releaseMortgage( const releasemog& rm ) {
    require_auth(aksansadmin);

    askIndex askContainer(currentAdmin, aksansadmin);
    auto askItem = askContainer.find(rm.askid);
    if(askItem == askContainer.end()){
         eosio_assert(false, ASK_NOT_EXISTS);
    }
    else{
          eosio_assert(askItem->releasedLable != lable_released, CANNOT_RELEASE_ASK_REPEAT);
          eosio_assert(askItem->endtime<=now(), TIME_NOT_REACHED);

          uint64_t nTotalCntAnswer = getAnswerCount(rm.askid);
          if(nTotalCntAnswer>0)
          {
              uint64_t avg = askItem->quantity.amount/nTotalCntAnswer;
              if(avg>0 || ansreqoct.amount>0){
                  AnswerIndex answerContainer(currentAdmin, aksansadmin);
                  auto answerItem = answerContainer.find(rm.askid);
                  if(answerItem != answerContainer.end()){
                      auto ansItem = answerItem->answerlist.begin();
                      std::vector<transfer> vectrs;
                      while(ansItem != answerItem->answerlist.end()){
                          transfer trs;
                          trs.from = aksansadmin;
                          trs.to = ansItem->from;
                          trs.memo = std::string("");
                          trs.quantity = askItem->quantity;
                          trs.quantity.amount = avg+ansreqoct.amount;
                          ++ansItem;
                          transferInline(trs);
                      }
                  }
              }
          }else{
               eosio_assert(false, LOGIC_ERROR);
          }

          askContainer.modify(askItem, 0, [&]( auto& s ) {
              s.id = askItem->id;
              s.endtime    = askItem->endtime;
              s.from        = askItem->from;
              s.quantity = askItem->quantity;
              s.releasedLable = lable_released;
              s.createtime = askItem->createtime;
              s.optionanswerscnt = askItem->optionanswerscnt;
              s.asktitle = askItem->asktitle;
              s.optionanswers = askItem->optionanswers;
           });
    }
}

void ocaskans::removeAsk(const rmask & ra){
    askIndex askContainer(currentAdmin, aksansadmin);
    auto  askItem = askContainer.find(ra.askid);
    if(askItem != askContainer.end()){
        askContainer.erase(askItem);
    }

    AnswerIndex ai(currentAdmin, aksansadmin);
    auto ansIte = ai.find(ra.askid);
    if(ansIte != ai.end()){
        ai.erase(ansIte);
    }
}

uint32_t ocaskans::getAnswerCount(uint64_t askid){
    AnswerIndex ai(currentAdmin, aksansadmin);
    auto answerItem = ai.find(askid);
    uint32_t count = 0;
    if(answerItem == ai.end()){
        return 0;
    }
    auto ansItem = answerItem->answerlist.begin();
    while(ansItem != answerItem->answerlist.end()){
        ++count;
        ++ansItem;
    }
    return count;
}


extern "C" {

using namespace eosio;

void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
    if( code == receiver){
        switch(action)
        {
            case N(ask):
            {
                ocaskans(receiver).store_ask( unpack_action_data<actask>() );
                break;
            }
            case N(answer):
            {
                ocaskans(receiver).store_answer( unpack_action_data<answer>() );
                break;
            }
            case N(releasemog):
            {
                ocaskans(receiver).releaseMortgage( unpack_action_data<releasemog>() );
                break;
            }
            case N(rmask):
            {
                ocaskans(receiver).removeAsk( unpack_action_data<rmask>() );
                break;
            }
            case N(config):{
                ocaskans(receiver).configinfo( unpack_action_data<config>() );
                break;
            }
        }
    }
  }
}

// extern "C"


