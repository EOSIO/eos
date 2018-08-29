#pragma once
#include <eosiolib/eosio.hpp>

const uint64_t tokenContract = N(octoneos);
using namespace eosio;


#define PARAMETER_ERROR "6000001 PARAMETER_ERROR" //参数错误
#define BLANCE_NOT_ENOUGH "6000002 BLANCE_NOT_ENOUGH" //余额不足
#define ASK_NOT_EXISTS "6000006 ASK_NOT_EXISTS"//问题不存在
#define TIME_NOT_REACHED "6000007 TIME_NOT_REACHED"//时间未到
#define CANNOT_RELEASE_ASK_REPEAT "6000008 CANNOT_RELEASE_ASK_REPEAT"//问题已经释放，不能重复释放一个问题
#define BALANCE_SUPRE_UINT64_LIMIT "6000009 BALANCE_SUPRE_UINT64_LIMIT"//余额已经超过代码极限（理论永远不会发生，但除非恶搞极限测试）
#define ASK_RELEASED_CANNOT_ANSWER "6000010 ASK_RELEASED_CANNOT_ANSWER"//问题已经释放，不能huida
#define ILLEGAL_ANSWER "6000011 ILLEGAL_ANSWER"//非法回答，比如只有2个答案，结果用户选3或则-1
#define OPTIONS_ANSWERS_COUNT_SHOULE_BIGGER_THAN_ONE  "6000012 OPTIONS_ANSWERS_COUNT_SHOULE_BIGGER_THAN_ONE_AND_LESS_THAN_10000"
#define CANNOT_MODIFY_OTHERS_ASK  "6000013 CANNOT_MODIFY_OTHERS_ASK"
#define NOT_ENOUGH_OCT_TO_DO_IT  "6000014 NOT_ENOUGH_OCT_TO_DO_IT"//没有足够的oct
#define CANNOT_MODIFY_RELEASED_ASK  "6000015 CANNOT_MODIFY_RELEASED_ASK"//不能回答已经释放的问题
#define YOU_ANSWERED_THIS_QUESTION_EVER  "6000016 YOU_ANSWERED_THIS_QUESTION_EVER"//你已经曾经回答过这个问题
#define NOT_ENOUGH_ALLOWED_OCT_TO_DO_IT  "6000017 NOT_ENOUGH_ALLOWED_OCT_TO_DO_IT"//没有抵押足够的OCT
#define INVALID_QUANTITY "6000018 INVALID_QUANTITY"//无效额数
#define MUST_ISSUE_POSITIVE_QUANTITY "6000019 MUST_ISSUE_POSITIVE_QUANTITY"//额度必须为正整数
#define NSUFFICIENT_AUTHORITY "6000020 NSUFFICIENT_AUTHORITY"//没有足够授权
#define ACCOUNT_IS_FROZEN_BY_ISSUER "6000021 ACCOUNT_IS_FROZEN_BY_ISSUER"//账户倍发行者冻结
#define ALL_TRANSFERS_ARE_FROZEN_BY_ISSUER "6000022 ALL_TRANSFERS_ARE_FROZEN_BY_ISSUER"//不能给被冻结的账户转账
#define ACCOUNT_IS_NOT_WHITE_LISTED "6000023 ACCOUNT_IS_NOT_WHITE_LISTED"//账户没有在白名单
#define CAN_ONLY_TRANSFER_TO_WHITE_LISTED_ACCOUNTS "6000024 CAN_ONLY_TRANSFER_TO_WHITE_LISTED_ACCOUNTS"//只能转账给白名单账户
#define RECEIVER_REQUIRES_WHITELIST_BY_ISSUER "6000025 RECEIVER_REQUIRES_WHITELIST_BY_ISSUER"//货币状态或者接收方状态要求白名单属性
#define INVALID_SYMBOL_NAME "6000026 INVALID_SYMBOL_NAME"//无效的货币
#define TOKEN_WITH_SYMBOL_ALREADY_EXISTS "6000027 TOKEN_WITH_SYMBOL_ALREADY_EXISTS"//货币已经存在

#define LOGIC_ERROR "6000028 LOGIC_ERROR" //参数错误
#define TOKEN_MAX_SUPPLY_MUST_POSITIVE "6000029 TOKEN_MAX_SUPPLY_MUST_POSITIVE" //参数错误
#define CANNOT_TRANSFER_TO_YOURSELF "6000030 CANNOT_TRANSFER_TO_YOURSELF"
#define TO_ACCOUNT_DOES_NOT_EXIST "6000031 TO_ACCOUNT_DOES_NOT_EXIST"
#define SYMBOL_PRECISION_MISMATCH "6000032 SYMBOL_PRECISION_MISMATCH"
#define APPROVE_QUANTITY_MUST_POSITIVE "6000033 APPROVE_QUANTITY_MUST_POSITIVE"
#define QUANTITY_EXCEEDS_AVAILABLE_SUPPLY "6000034 QUANTITY_EXCEEDS_AVAILABLE_SUPPLY"
#define TOKEN_WITH_SYMBOL_DOES_NOT_EXIST_CREATE_TOKEN_BEFORE_ISSUE "6000035 TOKEN_WITH_SYMBOL_DOES_NOT_EXIST_CREATE_TOKEN_BEFORE_ISSUE"




#define NOT_ENOUGH_ASSET_CAN_UNFROZEN " NOT_ENOUGH_asset_CAN_UNFROZEN"
#define MORTGAGE_FREEZEN_SECS_NOT_ENOUGH " MORTGAGE_FREEZEN_SECS_NOT_ENOUGH"
#define CONTRACT_NOT_REGISTER_STILL " CONTRACT_NOT_REGISTER_STILL"
#define USER_NOT_MORTGAGED_CANNOT_RELEASE " USER_NOT_MORTGAGED_CANNOT_RELEASE"
#define PERMISSION_NOT_SANTISFY " PERMISSION_NOT_SANTISFY"
#define CANNOT_VOTE_SOMEONE_NOT_JOIN_YOU_SERVER " CANNOT_VOTE_SOMEONE_NOT_JOIN_YOU_SERVER"
#define YOU_VOTED_REPEAT " YOU_VOTED_REPEAT"

#define APPEALED_BEHAVIOR_NOT_EXIST " YOU_VOTED_REPEAT"
#define APPEALED_OR_CHECKED_BY_ADMIN " APPEALED_OR_CHECKED_BY_ADMIN"

#define NOT_INT_CHECKED_STATUS " NOT_INT_CHECKED_STATUS"

#define AMOUNT_CAN_WITHDRAW_NOW_IS_ZERO " AMOUNT_CAN_WITHDRAW_NOW_IS_ZERO"
#define BAD_ERROR_MORTGAGE_DATA_NOT_EXIST " BAD_ERROR_MORTGAGE_DATA_NOT_EXIST"
#define ASSFROSEC_SCORES_FEE " ASSFROSEC_SCORES_FEE>=0"

#define NO_ACTIONS_CAN_BE_VOTED_NOW_ABOUNT_THIS_USER " NO_ACTIONS_CAN_BE_VOTED_NOW_ABOUNT_THIS_USER"

#define ONLY_CAN_VOTED_AD_GOOD_OR_EVIL " ONLY_CAN_VOTED_AD_GOOD_OR_EVIL"


struct transferfromact {
    account_name from;
    account_name to;
    eosio::asset quantity;

    transferfromact(){}

    transferfromact(account_name parfrom, account_name parto, eosio::asset parquantity){
        this->from =  parfrom;
        this->quantity = parquantity;
        this->to = parto;
    }

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


account_name currentAdmin = N(eosoramar);

void transferInline(
    account_name from,
    account_name to,
    eosio::asset      quantity,
    std::string       memo)
{
    INLINE_ACTION_SENDER(eosdactoken, transfer)(tokenContract, {from,N(active)},
    { from, to, quantity, memo } );
}


void transferFromInline(const transferfromact &tf){
    //require_auth(tf.to);

    eosio::asset fromBalance = eosdactoken(tokenContract).get_balance(tf.from, tf.quantity.symbol.name());
    eosio_assert(fromBalance.amount >= tf.quantity.amount,NOT_ENOUGH_OCT_TO_DO_IT);

    uint64_t allowed = eosdactoken(tokenContract).allowanceOf(tf.from, tf.to, tf.quantity.symbol.name());
    eosio_assert(allowed>= tf.quantity.amount, NOT_ENOUGH_ALLOWED_OCT_TO_DO_IT);

    INLINE_ACTION_SENDER(eosdactoken, transferfrom)(tokenContract, {currentAdmin, N(active)},
    { tf.from, tf.to, tf.quantity} );
}
