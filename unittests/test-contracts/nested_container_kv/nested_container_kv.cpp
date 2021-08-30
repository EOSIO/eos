/* Verify the support of nested containers for the new Key-value table eosio::kv::map
 * For each action, an example regarding how to use the action with the cleos command line is given.
 *
 * std:pair<T1,T2> is a struct with 2 fields first and second,
 * std::map<K,V> is handled as an array/vector of pairs/structs by EOSIO with implicit fields key, value,
 * the cases of combined use of key/value and first/second involving map,pair in the cleos are documented here,
 * so handling of std::pair is NOT the same as the handling of a general struct such as struct mystruct!
 *
 * When assigning data input with cleos:
 *      [] represents an empty vector<T>/set<T> or empty map<T1,T2> where T, T1, T2 can be any composite types
 *      null represents an uninitialized std::optional<T> where T can be any composite type
 *      BUT [] or null can NOT be used to represent an empty struct or empty std::pair
 *
 * Attention:
 *   1) Please follow https://github.com/EOSIO/eos/tree/develop/contracts/enable-kv to enable key-value support of nodeos first
 *   2) To get to the point right away, here authentication/permission checking is skipped for each action in this contract,
 *      however in practice, suitable permission checking such as require_auth(user) has to be used!
 *
 * Expected printout:
 *      For each setx action, the printed result on the cleos console is given in its corresponding prntx action.
 */

#include <eosio/eosio.hpp>
#include <eosio/map.hpp>

#include <vector>
#include <set>
#include <optional>
#include <map>
#include <tuple>

using namespace eosio;
using namespace std;


#define SETCONTAINERVAL(x) do { \
        person2kv obj = mymap[id]; \
        obj.x = x; \
        mymap[id] =  obj; \
    }while(0)

struct mystruct
{
    uint64_t   _count;
    string     _strID;
};

typedef optional< uint16_t > op_uint16;
typedef vector< op_uint16 > vec_op_unit16;
typedef optional< mystruct > op_struc;

struct person2kv {
    vector< op_uint16 > vo;
    set< op_uint16 > sto;
    vector< op_struc > vos;
    pair<uint16_t, vec_op_unit16> pvo;
};

class [[eosio::contract("nested_container_kv")]] nestcontn2kv : public eosio::contract {
    using my_map_t = eosio::kv::map<"people2kv"_n, int, person2kv>;

    private:
        my_map_t mymap{};

    public:
        using contract::contract;

        nestcontn2kv(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}

        [[eosio::action]]
        person2kv get(int id)
        {
            //This action will be used by all prntx actions to retrieve item_found for given id
            auto iter = mymap.find(id);
            check(iter != mymap.end(), "Record does not exist");
            const auto& item_found = iter->second();
            return item_found;
        }

        [[eosio::action]]
        void setvo(int id, const vector<op_uint16>& vo)
        {
            SETCONTAINERVAL(vo);
            eosio::print(" vector< optional<uint16_t> > stored successfully! ");
        }

        [[eosio::action]]
        void setsto(int id, const set<op_uint16>& sto)
        {
            SETCONTAINERVAL(sto);
            eosio::print(" set< optional<uint16_t> > stored successfully! ");
        }

        [[eosio::action]]
        void setvos(int id, const vector<op_struc>& vos)
        {
            SETCONTAINERVAL(vos);
            eosio::print("vector<optional<mystruct>> stored successfully");
        }

        [[eosio::action]]
        void setpvo(int id, const pair<uint16_t, vec_op_unit16>& pvo)
        {
            SETCONTAINERVAL(pvo);
            eosio::print("pair<uint16_t, vector<optional<uint16_t>>> stored successfully");
        }

        using get_action = eosio::action_wrapper<"get"_n, &nestcontn2kv::get>;
        using setvo_action = eosio::action_wrapper<"setvo"_n, &nestcontn2kv::setvo>;
        using setsto_action = eosio::action_wrapper<"setsto"_n, &nestcontn2kv::setsto>;
        using setvos_action = eosio::action_wrapper<"setvos"_n, &nestcontn2kv::setvos>;
        using setpvo_action = eosio::action_wrapper<"setpvo"_n, &nestcontn2kv::setpvo>;

};

