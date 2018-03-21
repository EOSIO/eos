#include <map>
#include <exception>
#include <string>
#include <cstdint>
#include <iostream>
#include <math.h>
#include <fc/exception/exception.hpp>
/*
#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/rational.hpp>

#include <sg14/fixed_point>
#include "fixed.hpp"
*/

#include <fc/time.hpp>
#include <fc/log/logger.hpp>

#include <boost/fusion/functional/invocation/invoke.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/include/std_tuple.hpp>



//#include "bfp/lib/posit.h"

using namespace std;

typedef long double real_type;
typedef double token_type;


   template<typename T, typename... Args>
   void call( T* obj, void (T::*func)(Args...)  ) {
      std::tuple<Args...> args;
      auto f2 = [&]( auto... a ){  (obj->*func)( a... ); };
      boost::fusion::invoke( f2, args );
   }

   struct test {
      void run( int a, char b ) {
         cout << a << " " << b << "\n";
      }
   };


/*
struct margin_position {
   account_name owner;
   uint64_t     exchange_id;
   asset        lent;
   asset        collateral;
   uint64_t     open_time;

   uint64_t     primary_key()const{ return owner; }
   uint256_t    by_owner_ex_lent_collateral()const {

   }

   real_type    by_call_price()const {
      return collateral.amount / real_type(lent.amount);
   }
};
*/



template<typename Real>
Real Abs(Real Nbr)
{
 if( Nbr >= 0 )
  return Nbr;
 else
  return -Nbr;
}

template<typename Real>
Real sqrt_safe( const Real Nbr)
{
  return sqrt(Nbr);
// cout << " " << Nbr << "\n";;
 Real Number = Nbr / Real(2.0);
 const Real Tolerance = Real(double(1.0e-12));
 //cout << "tol: " << Tolerance << "\n";

 Real Sq;
 Real Er;
 do {
  auto tmp = Nbr / Number;
  tmp += Number;
  tmp /= real_type(2.0);
  if( Number == tmp ) break;
  Number = tmp;
  Sq = Number * Number;
  Er = Abs(Sq - Nbr);
// wdump((Er.getDouble())(1.0e-8)(Tolerance.getDouble()));
//  wdump(((Er - Tolerance).getDouble()));
 }while( Er >= Tolerance );

 return Number;
}

typedef __uint128_t uint128_t;
typedef string  account_name;
typedef string  symbol_type;

static const symbol_type exchange_symbol = "EXC";

struct asset {
   token_type amount;
   symbol_type symbol;
};

struct margin_key {
   symbol_type lent;
   symbol_type collat;
};

struct margin {
   asset       lent;
   symbol_type collateral_symbol;
   real_type   least_collateralized_rate;
};

struct user_margin {
   asset lent;
   asset collateral;

   real_type call_price()const {
      return collateral.amount / real_type(lent.amount);
   }
};

struct exchange_state;
struct connector {
   asset      balance;
   real_type  weight = 0.5;
   token_type total_lent; /// lent from maker to users
   token_type total_borrowed; /// borrowed from users to maker
   token_type total_available_to_lend; /// amount available to borrow
   token_type interest_pool; /// total interest earned but not claimed,
                             /// each user can claim user_lent 

   void  borrow( exchange_state& ex, const asset& amount_to_borrow );
   asset convert_to_exchange( exchange_state& ex, const asset& input );
   asset convert_from_exchange( exchange_state& ex, const asset& input );
};


struct balance_key {
   account_name owner;
   symbol_type  symbol;

   friend bool operator < ( const balance_key& a, const balance_key& b ) {
      return std::tie( a.owner, a.symbol ) < std::tie( b.owner, b.symbol );
   }
   friend bool operator == ( const balance_key& a, const balance_key& b ) {
      return std::tie( a.owner, a.symbol ) == std::tie( b.owner, b.symbol );
   }
};

real_type fee = 1;//.9995;


int64_t maxtrade = 20000ll;

struct exchange_state {
   token_type  supply;
   symbol_type symbol = exchange_symbol;

   connector  base;
   connector  quote;

   void transfer( account_name user, asset q ) {
      output[balance_key{user,q.symbol}] += q.amount;
   }
   map<balance_key, token_type> output; 
   vector<margin>               margins;
};

/*
void  connector::borrow( exchange_state& ex, account_name user, 
                         asset amount_to_borrow, 
                         asset collateral,
                         user_margin& marg ) {
   FC_ASSERT( amount_to_borrow.amount < balance.amount, "attempt to borrow too much" );
   lent.amount += amount_to_borrow.amount;
   balance.amount -= amount_to_borrow.amount;
   ex.transfer( user, amount_to_borrow ); 

   marg.collateral.amount += collateral.amount;
   marg.lent.amount += amount_to_borrow.amount;
   auto p = marg.price();

   if( collateral.symbol == ex.symbol ) {
      if( p > ex_margin.least_collateralized_rate )
         ex_margin.least_collateralized_rate = p;
   }
   else if( collateral.symbol == peer_margin.collateral.symbol ) {
      if( p > peer_margin.least_collateralized_rate )
         peer_margin.least_collateralized_rate = p;
   }
}
*/

asset connector::convert_to_exchange( exchange_state& ex, const asset& input ) {

   real_type R(ex.supply);
   real_type S(balance.amount+input.amount);
   real_type F(weight);
   real_type T(input.amount);
   real_type ONE(1.0);

   auto E = R * (ONE - std::pow( ONE + T / S, F) );


   //auto real_issued = real_type(ex.supply) * (sqrt_safe( 1.0 + (real_type(input.amount) / (balance.amount+input.amount))) - 1.0);
   //auto real_issued = real_type(ex.supply) * (std::pow( 1.0 + (real_type(input.amount) / (balance.amount+input.amount)), weight) - real_type(1.0));
   //auto real_issued = R * (std::pow( ONE + (T / S), F) - ONE);

   //wdump((double(E))(double(real_issued)));
   token_type issued = -E; //real_issued;
   

   ex.supply      += issued;
   balance.amount += input.amount;

   return asset{ issued, exchange_symbol };
}

asset connector::convert_from_exchange( exchange_state& ex, const asset& input ) {

   real_type R(ex.supply - input.amount);
   real_type S(balance.amount);
   real_type F(weight);
   real_type E(input.amount);
   real_type ONE(1.0);

   real_type T = S * (std::pow( ONE + E/R, ONE/F) - ONE);


   /*
   real_type base = real_type(1.0) + ( real_type(input.amount) / real_type(ex.supply-input.amount));
   auto out = (balance.amount * ( std::pow(base,1.0/weight) - real_type(1.0) ));
   */
   auto out = T;

//   edump((double(out-T))(double(out))(double(T)));

   ex.supply -= input.amount;
   balance.amount -= token_type(out);
   return asset{ token_type(out), balance.symbol };
}


void eosio_assert( bool test, const string& msg ) {
   if( !test ) throw std::runtime_error( msg );
}

void print_state( const exchange_state& e );



/**
 *  Given the current state, calculate the new state
 */
exchange_state convert( const exchange_state& current,
                        account_name user,
                        asset        input,
                        asset        min_output,
                        asset*       out = nullptr) {

  eosio_assert( min_output.symbol != input.symbol, "cannot convert" );

  exchange_state result(current);

  asset initial_output = input;

  if( input.symbol != exchange_symbol ) {
     if( input.symbol == result.base.balance.symbol ) {
        initial_output = result.base.convert_to_exchange( result, input );
     }
     else if( input.symbol == result.quote.balance.symbol ) {
        initial_output = result.quote.convert_to_exchange( result, input );
     }
     else eosio_assert( false, "invalid symbol" );
  } else {
     if( min_output.symbol == result.base.balance.symbol ) {
        initial_output = result.base.convert_from_exchange( result, initial_output );
     }
     else if( min_output.symbol == result.quote.balance.symbol ) {
        initial_output= result.quote.convert_from_exchange( result, initial_output );
     }
     else eosio_assert( false, "invalid symbol" );
  }



  asset final_output = initial_output;

//  std::cerr << "\n\nconvert " << input.amount << " "<< input.symbol << "  =>  " << final_output.amount << " " << final_output.symbol << "  final: " << min_output.symbol << " \n";

  result.output[ balance_key{user,final_output.symbol} ] += final_output.amount;
  result.output[ balance_key{user,input.symbol} ] -= input.amount;

  if( min_output.symbol != final_output.symbol ) {
    return convert( result, user, final_output, min_output, out );
  }

  if( out ) *out = final_output;
  return result;
}

/*  VALIDATE MARGIN ALGORITHM
 *
 *  Given an initial condition, verify that all margin positions can be filled. 
 *
 *  Assume 3 assets, B, Q, and X and the notation LENT-COLLAT we get the following
 *  pairs:
 *
 *  B-X
 *  B-A
 *  A-X
 *  A-B
 *  X-A
 *  X-B
 *  
 *  We assume that pairs of the same lent-type have to be simultainously filled,
 *  as filling one could make it impossible to fill the other.  
 *
 *
void validate_margin( exchange_state& e ) {
   for( const auto& pos : e.margins ) {
      token_type min_collat = pos.lent.amount * pos.least_collateralized_rate;
      asset received;
      e = convert( e, "user", asset{ min_collat, pos.first.collat }, pos.lent, &received );
      FC_ASSERT( received > pos.lent.amount, "insufficient collateral" );

      received.amount -= pos.lent.amount;
      e = convert( e, "user", received, asset{ token_type(0), pos.collateral_symbol} );
   }
}
*/





/**
 *  A user has Collateral C and wishes to borrow B, so we give user B 
 *  provided that C is enough to buy B back after removing it from market and
 *  that no margin calls would be triggered.  
 */
exchange_state borrow( const exchange_state& current, account_name user,
                       asset amount_to_borrow,
                       asset collateral_provided ) {
    FC_ASSERT( amount_to_borrow.symbol != collateral_provided.symbol );

    /// lookup the margin position for user
    ///   update user's margin position
    ///   update least collateralized margin position on state
    ///   remove amount_to_borrow from exchange 
    ///   lock collateral for user 
    /// simulate complete margin calls 
}

exchange_state cover( const exchange_state& current, account_name user,
                      asset amount_to_cover, asset collateral_to_cover_with )
{
   /// lookup existing position for user/debt/collat
   /// verify collat > collateral_to_cover_with
   /// sell collateral_to_cover_with for debt on market
   /// reduce debt by proceeds
   /// add proceeds to connector
   //     - if borrowed from user, reduce borrowed from user
   /// calculate new call price and update least collateralized position
   /// simulate complete margin calls

}

exchange_state lend( const exchange_state& current, account_name lender,
                     asset asset_to_lend ) {
   /// add to pool of funds available for lending and buy SHARES in
   ///  interest pool at current rate.  

}

exchange_state unlend( const exchange_state& current, account_name lender,
                     asset asset_to_lend ) {
    /// sell shares in interest pool at current rate
    /// this is permitable so long as total borrowed from users remains less than
    /// total available to lend. Otherwise, margin is called on the least 
    /// collateralized position. 
}



void print_state( const exchange_state& e ) {
   std::cerr << "\n-----------------------------\n";
   std::cerr << "supply: " <<  e.supply  << "\n";
   std::cerr << "base: " <<  e.base.balance.amount << " " << e.base.balance.symbol << "\n";
   std::cerr << "quote: " <<  e.quote.balance.amount << " " << e.quote.balance.symbol << "\n";

   for( const auto& item : e.output ) {
      cerr << item.first.owner << "  " << item.second << " " << item.first.symbol << "\n";
   }
   std::cerr << "\n-----------------------------\n";
}


int main( int argc, char** argv ) {
 //  std::cerr << "root: " << double(root.numerator())/root.denominator() << "\n";

   test t;
   t.run(3, 'a');
   call( &t, &test::run );

   return 0;

   exchange_state state;
   state.supply = 100000000000ll;
   //state.base.weight  = state.total_weight / 2.;
   state.base.balance.amount = 100000000;
   state.base.balance.symbol = "USD";
   state.base.weight = .49;
   //state.quote.weight = state.total_weight / 2.;
   state.quote.balance.amount = state.base.balance.amount;
   state.quote.balance.symbol = "BTC";
   state.quote.weight = .51;

   print_state( state );

   //state = convert( state, "dan", asset{ 100, "USD"}, asset{ 0, "BTC" } );

   auto start = fc::time_point::now();
   for( uint32_t i = 0; i < 10000; ++i ) {
     if( rand() % 2 == 0 )
        state = convert( state, "dan", asset{ token_type(uint32_t(rand())%maxtrade), "USD"}, asset{ 0, "BTC" } );
     else
        state = convert( state, "dan", asset{ token_type(uint32_t(rand())%maxtrade), "BTC"}, asset{ 0, "USD" } );
   }
   for( const auto& item : state.output ) {
      if( item.second > 0 ) {
         if( item.first.symbol == "USD" )
           state = convert( state, "dan", asset{ item.second, item.first.symbol}, asset{ 0, "BTC" } );
         else
           state = convert( state, "dan", asset{ item.second, item.first.symbol}, asset{ 0, "USD" } );
        break;
      }
   }
   print_state( state );

   auto end = fc::time_point::now();
   wdump((end-start));
   /*
   auto new_state = convert( state, "dan", asset{ 100, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 100, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 100, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 100, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 100, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 100, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 100, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 100, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 100, "USD"}, asset{ 0, "BTC" } );

        new_state = convert( new_state, "dan", asset{ 100, "BTC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 100, "BTC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 100, "BTC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 100, "BTC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 100, "BTC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 100, "BTC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 100, "BTC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 92.5-0.08-.53, "BTC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 100, "BTC"}, asset{ 0, "USD" } );
        */

        //new_state = convert( new_state, "dan", asset{ 442+487-733+280+349+4.493+62.9, "BTC"}, asset{ 0, "USD" } );
   /*
   auto new_state = convert( state, "dan", asset{ 500, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 500, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 442+487, "BTC"}, asset{ 0, "USD" } );
        */
        /*
        new_state = convert( new_state, "dan", asset{ 487, "BTC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 442, "BTC"}, asset{ 0, "USD" } );
        */
        //new_state = convert( new_state, "dan", asset{ 526, "BTC"}, asset{ 0, "USD" } );
        //new_state = convert( new_state, "dan", asset{ 558, "BTC"}, asset{ 0, "USD" } );
        //new_state = convert( new_state, "dan", asset{ 1746, "BTC"}, asset{ 0, "USD" } );
        /*
        new_state = convert( new_state, "dan", asset{ 526, "BTC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 500, "USD"}, asset{ 0, "EXC" } );
        new_state = convert( new_state, "dan", asset{ 500, "BTC"}, asset{ 0, "EXC" } );
        new_state = convert( new_state, "dan", asset{ 10, "EXC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 10, "EXC"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 500, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 500, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 500, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 500, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 2613, "BTC"}, asset{ 0, "USD" } );
        */



   /*
   auto new_state = convert( state, "dan", asset{ 10, "EXC"}, asset{ 0, "USD" } );

   print_state( new_state );

   new_state = convert( state, "dan", asset{ 10, "EXC"}, asset{ 0, "BTC" } );
   print_state( new_state );
   new_state = convert( new_state, "dan", asset{ 10, "EXC"}, asset{ 0, "USD" } );
   print_state( new_state );


   //new_state = convert( new_state, "dan", asset{ 52, "USD"}, asset{ 0, "EXC" } );
   */

   return 0;
}



#if 0

0. if( margin_fault )
     Convert Least Collateral
     if( margin fault ))
        defer

if( margin_fault ) assert( false, "busy calling" );

1. Fill Incoming Order
2. Check Counter Order  
3. if( margin fault )
   Defer Trx to finish margin call


#endif 
