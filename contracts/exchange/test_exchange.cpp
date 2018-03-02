#include <map>
#include <exception>
#include <string>
#include <cstdint>
#include <iostream>

using namespace std;

typedef __uint128_t uint128_t;
typedef string  account_name;
typedef string  symbol_type;

static const symbol_type exchange_symbol = "EXC";

struct asset {
   int64_t     amount;
   symbol_type symbol;
};

struct exchange_state;
struct connector {
   uint32_t weight;
   asset    balance;

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

struct exchange_state {
   int64_t    supply;
   uint64_t   total_weight = 1000*1000;

   connector  base;
   connector  quote;

   map<balance_key, int64_t> output; 
};

asset connector::convert_to_exchange( exchange_state& ex, const asset& input ) {
   uint64_t relevant_ex_shares =  (ex.supply * weight) / ex.total_weight;

   double init_ex_per_in = double(balance.amount) / relevant_ex_shares;
 //  std::cerr << "init_ex_per_in: "  << init_ex_per_in << "\n";
   double init_out       = init_ex_per_in * input.amount;
 //  std::cerr << "init out: "  << init_out << "\n";


   double final_ex_per_in = double(balance.amount + input.amount) / (relevant_ex_shares+init_out);
   double final_out = final_ex_per_in * input.amount;

   /*
   std::cerr << "input: "  << input.amount << " " << input.symbol << "\n";
   std::cerr << "final_ex_per_in: "  << final_ex_per_in << "\n";
   std::cerr << "final out: "  << final_out << "\n";
   */

   ex.supply      += int64_t(final_out);
   balance.amount += input.amount;

   return asset{ int64_t(final_out), exchange_symbol };
}

asset connector::convert_from_exchange( exchange_state& ex, const asset& input ) {

   uint64_t relevant_ex_shares =  (ex.supply * weight) / ex.total_weight;

   double init_in_per_out = relevant_ex_shares / double(balance.amount);
   double init_out   = init_in_per_out * input.amount;

   double final_in_per_out = (relevant_ex_shares-input.amount) / (balance.amount-init_out);
   int64_t final_out   = final_in_per_out * input.amount;

   ex.supply      -= input.amount;
   balance.amount -= final_out;

   return asset{ final_out, balance.symbol };
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
                        asset        min_output ) {

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

  /*
  if( min_output.symbol != initial_output.symbol ) {
     if( min_output.symbol == result.base.balance.symbol ) {
        final_output = result.base.convert_from_exchange( result, initial_output );
     }
     else if( min_output.symbol == result.quote.balance.symbol ) {
        final_output = result.quote.convert_from_exchange( result, initial_output );
     }
     else eosio_assert( false, "invalid symbol" );
  }

  //std::cerr << "output: " << final_output.amount << " " << final_output.symbol << "\n";
  */


  std::cerr << "\n\nconvert " << input.amount << " "<< input.symbol << "  =>  " << final_output.amount << " " << final_output.symbol << "  final: " << min_output.symbol << " \n";

  result.output[ balance_key{user,final_output.symbol} ] += final_output.amount;
  result.output[ balance_key{user,input.symbol} ] -= input.amount;

  print_state( result );


  if( min_output.symbol != final_output.symbol ) {
    return convert( result, user, final_output, min_output );
  }


  return result;
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

   exchange_state state;
   state.supply = 1000000;
   state.total_weight = 1000000;
   state.base.weight  = state.total_weight / 2;
   state.base.balance.amount = 10000;
   state.base.balance.symbol = "USD";
   state.quote.weight = state.total_weight / 2;
   state.quote.balance.amount = 10000;
   state.quote.balance.symbol = "BTC";

   print_state( state );

   auto new_state = convert( state, "dan", asset{ 500, "USD"}, asset{ 0, "BTC" } );
   //     new_state = convert( new_state, "dan", asset{ 10, "EXC"}, asset{ 0, "USD" } );
   //     new_state = convert( state, "dan", asset{ 500, "BTC"}, asset{ 0, "EXC" } );
   //     new_state = convert( new_state, "dan", asset{ 10, "EXC"}, asset{ 0, "USD" } );
   //

        new_state = convert( state, "dan", asset{ 500, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 500, "USD"}, asset{ 0, "BTC" } );
        new_state = convert( new_state, "dan", asset{ 526, "BTC"}, asset{ 0, "USD" } );
        new_state = convert( new_state, "dan", asset{ 526, "BTC"}, asset{ 0, "USD" } );
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
