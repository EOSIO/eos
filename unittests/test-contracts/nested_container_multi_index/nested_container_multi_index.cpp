/* Verify the support of nested containers in eosio multi-index table
 * For each action, an example regarding how to use the action with the cleos command line is given.
 *
 * std:pair<T1,T2> is a struct with 2 fields first and second,
 * std::map<K,V> is handled as an array/vector of pairs/structs by EOSIO with implicit fields key, value,
 * the cases of combined use of key/value and first/second involving map,pair in the cleos are documented here.
 * so handling of std::pair is NOT the same as the handling of a general struct such as struct mystruct!
 *
 * When assigning data input with cleos:
 *      [] represents an empty vector<T>/set<T> or empty map<T1,T2> where T, T1, T2 can be any composite types
 *      null represents an uninitialized std::optional<T> where T can be any composite type
 *      BUT [] or null can NOT be used to represent an empty struct or empty std::pair
 *
 * Expected printout:
 *      For each setx action, the printed result on the cleos console is given in its corresponding prntx action.
 */

#include <eosio/eosio.hpp>

#include <vector>
#include <set>
#include <optional>
#include <map>
#include <tuple>

using namespace eosio;
using namespace std;

#define  SETCONTAINERVAL(x) do { \
        require_auth(user); \
        psninfoindex2 tblIndex(get_self(), get_first_receiver().value); \
        auto iter = tblIndex.find(user.value); \
        if (iter == tblIndex.end()) \
        { \
            tblIndex.emplace(user, [&](auto &row) { \
                        row.key = user; \
                        row.x = x; \
                    }); \
        } \
        else \
        { \
            tblIndex.modify(iter, user, [&]( auto& row ) { \
                        row.x = x; \
                    }); \
        } \
    }while(0)

struct mystruct
{
    uint64_t   _count;
    string     _strID;
};


typedef optional< uint16_t > op_uint16;
typedef vector< op_uint16 > vec_op_unit16;
typedef optional< mystruct > op_struc;

class [[eosio::contract("nested_container_multi_index")]] nestcontnmi : public eosio::contract {
    private:
        struct [[eosio::table]] person2 {
            name key;

            //  Each container/object is represented by one letter: v-vector, m-map, s-mystruct,o-optional, p-pair,
            //  with exceptions:     s2 - mystruct2,    st - set
            vector< op_uint16 > vo;
            set< op_uint16 > sto;
            vector< op_struc > vos;
            pair<uint16_t, vec_op_unit16> pvo;

            uint64_t primary_key() const { return key.value; }
        };
        using psninfoindex2 = eosio::multi_index<"people2"_n, person2>;


    public:
        using contract::contract;

        nestcontnmi(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}

        [[eosio::action]]
        void setvo(name user, const vector<op_uint16>& vo)
        {
            SETCONTAINERVAL(vo);
            eosio::print("vector<optional<uint16_t>> stored successfully");
        }

        [[eosio::action]]
        void setsto(name user, const set<op_uint16> & sto)
        {
            SETCONTAINERVAL(sto);
            eosio::print("set<optional<uint16_t>> stored successfully");
        }

        [[eosio::action]]
        void setvos(name user, const vector<op_struc>& vos)
        {
            SETCONTAINERVAL(vos);
            eosio::print("vector<optional<mystruct>> stored successfully");
        }

        [[eosio::action]]
        void setpvo(name user, const pair<uint16_t, vec_op_unit16>& pvo)
        {
            SETCONTAINERVAL(pvo);
            eosio::print("pair<uint16_t, vector<optional<uint16_t>>> stored successfully");
        }

};