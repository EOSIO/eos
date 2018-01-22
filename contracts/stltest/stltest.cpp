/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <stltest/stltest.hpp> /// defines transfer struct (abi)

#include <array>
#include <vector>
#include <stack>
#include <queue>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <string>

#include <eoslib/eos.hpp>

#include <eoslib/token.hpp>
#include <eoslib/dispatcher.hpp>

using namespace eosio;

namespace stltest {

    class contract {
    public:
        typedef eosio::token<N(mycurrency),S(4,MYCUR)> token_type;
        static const uint64_t code                = token_type::code;
        static const uint64_t symbol              = token_type::symbol;
        static const uint64_t sent_table_name = N(sent);
        static const uint64_t received_table_name = N(received);

        struct message {
            account_name from;
            account_name to;
            string msg;

            static uint64_t get_account() { return current_receiver(); }
            static uint64_t get_name()  { return N(message); }

            template<typename DataStream>
            friend DataStream& operator << ( DataStream& ds, const message& m ){
                return ds << m.from << m.to << m.msg;
            }
            template<typename DataStream>
            friend DataStream& operator >> ( DataStream& ds, message& m ){
                return ds >> m.from >> m.to >> m.msg;
            }
        };

        static void on(const message& msg) {
           print("STL test start.");
           std::array<uint32_t, 10> arr;
           arr[0] = 0;
           std::vector<uint32_t> v;
           v.push_back(0);
           std::stack<char> stack;
           stack.push('J');
           std::queue<unsigned int> q;
           q.push(0);
           std::deque<float> dq;
           dq.push_front(0.0f);
           std::list<uint32_t> l;
           l.push_back(0);
           std::string s;
           s.append(1, 'a');
           std::map<int, double> m;
           m.emplace(0, 1.0);
           std::set<long> st;
           st.insert(0);
           std::unordered_map<int, string> hm;
           //hm[0] = "abc";
           std::unordered_set<int> hs;
           //hs.insert(0);
           print("STL test done.");
        }

        static void apply( account_name c, action_name act) {
            eosio::dispatch<stltest::contract, message>(c,act);
        }
    };

} /// namespace eosio


extern "C" {
/// The apply method implements the dispatch of events to this contract
void apply( uint64_t code, uint64_t action ) {
    stltest::contract::apply( code, action );
}
}
