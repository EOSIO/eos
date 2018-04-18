/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "erc20currency.hpp"

namespace oct {

   using std::string;
   using std::array;
using namespace eosio;
   /**
    *  This contract enables the creation, issuance, and transfering of many different tokens.
    *
    */
    void currency::on( const create& c ) {
       require_auth( c.issuer );
       create_currency( c );
    }

    void currency::on( const issue& i ) {
       auto sym = i.quantity.symbol.name();
       stats statstable( _contract, sym );
       const auto& st = statstable.get( sym );

       require_auth( st.issuer );
       eosio_assert( i.quantity.is_valid(), "invalid quantity" );
       eosio_assert( i.quantity.amount > 0, "must issue positive quantity" );

       statstable.modify( st, 0, [&]( auto& s ) {
          s.supply.amount += i.quantity.amount;
       });

       add_balance( st.issuer, i.quantity, st, st.issuer );

       if( i.to != st.issuer )
       {
          inline_transfer( st.issuer, i.to, i.quantity, i.memo );
       }
    }

    void currency::on( const transfer& t ) {
        if(has_auth(t.from)){
          require_auth(t.from);
        }
        else
        {
            eosio_assert( false, "insufficient authority" );
        }
       auto sym = t.quantity.symbol.name();
       stats statstable( _contract, sym );
       const auto& st = statstable.get( sym );

       require_recipient( t.to );

       eosio_assert( t.quantity.is_valid(), "invalid quantity" );
       eosio_assert( t.quantity.amount > 0, "must transfer positive quantity" );
       sub_balance( t.from, t.quantity, st );
       add_balance( t.to, t.quantity, st, t.from );
    }

    void currency::sub_balance( account_name owner, asset value, const currency_stats& st ) {
       accounts from_acnts( _contract, owner );

       const auto& from = from_acnts.get( value.symbol );
       eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

       eosio_assert( !st.can_freeze || !from.frozen, "account is frozen by issuer" );
       eosio_assert( !st.can_freeze || !st.is_frozen, "all transfers are frozen by issuer" );
       eosio_assert( !st.enforce_whitelist || from.whitelist, "account is not white listed" );

       from_acnts.modify( from, 0, [&]( auto& a ) {
           a.balance.amount -= value.amount;
       });
    }

    void currency::add_balance( account_name owner, asset value, const currency_stats& st, account_name ram_payer )
    {
       accounts to_acnts( _contract, owner );
       auto to = to_acnts.find( value.symbol );
       if( to == to_acnts.end() ) {
          eosio_assert( !st.enforce_whitelist, "can only transfer to white listed accounts" );
          to_acnts.emplace( ram_payer, [&]( auto& a ){
            a.balance = value;
          });
       } else {
          eosio_assert( !st.enforce_whitelist || to->whitelist, "receiver requires whitelist by issuer" );
          to_acnts.modify( to, 0, [&]( auto& a ) {
            a.balance.amount += value.amount;
          });
       }
    }


    void currency::approve(const approveact & _approveobj){
        require_auth(_approveobj.owner);
        account_name owner=_approveobj.owner;
        account_name spender=_approveobj.spender;
        asset quantity=_approveobj.quantity;

        approves approveobj(_contract, owner);
        if(approveobj.find(quantity.symbol.value) != approveobj.end()){
            const auto &approSymIte = approveobj.get(quantity.symbol.value);

            approveobj.modify(approSymIte, owner, [&](auto &a){
                auto approvetoPairIte = a.approved.begin();
                while(approvetoPairIte != a.approved.end()){
                    if(approvetoPairIte->to == spender){
                        approvetoPairIte->value = quantity.amount;
                        break;
                    }
                    approvetoPairIte++;
                }
                if(approvetoPairIte == a.approved.end()){
                    approvetoPair atp;
                    atp.to = spender;
                    atp.value = quantity.amount;
                    a.approved.push_back(atp);
                }
            });
        }else{
            approvetoPair atp;
            atp.to = spender;
            atp.value = quantity.amount;

            approveto at;
            at.approved.push_back(atp);
            at.symbol_name=quantity.symbol.value;

            approveobj.emplace(owner, [&](auto &a){
                a.symbol_name = quantity.symbol.value;
                a.approved = at.approved;
            });
        }
    }

    void currency::balanceOf(const balanceOfAct & boa){
        symbol_name sn = string_to_symbol(4, boa.symbol.c_str());
        print("balanceOf[",boa.symbol.c_str(),"]", get_balance(boa.owner, sn));
    }

    void currency::allowanceOf(const allowanceOfAct & aof){
        approves approveobj(_contract, aof.owner);

        if(approveobj.find(string_to_symbol(4, aof.symbol.c_str())) != approveobj.end()){
            const auto &approSymIte = approveobj.get(string_to_symbol(4, aof.symbol.c_str()));
            auto approvetoPairIte = approSymIte.approved.begin();
            while(approvetoPairIte != approSymIte.approved.end()){

                if(approvetoPairIte->to == aof.spender){
                    print("allowanceOf[", account_name(approvetoPairIte->to), "]=", approvetoPairIte->value);
                    break;
                }
                approvetoPairIte++;
            }
            if(approvetoPairIte == approSymIte.approved.end()){
                print("allowanceOf[", account_name(aof.spender), "]=", 0);
            }

        }else{
            print("allowanceOf[", (account_name)aof.spender, "]=", 0);
        }
    }

    void currency::transferFrom(const transferfromact& tfa){
        require_auth(tfa.to);

        account_name owner=tfa.from;
        account_name spender=tfa.to;
        asset quantity=tfa.quantity;

        approves approveobj(_contract, tfa.from);
        if(approveobj.find(quantity.symbol.value) != approveobj.end()){
            const auto &approSymIte = approveobj.get(quantity.symbol.value);
            approveobj.modify(approSymIte, owner, [&](auto &a){
                a = approSymIte;
                auto approvetoPairIte = a.approved.begin();
                while(approvetoPairIte != a.approved.end()){

                    if(approvetoPairIte->to == spender){
                        eosio_assert(approvetoPairIte->value>=tfa.quantity.amount, NOT_ENOUGH_ALLOWED);
                        approvetoPairIte->value -= quantity.amount;

                        auto sym = tfa.quantity.symbol.name();
                        stats statstable( _contract, sym );
                        const auto& st = statstable.get( sym );
                        require_recipient( tfa.to );

                        eosio_assert( tfa.quantity.is_valid(), "invalid quantity" );
                        eosio_assert( tfa.quantity.amount > 0, "must transfer positive quantity" );
                        sub_balance( tfa.from, tfa.quantity, st );
                        add_balance( tfa.to, tfa.quantity, st, tfa.to );
                        break;
                    }
                    approvetoPairIte++;
                }
                if(approvetoPairIte == a.approved.end()){
                    eosio_assert(false, NOT_ENOUGH_ALLOWED);
                }
            });
        }else{
            eosio_assert(false, NOT_ENOUGH_ALLOWED);
        }
    }

    void currency::create_currency( const create& c ) {
      auto sym = c.maximum_supply.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );

       stats statstable( _contract, sym.name() );
       auto existing = statstable.find( sym.name() );
       eosio_assert( existing == statstable.end(), "token with symbol already exists" );

       statstable.emplace( c.issuer, [&]( auto& s ) {
          s.supply.symbol = c.maximum_supply.symbol;
          s.max_supply    = c.maximum_supply;
          s.issuer        = c.issuer;
          s.can_freeze    = c.issuer_can_freeze;
          s.can_recall    = c.issuer_can_recall;
          s.can_whitelist = c.issuer_can_whitelist;
       });
    }

    bool currency::apply( account_name contract, action_name act ) {
       if( contract != _contract )
          return false;

       switch( act ) {
          case N(issue):
            on( unpack_action_data<issue>() );
            return true;
          case N(transfer):
            on( unpack_action_data<transfer>() );
            return true;
          case N(create):
            on( unpack_action_data<create>() );
            return true;
          case N(approve):
            approve(unpack_action_data<approveact>());
            return true;
          case N(transferfrom):
            transferFrom(unpack_action_data<transferfromact>());
            return true;
          case N(balanceof):
            balanceOf(unpack_action_data<balanceOfAct>());
            return true;
          case N(allowanceof):
           allowanceOf(unpack_action_data<allowanceOfAct>());
           return true;
       }
       return false;
    }
}
extern "C" {
    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
       oct::currency(receiver).apply( code, action );
    }
}
