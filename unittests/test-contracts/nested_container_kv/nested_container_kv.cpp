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

typedef set<uint16_t> set_uint16;
typedef vector<uint16_t> vec_uint16;
typedef optional<uint16_t> op_uint16;
typedef map<uint32_t, uint32_t> mp_uint16;
typedef pair<uint32_t, uint32_t> pr_uint16;

typedef vector< op_uint16 > vec_op_uint16;
typedef optional< mystruct > op_struc;

struct person2kv {
    set< set_uint16 > stst;
    set< vec_uint16 > stv;
    set< op_uint16 > sto;
    set< mp_uint16 > stm;
    set< pr_uint16 > stp;

    vector< set_uint16 > vst;
    vector< vec_uint16 > vv;
    vector< op_uint16 > vo;
    vector< mp_uint16 > vm;
    vector< pr_uint16 > vp;

    optional< set_uint16 > ost;
    optional< vec_uint16 > ov;
    optional< op_uint16 > oo;
    optional< mp_uint16 > om;
    optional< pr_uint16 > op;

    map< uint16_t, set_uint16 > mst;
    map< uint16_t, vec_uint16 > mv;
    map< uint16_t, op_uint16 > mo;
    map< uint16_t, mp_uint16 > mm;
    map< uint16_t, pr_uint16 > mp;

    pair< uint16_t, set_uint16 > pst;
    pair< uint16_t, vec_uint16 > pv;
    pair< uint16_t, op_uint16 > po;
    pair< uint16_t, mp_uint16 > pm;
    pair< uint16_t, pr_uint16 > pp;

    vector< op_struc > vos;
    pair<uint16_t, vec_op_uint16> pvo;
};

class [[eosio::contract("nested_container_kv")]] nestcontn2kv : public eosio::contract {
    using my_map_t = eosio::kv::map<"people2kv"_n, int, person2kv>;

    private:
        my_map_t mymap{};

    public:
        using contract::contract;

        nestcontn2kv(name receiver, name code,  datastream<const char*> ds): contract(receiver, code, ds) {}

        [[eosio::action]]
        person2kv get(int id){
            //This action will be used by all prntx actions to retrieve item_found for given id
            auto iter = mymap.find(id);
            check(iter != mymap.end(), "Record does not exist");
            const auto& item_found = iter->second();
            return item_found;
        }

        [[eosio::action]]
        void setstst(int id, const set< set_uint16 >& stst){
            SETCONTAINERVAL(stst);
            eosio::print("type defined set< set< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setstv(int id, const set< vec_uint16 >& stv){
            SETCONTAINERVAL(stv);
            eosio::print("type defined set< vector< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setsto(int id, const set< op_uint16 >& sto){
            SETCONTAINERVAL(sto);
            eosio::print("type defined set< optional< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setstm(int id, const set< mp_uint16 >& stm){
            SETCONTAINERVAL(stm);
            eosio::print("type defined set< map< uint16_t, uint16_t>> stored successfully!");
        }

        [[eosio::action]]
        void setstp(int id, const set< pr_uint16 >& stp){
            SETCONTAINERVAL(stp);
            eosio::print("type defined set< pair< uint16_t, uint16_t >> stored successfully");
        }


        [[eosio::action]]
        void setvst(int id, const vector< set_uint16 >& vst){
            SETCONTAINERVAL(vst);
            eosio::print("type defined vector< set< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setvv(int id, const vector< vec_uint16 >& vv){
            SETCONTAINERVAL(vv);
            eosio::print("type defined vector< vector< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setvo(int id, const vector< op_uint16 >& vo){
            SETCONTAINERVAL(vo);
            eosio::print("type defined vector< optional< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setvm(int id, const vector< mp_uint16 >& vm){
            SETCONTAINERVAL(vm);
            eosio::print("type defined vector< map< uint16_t, uint16_t>> stored successfully!");
        }

        [[eosio::action]]
        void setvp(int id, const vector< pr_uint16 >& vp){
            SETCONTAINERVAL(vp);
            eosio::print("type defined vector< pair< uint16_t, uint16_t >> stored successfully");
        }


        [[eosio::action]]
        void setost(int id, const optional< set_uint16 >& ost){
            SETCONTAINERVAL(ost);
            eosio::print("type defined optional< set< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setov(int id, const optional< vec_uint16 >& ov){
            SETCONTAINERVAL(ov);
            eosio::print("type defined optional< vector< uint16_t >> stored successfully!");
        }

        [[eosio::action]] 
        void setoo(int id, const optional< op_uint16 > & oo){
            SETCONTAINERVAL(oo);
            eosio::print("type defined optional< optional< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setom(int id, const optional< mp_uint16 >& om){
            SETCONTAINERVAL(om);
            eosio::print("type defined optional< map< uint16_t, uint16_t>> stored successfully!");
        }

        [[eosio::action]]
        void setop(int id, const optional< pr_uint16 >& op){
            SETCONTAINERVAL(op);
            eosio::print("type defined optional< pair< uint16_t, uint16_t >> stored successfully");
        }


        [[eosio::action]]
        void setmst(int id, const map< uint16_t, set_uint16 >& mst){
            SETCONTAINERVAL(mst);
            eosio::print("type defined map< set< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setmv(int id, const map< uint16_t, vec_uint16 >& mv){
            SETCONTAINERVAL(mv);
            eosio::print("type defined map< vector< uint16_t >> stored successfully!");
        }

        [[eosio::action]] 
        void setmo(int id, const map< uint16_t, op_uint16 > & mo){
            SETCONTAINERVAL(mo);
            eosio::print("type defined map< optional< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setmm(int id, const map< uint16_t, mp_uint16 >& mm){
            SETCONTAINERVAL(mm);
            eosio::print("type defined map< map< uint16_t, uint16_t>> stored successfully!");
        }

        [[eosio::action]]
        void setmp(int id, const map< uint16_t, pr_uint16 >& mp){
            SETCONTAINERVAL(mp);
            eosio::print("type defined map< pair< uint16_t, uint16_t >> stored successfully");
        }


        [[eosio::action]]
        void setpst(int id, const pair< uint16_t, set_uint16 >& pst){
            SETCONTAINERVAL(pst);
            eosio::print("type defined pair< set< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setpv(int id, const pair< uint16_t, vec_uint16 >& pv){
            SETCONTAINERVAL(pv);
            eosio::print("type defined pair< vector< uint16_t >> stored successfully!");
        }

        [[eosio::action]] 
        void setpo(int id, const pair< uint16_t, op_uint16 > & po){
            SETCONTAINERVAL(po);
            eosio::print("type defined pair< optional< uint16_t >> stored successfully!");
        }

        [[eosio::action]]
        void setpm(int id, const pair< uint16_t, mp_uint16 >& pm){
            SETCONTAINERVAL(pm);
            eosio::print("type defined pair< map< uint16_t, uint16_t>> stored successfully!");
        }

        [[eosio::action]]
        void setpp(int id, const pair< uint16_t, pr_uint16 >& pp){
            SETCONTAINERVAL(pp);
            eosio::print("type defined pair< pair< uint16_t, uint16_t >> stored successfully");
        }



        [[eosio::action]]
        void setvos(int id, const vector<op_struc>& vos)
        {
            SETCONTAINERVAL(vos);
            eosio::print("vector<optional<mystruct>> stored successfully");
        }

        [[eosio::action]]
        void setpvo(int id, const pair<uint16_t, vec_op_uint16>& pvo)
        {
            SETCONTAINERVAL(pvo);
            eosio::print("pair<uint16_t, vector<optional<uint16_t>>> stored successfully");
        }

        using get_action = eosio::action_wrapper<"get"_n, &nestcontn2kv::get>;

        using setstst_action = eosio::action_wrapper<"setstst"_n, &nestcontn2kv::setstst>;
        using setstv_action = eosio::action_wrapper<"setstv"_n, &nestcontn2kv::setstv>;
        using setsto_action = eosio::action_wrapper<"setsto"_n, &nestcontn2kv::setsto>;
        using setstm_action = eosio::action_wrapper<"setstm"_n, &nestcontn2kv::setstm>;
        using setstp_action = eosio::action_wrapper<"setstp"_n, &nestcontn2kv::setstp>;

        using setvst_action = eosio::action_wrapper<"setvst"_n, &nestcontn2kv::setvst>;
        using setvv_action = eosio::action_wrapper<"setvv"_n, &nestcontn2kv::setvv>;
        using setvo_action = eosio::action_wrapper<"setvo"_n, &nestcontn2kv::setvo>;
        using setvm_action = eosio::action_wrapper<"setvm"_n, &nestcontn2kv::setvm>;
        using setvp_action = eosio::action_wrapper<"setvp"_n, &nestcontn2kv::setvp>;

        using setost_action = eosio::action_wrapper<"setost"_n, &nestcontn2kv::setost>;
        using setov_action = eosio::action_wrapper<"setov"_n, &nestcontn2kv::setov>;
        using setoo_action = eosio::action_wrapper<"setoo"_n, &nestcontn2kv::setoo>;
        using setom_action = eosio::action_wrapper<"setom"_n, &nestcontn2kv::setom>;
        using setop_action = eosio::action_wrapper<"setop"_n, &nestcontn2kv::setop>;

        using setmst_action = eosio::action_wrapper<"setmst"_n, &nestcontn2kv::setmst>;
        using setmv_action = eosio::action_wrapper<"setmv"_n, &nestcontn2kv::setmv>;
        using setmo_action = eosio::action_wrapper<"setmo"_n, &nestcontn2kv::setmo>;
        using setmm_action = eosio::action_wrapper<"setmm"_n, &nestcontn2kv::setmm>;
        using setmp_action = eosio::action_wrapper<"setmp"_n, &nestcontn2kv::setmp>;

        using setpst_action = eosio::action_wrapper<"setpst"_n, &nestcontn2kv::setpst>;
        using setpv_action = eosio::action_wrapper<"setpv"_n, &nestcontn2kv::setpv>;
        using setpo_action = eosio::action_wrapper<"setpo"_n, &nestcontn2kv::setpo>;
        using setpm_action = eosio::action_wrapper<"setpm"_n, &nestcontn2kv::setpm>;
        using setpp_action = eosio::action_wrapper<"setpp"_n, &nestcontn2kv::setpp>;


        using setvos_action = eosio::action_wrapper<"setvos"_n, &nestcontn2kv::setvos>;
        using setpvo_action = eosio::action_wrapper<"setpvo"_n, &nestcontn2kv::setpvo>;

};

