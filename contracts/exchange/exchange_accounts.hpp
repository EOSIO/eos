#pragma once
#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>

namespace eosio {

   using boost::container::flat_map;

   /**
    *  Each user has their own account with the exchange contract that keeps track
    *  of how much a user has on deposit for each extended asset type. The assumption
    *  is that storing a single flat map of all balances for a particular user will
    *  be more practical than breaking this down into a multi-index table sorted by
    *  the extended_symbol.  
    */
   struct exaccount {
      /*用户ID*/
      account_name                         owner;
      /*保存用户下的所有代币资产
       *key:extended_symbol(代币类型)
       *value:代币余额
       * */
      flat_map<extended_symbol, int64_t>   balances;

      uint64_t primary_key() const { return owner; }
      EOSLIB_SERIALIZE( exaccount, (owner)(balances) )
   };

   /*定义一张代币余额表
    *表名:<N(exaccounts)
    *表结构体:struct exaccount
    * */
   typedef eosio::multi_index<N(exaccounts), exaccount> exaccounts;


   /**
    *  Provides an abstracted interface around storing balances for users. This class
    *  caches tables to make multiple accesses effecient.
    */
   struct exchange_accounts {
      /*构造函数*/
      exchange_accounts( account_name code ):_this_contract(code){}

      /*对用户代币金额累加*/
      void adjust_balance( account_name owner, extended_asset delta, const string& reason = string() ); 

      private: 
         /*有权限调用本合约的对象*/
         account_name _this_contract;
         /**
          *  Keep a cache of all accounts tables we access
          */
         /*保存每个用户的代币余额表已实例化的对象(每个用户对应同一个对象？)*/
         flat_map<account_name, exaccounts> exaccounts_cache;
   };
} /// namespace eosio
