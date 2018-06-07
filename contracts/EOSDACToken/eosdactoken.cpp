/**
 * author chenlian@oraclechain.io
 * version 1.0
 * source reference eosdactoken contract,thanks eos contracts/eosdactoken contract
 * this eosdactoken follow the erc2.0 norms(trans,issue,approve,transferfrom,balanceof,allowance)
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "eosdactoken.hpp"
#include"tool.hpp"

EOSIO_ABI( eosdactoken, (transfer)(create)(issue)(transferfee)(approve)(transferfrom)(balanceof)(allowance)(totalsupply))

using std::string;
using std::array;
using namespace eosio;


    void eosdactoken::checkasset(const asset &quantity){
        eosio_assert( quantity.symbol.is_valid(), INVALID_SYMBOL_NAME);
        auto sym = quantity.symbol.name();
        Stats statstable( _self, sym );
        const auto& ite = statstable.find( sym );
        eosio_assert( ite != statstable.end(),  TOKEN_WITH_SYMBOL_DOES_NOT_EXIST_CREATE_TOKEN_BEFORE_ISSUE);
        const auto& st = *ite;
        eosio_assert( quantity.is_valid(), INVALID_QUANTITY );
        eosio_assert( quantity.amount > 0, MUST_ISSUE_POSITIVE_QUANTITY );
        eosio_assert( quantity.symbol == st.supply.symbol, SYMBOL_PRECISION_MISMATCH);
        checkoutAmount(quantity.amount);
    }

    void eosdactoken::issue(account_name to,
                         asset        quantity,
                         string       memo) {

        checkasset(quantity);

        eosio_assert( memo.size() <= 256, MEMO_HAS_MORE_THAN_256_BYTES);
        auto sym = quantity.symbol.name();
        Stats statstable( _self, sym );
        const auto& ite = statstable.find( sym );
        const auto& st = *ite;
        eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, QUANTITY_EXCEEDS_AVAILABLE_SUPPLY);

        require_auth( st.issuer);

        statstable.modify( st, 0, [&]( auto& s ) {
           s.supply += quantity;
        });

        add_balance( st.issuer, quantity,  st.issuer);

        if( to != st.issuer )
        {
           SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
        }
    }

    void eosdactoken::transferfee(
            account_name from,
            account_name to,
            asset        quantity,
            account_name tofeeadmin,
            asset        feequantity,
            string       memo){

          checkasset(quantity);
          checkasset(feequantity);

          require_auth(from);

          eosio_assert( from != to, CANNOT_TRANSFER_TO_YOURSELF);
          eosio_assert( from != tofeeadmin, CANNOT_TRANSFER_TO_YOURSELF);

          eosio_assert( is_account( to ), TO_ACCOUNT_DOES_NOT_EXIST);
          eosio_assert( is_account( tofeeadmin ), TO_ACCOUNT_DOES_NOT_EXIST);

          auto sym = quantity.symbol.name();
          Stats statstable( _self, sym );
          const auto& st = statstable.get( sym );

          require_recipient( from );
          require_recipient( to );
          require_recipient( tofeeadmin );

          sub_balance( from, quantity, from );
          add_balance( to, quantity, from);

          sub_balance( from, feequantity, from);
          add_balance( tofeeadmin, feequantity, from);
    }

    void eosdactoken::transfer(  account_name from,
                              account_name to,
                              asset        quantity,
                              string       memo) {

        checkasset(quantity);

        require_auth(from);

        eosio_assert( from != to, CANNOT_TRANSFER_TO_YOURSELF);
        eosio_assert( is_account( to ), TO_ACCOUNT_DOES_NOT_EXIST);

        auto sym = quantity.symbol.name();
        Stats statstable( _self, sym );
        const auto& st = statstable.get( sym );

        require_recipient( from );
        require_recipient( to );

        eosio_assert( memo.size() <= 256, MEMO_HAS_MORE_THAN_256_BYTES);

        sub_balance( from, quantity, from);
        add_balance( to, quantity, from);
    }

    void eosdactoken::sub_balance( account_name owner, asset value, uint64_t payer) {

        require_auth(payer);

        Accounts from_acnts( _self, owner );

       const auto& from = from_acnts.get( value.symbol.name(),  NO_BALANCE_OBJECT_FOUND_FOR_THIS_ACCOUNT);
       eosio_assert( from.balance.amount >= value.amount, BLANCE_NOT_ENOUGH );

       if( from.balance.amount == value.amount ) {
            from_acnts.erase( from );
         } else {
            from_acnts.modify( from, payer, [&]( auto& a ) {
                a.balance -= value;
            });
       }
    }

    void eosdactoken::add_balance( account_name owner, asset value, account_name payer )
    {
       require_auth(payer);

       Accounts to_acnts( _self, owner );
       auto to = to_acnts.find( value.symbol.name() );
       if( to == to_acnts.end() ) {
          to_acnts.emplace( payer, [&]( auto& a ){
             a.balance = value;
          });
       } else {
          to_acnts.modify( to, payer, [&]( auto& a ) {
            a.balance += value;
          });
       }
    }


    void eosdactoken::approve(account_name owner,
                           account_name spender,
                           asset quantity){

        checkasset(quantity);

        require_auth(owner);

        eosio_assert( is_account( spender ), TO_ACCOUNT_DOES_NOT_EXIST);

        Accounts from_acnts( _self, owner );
        const auto& from = from_acnts.find( quantity.symbol.name());

        eosio_assert(from!=from_acnts.end(),  YOU_NOT_HAVE_THIS_TOKEN_NOW);

        eosio_assert( from->balance.amount >= quantity.amount, BLANCE_NOT_ENOUGH );

        Approves approveobj(_self, owner);
        if(approveobj.find(quantity.symbol.name()) != approveobj.end()){

            auto &approSymIte = approveobj.get(quantity.symbol.name());

            approveobj.modify(approSymIte, owner, [&](auto &a){

                auto approvetoPairIte = a.approved.begin();

                bool finded = false;
                while(approvetoPairIte != a.approved.end()){

                    if(approvetoPairIte->to == spender){

                        finded = true;
                        if(quantity.amount == 0){
                            a.approved.erase(approvetoPairIte);
                        }else{
                            approvetoPairIte->value = quantity.amount;
                        }
                        break;
                    }
                    approvetoPairIte++;
                }

                if(!finded && quantity.amount>0){
                    approvetoPair atp;
                    atp.to = spender;
                    atp.value = quantity.amount;
                    a.approved.push_back(atp);
                }
            });
        }else if(quantity.amount>0){

            approvetoPair atp;
            atp.to = spender;
            atp.value = quantity.amount;

            approveobj.emplace(owner, [&](auto &a){
                a.symbol_name = quantity.symbol;
                a.approved.push_back(atp);
            });
        }
    }

    void eosdactoken::balanceof(account_name owner,
                             std::string  symbol){
        symbol_name sn = string_to_symbol(4, symbol.c_str());
        print("balanceof[",symbol.c_str(),"]", get_balance(owner, symbol_type(sn).name()));
    }


    void eosdactoken::allowance(account_name owner,
                              account_name spender,
                              std::string  symbol){
        Approves approveobj(_self, owner);

        symbol_type st = symbol_type(string_to_symbol(4, symbol.c_str()));
         if(approveobj.find(st.name()) != approveobj.end()){
             const auto &approSymIte = approveobj.get(st.name());
             auto approvetoPairIte = approSymIte.approved.begin();
             while(approvetoPairIte != approSymIte.approved.end()){

                 if(approvetoPairIte->to == spender){
                     print("allowanceof[", account_name(approvetoPairIte->to), "]=", approvetoPairIte->value);
                     return;
                 }
                 approvetoPairIte++;
             }
             if(approvetoPairIte == approSymIte.approved.end()){
                 print("allowanceOf[", account_name(spender), "]=", 0);
             }

         }else{
             print("allowanceOf[", (account_name)spender, "]=", 0);
         }
    }

    void eosdactoken::transferfrom(account_name owner,
                                account_name spender,
                                asset quantity){

        checkasset(quantity);

        require_auth(spender);

        eosio_assert( is_account( owner ), TO_ACCOUNT_DOES_NOT_EXIST);


       eosio_assert( quantity.amount > 0, MUST_ISSUE_POSITIVE_QUANTITY);

       Approves approveobj(_self, owner);
       if(approveobj.find(quantity.symbol.name()) != approveobj.end()){

           const auto &approSymIte = approveobj.get(quantity.symbol.name());
           approveobj.modify(approSymIte, owner, [&](auto &a){
               a = approSymIte;
               auto approvetoPairIte = a.approved.begin();
               bool finded = false;
               while(approvetoPairIte != a.approved.end()){

                   if(approvetoPairIte->to == spender){
                       finded = true;
                       eosio_assert(approvetoPairIte->value>=quantity.amount, NOT_ENOUGH_ALLOWED_OCT_TO_DO_IT);
                       eosio_assert(approvetoPairIte->value > approvetoPairIte->value-quantity.amount, NOT_ENOUGH_ALLOWED_OCT_TO_DO_IT);
                       approvetoPairIte->value -= quantity.amount;

                       checkoutAmount(approvetoPairIte->value);

                       if(approvetoPairIte->value == 0){
                           a.approved.erase(approvetoPairIte);
                       }

                       require_recipient( spender );
                       sub_balance( owner, quantity, spender);
                       add_balance( spender,   quantity, spender);
                       break;
                   }
                   approvetoPairIte++;
               }
               if(!finded){
                   eosio_assert(false, NOT_ENOUGH_ALLOWED_OCT_TO_DO_IT);
               }
           });
       }else{
           eosio_assert(false, NOT_ENOUGH_ALLOWED_OCT_TO_DO_IT);
       }
    }

    void eosdactoken::create(       account_name           issuer,
                                 asset                  currency) {
      require_auth( _self );

      auto sym = currency.symbol;
      eosio_assert( sym.is_valid(), INVALID_SYMBOL_NAME);
      eosio_assert( currency.is_valid(), INVALID_QUANTITY);
      eosio_assert( currency.amount>0, TOKEN_MAX_SUPPLY_MUST_POSITIVE) ;

       Stats statstable( _self, sym.name() );
       auto existing = statstable.find( sym.name() );
       eosio_assert( existing == statstable.end(), TOKEN_WITH_SYMBOL_ALREADY_EXISTS);



       statstable.emplace( _self, [&]( auto& s ) {
          s.supply.symbol = currency.symbol;
          s.max_supply    = currency;
          s.issuer        = issuer;
       });
    }
