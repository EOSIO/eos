/**
This contract follows the erc20 standard
contract ERC20Interface {
    function totalSupply() public constant returns (uint);
    function balanceOf(address tokenOwner) public constant returns (uint balance);
    function allowance(address tokenOwner, address spender) public constant returns (uint remaining);
    function transfer(address to, uint tokens) public returns (bool success);
    function approve(address spender, uint tokens) public returns (bool success);
    function transferFrom(address from, address to, uint tokens) public returns (bool success);
}
*/

#pragma once
#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>
#include <vector>

using std::string;
using std::array;

using namespace eosio;
using boost::container::flat_map;

//one cheque
struct approvetoPair
{
    account_name to;//cheque to who
    int64_t value;//cheque value
    approvetoPair() {}
    EOSLIB_SERIALIZE(approvetoPair, (to)(value))
};

/*
 * cheques list of someone
@abi table approves
*/
struct approveto {
   symbol_type symbol_name;//name of symbol
   std::vector<approvetoPair> approved;//list of cheque to others
   uint64_t primary_key()const { return symbol_name.name(); }
   EOSLIB_SERIALIZE( approveto, (symbol_name)(approved))
};

/*
account info
*/
struct account {
   asset     balance;//account balance
   uint64_t primary_key()const { return balance.symbol.name(); }
   EOSLIB_SERIALIZE( account, (balance))
};

/*
status of one currency
*/
struct curstats {
   asset          supply;//amount of money already supplied
   asset          max_supply;//Maximum supply of money
   account_name   issuer;//Currency issuer

   uint64_t primary_key()const { return supply.symbol.name(); }
   EOSLIB_SERIALIZE( curstats, (supply)(max_supply)(issuer))
};

class eosdactoken : public contract {
  public:
     eosdactoken( account_name self )
     :contract(self)
     {}

     struct fee_schedule {
        uint64_t primary_key()const { return 0; }

        array<extended_asset,7> fee_per_length;
        EOSLIB_SERIALIZE( fee_schedule, (fee_per_length) )
     };


     // ------------------------------------------------------------------------
     // get amount of  `spender` can withdraw from your account
     // ------------------------------------------------------------------------
     uint64_t allowanceOf( account_name owner,
     account_name spender,
     symbol_name  symbol){
         Approves approveobj(get_self(), owner);

         if(approveobj.find(symbol) != approveobj.end()){
             const auto &approSymIte = approveobj.get(symbol);
             auto approvetoPairIte = approSymIte.approved.begin();
             while(approvetoPairIte != approSymIte.approved.end()){

                 if(approvetoPairIte->to == spender){
                     print("allowanceOf[", account_name(approvetoPairIte->to), "]=", approvetoPairIte->value);
                     return approvetoPairIte->value;
                 }
                 approvetoPairIte++;
             }
             if(approvetoPairIte == approSymIte.approved.end()){
                 print("allowanceOf[", account_name(spender), "]=", 0);
             }

         }else{
             print("allowanceOf[", (account_name)spender, "]=", 0);
         }
         return 0;
     }

     typedef eosio::multi_index<N(accounts), account> Accounts;
     typedef eosio::multi_index<N(stats), curstats> Stats;
     typedef eosio::multi_index<N(approves), approveto> Approves;


     asset get_balance( account_name owner, symbol_name symbol )const {
        Accounts t( _self, owner );
        if(t.find(symbol) == t.end()){
            asset as;
            as.amount = 0;
            as.symbol = symbol_type(symbol);
            return as;
        }
        return t.get(symbol).balance;
     }


     asset get_supply( symbol_name symbol )const {
        Stats statstable( _self, symbol_type(symbol).name());
        const auto& st = statstable.get(symbol_type(symbol).name());
        return st.max_supply;
     }

      // ------------------------------------------------------------------------
      //A currency is issued, the issuer is ‘issuer’
      // ------------------------------------------------------------------------
      /// @abi action
      void create(        account_name           issuer,
      asset  currency
      );

      // ------------------------------------------------------------------------
      // Transfer the balance from token owner's account to `to` account
      // - from's account must have sufficient balance to transfer
      // ------------------------------------------------------------------------
      /// @abi action
      void transfer(
      account_name from,
      account_name to,
      asset        quantity,
      string       memo);

      // ------------------------------------------------------------------------
      // Transfer the balance from token owner's account to `to` account,and transfer 'feequantity' to 'tofeeadmin' account at the same time
      // - from's account must have sufficient balance to transfer
      // ------------------------------------------------------------------------
      /// @abi action
      void transferfee(       account_name from,
                              account_name to,
                              asset        quantity,
                              account_name tofeeadmin,
                              asset        feequantity,
                              string       memo);

      // ------------------------------------------------------------------------
      //Issue money to users
      // ------------------------------------------------------------------------
      /// @abi action
      void issue(
      account_name to,
      asset        quantity,
      string       memo);

      // ------------------------------------------------------------------------
      // Allow `spender` to withdraw from your account, multiple times, up to the `quantity` amount.
      // If this function is called again it overwrites the current allowance with value.
      // ------------------------------------------------------------------------
      /// @abi action
      void approve(
      account_name owner,
      account_name spender,
      asset quantity);

      // ------------------------------------------------------------------------
      // Returns the amount of tokens approved by the owner that can be
      // transferred to the spender's account
      // ------------------------------------------------------------------------
      /// @abi action
      void allowance(
      account_name owner,
      account_name spender,
      std::string  symbol);

      // ------------------------------------------------------------------------
      // Transfer `tokens` from the `from` account to the `to` account
      //
      // The calling account must already have sufficient tokens approve(...)-d
      // for spending from the `from` account and
      // - From account must have sufficient balance to transfer
      // - Spender must have sufficient allowance to transfer
      // ------------------------------------------------------------------------
      /// @abi action
      void transferfrom(
      account_name from,
      account_name to,
      asset quantity);

      // ------------------------------------------------------------------------
      //Get the token on symbol balance for account `owner`
      // ------------------------------------------------------------------------
      /// @abi action
      void balanceof(
      account_name owner,
      std::string  symbol);

      // ------------------------------------------------------------------------
      // Total supply of token symbol
      // ------------------------------------------------------------------------
      //@abi action
      void totalsupply(std::string  symbol){
          symbol_name sn = string_to_symbol(4, symbol.c_str());
          print("totalsupply[",symbol.c_str(),"]", get_supply(sn).amount);
      }


  private:
      //Sub currency to the 'owner' account,and the cost of 'bindwidth ram、cpu'  is paid by payer
      void sub_balance( account_name owner, asset currency,  uint64_t payer);

      //Add currency to the 'owner' account,and the cost of 'bindwidth ram、cpu'  is paid by payer
      void add_balance( account_name owner, asset currency, account_name payer );

      void checkasset(const asset &quantity);
};


