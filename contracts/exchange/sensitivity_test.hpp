#include <string>

using namespace std;
typedef string account_name;
typedef long double real_type;
static const string exchange_symbol = "EXC";
struct asset {
    double amount;
    string symbol;
};

struct exchange_state;

struct connector {
    asset balance;
    long double weight = 0.5;
    double total_lent; /// lent from maker to users
    double total_borrowed; /// borrowed from users to maker
    double total_available_to_lend; /// amount available to borrow
    double interest_pool; /// total interest earned but not claimed,
    /// each user can claim user_lent

    void borrow(exchange_state &ex, const asset &amount_to_borrow);

    asset convert_to_exchange(exchange_state &ex, const asset &input);

    asset convert_from_exchange(exchange_state &ex, const asset &input);
};

struct margin {
    asset lent;
    string collateral_symbol;
    real_type least_collateralized_rate;
};

struct balance_key {
    account_name owner;
    string symbol;

    friend bool operator<(const balance_key &a, const balance_key &b) {
        return std::tie(a.owner, a.symbol) < std::tie(b.owner, b.symbol);
    }

    friend bool operator==(const balance_key &a, const balance_key &b) {
        return std::tie(a.owner, a.symbol) == std::tie(b.owner, b.symbol);
    }
};

struct exchange_state {
    double supply;
    string symbol = exchange_symbol;
    map<balance_key, double> output;
    vector<margin> margins;
    connector base;
    connector quote;


    void transfer(account_name user, asset q) {
        output[balance_key{user, q.symbol}] += q.amount;
    }

};