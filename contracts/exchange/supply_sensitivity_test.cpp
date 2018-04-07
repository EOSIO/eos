#include <map>
#include <exception>
#include <string>
#include <cstdint>
#include <iostream>
#include <math.h>
#include <fc/exception/exception.hpp>
#include "sensitivity_test.hpp"
#include <fc/time.hpp>
#include <fc/log/logger.hpp>
/*
 * This file is to simulate the bancor algorithm and
 * plot diagram between amount of token XYZ can be bought with stepped-incremental pegged USD to attempt to convert
 * with different supply amount: 10B, 1B, 100M, 10M and fixed connector weight = 0.5
 */

using namespace std;
typedef long double real_type;

void eosio_assert(bool test, const string &msg) {
    if (!test) throw std::runtime_error(msg);
}

asset connector::convert_from_exchange(exchange_state &ex, const asset &input) {

    real_type DEDUCTED_EXC_AMT(ex.supply - input.amount);
    real_type CONNECTOR_AMT(balance.amount);
    real_type WEIGHT(weight);
    real_type EXC_TO_CONVERT(input.amount);
    real_type ONE(1.0);

    real_type EXC_PAID_OUT = CONNECTOR_AMT * (std::pow(ONE + EXC_TO_CONVERT / DEDUCTED_EXC_AMT, ONE / WEIGHT) - ONE);


    /*
    real_type base = real_type(1.0) + ( real_type(input.amount) / real_type(ex.supply-input.amount));
    auto out = (balance.amount * ( std::pow(base,1.0/weight) - real_type(1.0) ));
    */
    auto out = EXC_PAID_OUT;

//   edump((double(out-T))(double(out))(double(T)));

    ex.supply -= input.amount;
    balance.amount -= double(out);
    return asset{double(out), balance.symbol};
}

asset connector::convert_to_exchange(exchange_state &ex, const asset &input) {
    real_type EXC_AMT(ex.supply);
    real_type ADDED_EXC_AMT(balance.amount + input.amount);
    real_type WEIGHT(weight);
    real_type CONNECTOR_TO_CONVERT(input.amount);
    real_type ONE(1.0);
    auto CONNECTOR_TOKEN_ISSUED = EXC_AMT * (ONE - std::pow(ONE + CONNECTOR_TO_CONVERT / ADDED_EXC_AMT, WEIGHT));
    double issued = -CONNECTOR_TOKEN_ISSUED; //real_issued;
    ex.supply += issued;
    balance.amount += input.amount;
    return asset{issued, exchange_symbol};
}

exchange_state convert(const exchange_state &current,
                       account_name user,
                       asset input,
                       asset min_output,
                       asset *out = nullptr) {

    eosio_assert(min_output.symbol != input.symbol, "cannot convert");

    exchange_state result(current);

    asset initial_output = input;

    if (input.symbol != exchange_symbol) {
        if (input.symbol == result.base.balance.symbol) {
            initial_output = result.base.convert_to_exchange(result, input);
        } else if (input.symbol == result.quote.balance.symbol) {
            initial_output = result.quote.convert_to_exchange(result, input);
        } else eosio_assert(false, "invalid symbol");
    } else {
        if (min_output.symbol == result.base.balance.symbol) {
            initial_output = result.base.convert_from_exchange(result, initial_output);
        } else if (min_output.symbol == result.quote.balance.symbol) {
            initial_output = result.quote.convert_from_exchange(result, initial_output);
        } else eosio_assert(false, "invalid symbol");
    }


    asset final_output = initial_output;

//  std::cerr << "\n\nconvert " << input.amount << " "<< input.symbol << "  =>  " << final_output.amount << " " << final_output.symbol << "  final: " << min_output.symbol << " \n";

    result.output[balance_key{user, final_output.symbol}] += final_output.amount;
    result.output[balance_key{user, input.symbol}] -= input.amount;

    if (min_output.symbol != final_output.symbol) {
        return convert(result, user, final_output, min_output, out);
    }

    if (out) *out = final_output;
    return result;
}

void print_state(const exchange_state &e) {
    std::cerr << "\n-----------------------------\n";
    std::cerr << "supply: " << fixed << e.supply << "\n";
    std::cerr << "base: " << fixed << e.base.balance.amount << " " << e.base.balance.symbol << "\n";
    std::cerr << "quote: " << fixed << e.quote.balance.amount << " " << e.quote.balance.symbol << "\n";

    for (const auto &item : e.output) {
        cerr << item.first.owner << "  " << item.second << " " << item.first.symbol << "\n";
    }
    std::cerr << "\n-----------------------------\n";
}


void print_csf(const exchange_state &e) {
    for (const auto &item : e.output) {
        if (item.first.symbol != "EXC") {
            if (item.second < 0) {
                cerr << fixed <<  -item.second << ", ";
            } else {
                cerr << fixed << item.second << ", ";
            }
        }
    }
    cerr << "\n";
}

void print_supply_price(const exchange_state &e) {
    double quoted_price = e.base.balance.amount / (e.supply * e.base.weight);
    cerr << fixed << e.supply << "," << quoted_price << "\n";
}

void sensitivity_analysis(exchange_state &state) {
    auto start = fc::time_point::now();
    std::cerr << "\n------------" << "state.supply (collateral): " << state.supply << "-----------------\n";
    std::cerr << "USD, XYZ" << "\n";
    for (int i = 0; i < 10; i++) {
        state = convert(state, "dan", asset{1000000, "USD"}, asset{0, "XYZ"}); // convert 1 million USD to XYZ
        print_csf(state);
    }
    auto end = fc::time_point::now();
    wdump((end - start));
}

void collateral_elasticity_test(exchange_state& origin_state){
    exchange_state state = origin_state;
    // 10B
    state.supply = 10000000000ll;
    state.base.balance.amount = 10000000000;
    state.quote.balance.amount = state.base.balance.amount;
    sensitivity_analysis(state);
    // 1B
    state = origin_state;
    state.supply = 1000000000ll;
    state.base.balance.amount = 1000000000;
    state.quote.balance.amount = state.base.balance.amount;
    sensitivity_analysis(state);
    // 100M
    state = origin_state;
    state.supply = 100000000ll;
    state.base.balance.amount = 100000000;
    state.quote.balance.amount = state.base.balance.amount;
    sensitivity_analysis(state);
    // 10M
    state = origin_state;
    state.supply = 10000000ll;
    state.base.balance.amount = 10000000;
    state.quote.balance.amount = state.base.balance.amount;
    sensitivity_analysis(state);
}

int main(int argc, char **argv) {
    //  std::cerr << "root: " << double(root.numerator())/root.denominator() << "\n";
    exchange_state origin_state;
    origin_state.base.weight = 0.5;
    origin_state.quote.weight = 0.5;
    origin_state.base.balance.symbol = "USD";
    origin_state.quote.balance.symbol = "XYZ";
    print_state(origin_state);
    std::cerr << "\n------------" << "origin_state.base.weight="<< origin_state.base.weight << "-----------------\n";
    collateral_elasticity_test(origin_state);

}