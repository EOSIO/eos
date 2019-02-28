/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
// include entire libc
#include <alloca.h>
#include <assert.h>
#include <byteswap.h>
#include <crypt.h>
#include <ctype.h>
#include <endian.h>
#include <errno.h>
#include <features.h>
#include <inttypes.h>
#include <iso646.h>
#include <limits.h>
#include <locale.h>
#include <malloc.h>
#include <math.h>
#include <memory.h>
#include <monetary.h>
#include <search.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdc-predef.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <strings.h>
#include <uchar.h>
#include <unistd.h>
#include <values.h>
#include <wchar.h>
#include <wctype.h>

//include entire libstdc++
#include<algorithm>
#include<any>
#include<array>
#include<bitset>
#include<cassert>
//include<ccomplex>
#include<cctype>
#include<cerrno>
//include<cfenv>
#include<cfloat>
//include<chrono>
#include<cinttypes>
#include<ciso646>
#include<climits>
#include<clocale>
#include<cmath>
#include<codecvt>
//include<complex>
#include<condition_variable>
//include<csetjmp>
//include<csignal>
#include<cstdarg>
#include<cstdbool>
#include<cstddef>
#include<cstdint>
#include<cstdio>
#include<cstdlib>
#include<cstring>
//include<ctgmath>
#include<ctime>
#include<cwchar>
#include<cwctype>
#include<deque>
#include<exception>
#include<forward_list>
//include<fstream>
#include<functional>
//include<future>
#include<initializer_list>
#include<iomanip>
#include<ios>
#include<iosfwd>
#include<iostream>
#include<istream>
#include<iterator>
#include<limits>
#include<list>
#include<locale>
#include<map>
#include<memory>
#include<mutex>
#include<new>
#include<numeric>
#include<optional>
#include<ostream>
#include<queue>
//include<random>
#include<ratio>
#include<regex>
#include<scoped_allocator>
#include<set>
//include<shared_mutex>
#include<sstream>
#include<stack>
#include<stdexcept>
#include<streambuf>
#include<string>
#include<string_view>
#include<strstream>
#include<system_error>
//include<thread>
#include<tuple>
#include<type_traits>
#include<typeindex>
#include<typeinfo>
#include<unordered_map>
#include<unordered_set>
#include<utility>
#include<valarray>
#include<variant>
#include<vector>


/*
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
#include <stdexcept>
*/
//include <eosiolib/eos.hpp>
#include <eosiolib/dispatcher.hpp>

using namespace eosio;
/*
namespace std {
   extern ios_base __start_std_streams;
}
*/
namespace stltest {

   struct MSTR {
      MSTR() : x(7891) {
         prints("ATTENTION! S::S() called\n");
      }
      int x;
      ~MSTR() {
         prints("~MSTR");
      }
   };
    
    class contract {
    public:
        static const uint64_t sent_table_name = N(sent);
        static const uint64_t received_table_name = N(received);

        struct message {
            account_name from;
            account_name to;
           //string msg;

            static uint64_t get_account() { return N(stltest); }
            static uint64_t get_name()  { return N(message); }

            template<typename DataStream>
            friend DataStream& operator << ( DataStream& ds, const message& m ){
               return ds << m.from << m.to;// << m.msg;
            }
            template<typename DataStream>
            friend DataStream& operator >> ( DataStream& ds, message& m ){
               return ds >> m.from >> m.to;// >> m.msg;
            }
        };

       static void f(const char* __restrict, ...) {
          prints("f() called\n");
       }

        static void on( const message& ) {
           /* manual initialization of global variable
           new(&std::__start_std_streams)std::ios_base::Init;
           */
           /*
           std::ostringstream osm;
           osm << "abcdef";
           std::string s = osm.str();
           prints_l(s.data(), s.size());
           */
           /*
           prints("STD string: "); prints(s.c_str());
           prints("\nEOS string: "); prints_l(s2.get_data(), s2.get_size());
           */
           prints("STL test start\n");
           /* doesn't work with WASM::serializeWithInjection
           printf("stdout output\n", 0);
           fprintf(stderr, "stderr output\n", 0);
           */
           void* ptr = malloc(10);
           free(ptr);
           f("abc", 10, 20);

           //auto mptr = new MSTR();
           //delete mptr;

           std::array<uint32_t, 10> arr;
           arr.fill(3);
           arr[0] = 0;

           std::vector<uint32_t> v;
           v.push_back(0);

           std::stack<char> stack;
           stack.push('J');
           stack.pop();

           std::queue<unsigned int> q;
           q.push(0);

           std::deque<long> dq;
           dq.push_front(0.0f);

           std::list<uint32_t> l;
           l.push_back(0);

           std::string s = "abcdef";
           s.append(1, 'g');

           std::map<int, long> m;
           m.emplace(0, 1);
           m.lower_bound(2);

           std::set<long> st;
           st.insert(0);
           st.erase(st.begin());
           st.count(0);

           //std::unordered_map<int, string> hm;
           //hm[0] = "abc";
           //std::unordered_set<int> hs;
           //hs.insert(0);

           sort(dq.begin(), dq.end());
           find_if(l.begin(), l.end(), [](uint32_t f) { return f < 10; });
           prints("STL test done.\n");
           //std::cout << "STL test done." << std::endl;
        }

        static void apply( account_name c, action_name act) {
            eosio::dispatch<stltest::contract, message>(c,act);
        }
    };

} /// namespace eosio


extern "C" {
/// The apply method implements the dispatch of events to this contract
void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
    (void)receiver;
    stltest::contract::apply( code, action );
}
}
