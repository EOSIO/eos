#pragma once
#include <fc/reflect/reflect.hpp>
#include <vector>

namespace cyberway { namespace genesis {


struct posting_rules {
    struct fn_value {
        uint8_t kind;
        uint64_t idx;
    };
    struct fn_bytecode {
        uint64_t varssize;
        std::vector<uint64_t> operators;
        std::vector<fn_value> values;
        std::vector<int64_t> nums;
        std::vector<int64_t> consts;
    };
    struct fn_info {
        fn_bytecode code;
        int64_t maxarg;
    };

    fn_info mainfunc;           // reward_fn
    fn_info curationfunc;       // curation_fn
    fn_info timepenalty;        // time_penalty_fn
    int64_t maxtokenprop;       // max_token_prop
};

}} // cyberway::genesis

FC_REFLECT(cyberway::genesis::posting_rules::fn_value, (kind)(idx))
FC_REFLECT(cyberway::genesis::posting_rules::fn_bytecode, (varssize)(operators)(values)(nums)(consts))
FC_REFLECT(cyberway::genesis::posting_rules::fn_info, (code)(maxarg))
FC_REFLECT(cyberway::genesis::posting_rules, (mainfunc)(curationfunc)(timepenalty)(maxtokenprop))
