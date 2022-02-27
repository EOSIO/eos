#pragma once

#include <eosio/eosio.hpp>
#include <eosio/varint.hpp>
#include <eosio/ship_protocol.hpp>

namespace ship = eosio::ship_protocol;

#define EOSIO_DECLARE_ACTIONS_INTERNAL_WRAPPER(DUMMY, ACT)                                                             \
    using ACT = eosio::action_wrapper<#ACT##_h, &__contract_class::ACT, __contract_account>;

#define EOSIO_DECLARE_ACTIONS_INTERNAL_DISPATCH(DUMMY, ACT)                                                            \
    case eosio::hash_name(#ACT):                                                                                       \
        executed = eosio::execute_action(eosio::name(receiver), eosio::name(code), &__contract_class::ACT);            \
        break;

#define EOSIO_DECLARE_ACTIONS_INTERNAL_JSON_ACTION_LIST_ITEM(DUMMY, ACT) ",\"" #ACT "\""

#define EOSIO_DECLARE_ACTIONS_INTERNAL_JSON_ACTION_LIST(ACT, ...)                                                      \
    "\"" #ACT "\"" EOSIO_MAP_REUSE_ARG0(EOSIO_DECLARE_ACTIONS_INTERNAL_JSON_ACTION_LIST_ITEM, dummy, __VA_ARGS__)

#define EOSIO_DECLARE_ACTIONS_REFLECT_ITEM(DUMMY, ACT)                                                                 \
    f(#ACT, #ACT##_h,                                                                                                  \
      [](auto p) -> decltype(&std::decay_t<decltype(*p)>::ACT) { return &std::decay_t<decltype(*p)>::ACT; });

#define EOSIO_DECLARE_ACTIONS(CONTRACT_CLASS, CONTRACT_ACCOUNT, ...)                                                   \
    namespace actions {                                                                                                \
    static constexpr auto __contract_account = CONTRACT_ACCOUNT;                                                       \
    using __contract_class                   = CONTRACT_CLASS;                                                         \
    EOSIO_MAP_REUSE_ARG0(EOSIO_DECLARE_ACTIONS_INTERNAL_WRAPPER, dummy, __VA_ARGS__)                                   \
                                                                                                                       \
    inline void eosio_apply(uint64_t receiver, uint64_t code, uint64_t action) {                                       \
        eosio::check(code == receiver, "notifications not supported by dispatcher");                                   \
        bool executed = false;                                                                                         \
        switch (action) { EOSIO_MAP_REUSE_ARG0(EOSIO_DECLARE_ACTIONS_INTERNAL_DISPATCH, dummy, __VA_ARGS__) }          \
        eosio::check(executed == true, "unknown action");                                                              \
    }                                                                                                                  \
    }                                                                                                                  \
    inline constexpr const char eosio_json_action_list[] =                                                             \
        "[" EOSIO_DECLARE_ACTIONS_INTERNAL_JSON_ACTION_LIST(__VA_ARGS__) "]";                                          \
    template <typename F>                                                                                              \
    constexpr void eosio_for_each_action(CONTRACT_CLASS*, F f) {                                                       \
        EOSIO_MAP_REUSE_ARG0(EOSIO_DECLARE_ACTIONS_REFLECT_ITEM, dummy, __VA_ARGS__)                                   \
    }

namespace eosio::wasm_helpers {
extern "C" [[eosio::wasm_import]] uint32_t get_input_data(void* dest, uint32_t size);
extern "C" [[eosio::wasm_import]] void set_output_data(const void* data, uint32_t size);
extern "C" [[eosio::wasm_import]] void push_data(const void* data, uint32_t size);
} // namespace wasm_helpers

inline std::vector<char> get_input_data() {
   std::vector<char> result(eosio::wasm_helpers::get_input_data(nullptr, 0));
   eosio::wasm_helpers::get_input_data(result.data(), result.size());

   return result;
}

struct action_trace_order {
    ship::action_trace_v1* atrace = nullptr;
    std::vector<uint32_t> children;
};

template <typename T, typename R, typename... Args>
void execute_trace(
    uint32_t block_num,
    ship::signed_block_v1& block,
    ship::transaction_trace_v0& ttrace,
    ship::action_trace_v1& atrace, R (T::*func)(Args...)
) {
    std::tuple<std::decay_t<Args>...> args;
    auto stream_copy = atrace.act.data;
    from_bin(args, stream_copy);

    T inst{block_num, block, ttrace, atrace};
    auto f = [&](auto... a) { return (inst.*func)(a...); };
    std::apply(f, args);
}

template <typename F>
void process_traces_impl(
    ship::get_blocks_result_v0& result,
    ship::signed_block_v1& block,
    ship::transaction_trace_v0& ttrace,
    std::vector<action_trace_order>& order,
    uint32_t i,
    const F& f
) {
    auto& o = order[i];
    if (!o.atrace)
        return;
    // print("        ", o.atrace->receiver.to_string(), " ", o.atrace->act.account.to_string(), " ",
    //       o.atrace->act.name.to_string(), " ", o.atrace->act.data.remaining(), " ",
    //       o.atrace->return_value.remaining(),
    //       "\n");
    if (o.atrace->receiver == o.atrace->act.account)
        f(result.this_block->block_num, block, ttrace, *o.atrace);
    o.atrace = nullptr;
    for (auto child : o.children) {
        // print("        child:\n");
        process_traces_impl(result, block, ttrace, order, child, f);
    }
}

template <typename F>
void process_traces_impl(
    ship::get_blocks_result_v0& result,
    const F& f
) {
    ship::signed_block_v1 block;
    if (result.block)
        from_bin(block, *result.block);
    std::vector<ship::transaction_trace> traces;
    from_bin(traces, *result.traces);

    for (auto& ttrace : traces) {
        auto& ttrace0 = std::get<ship::transaction_trace_v0>(ttrace);
        if (ttrace0.status != ship::transaction_status::executed) {
            break;
        }

        std::vector<action_trace_order> order(ttrace0.action_traces.size());

        auto get_order = [&](uint32_t i) -> action_trace_order& {
            eosio::check(i >= 1 && i <= order.size(), "invalid action ordinal");
            return order[i - 1];
        };

        for (uint32_t i = 0; i < ttrace0.action_traces.size(); ++i) {
            auto& atrace1 = std::get<ship::action_trace_v1>(ttrace0.action_traces[i]);
            get_order(atrace1.action_ordinal.value).atrace = &atrace1;
            if (atrace1.creator_action_ordinal.value)
                get_order(atrace1.creator_action_ordinal.value).children.push_back(atrace1.action_ordinal.value - 1);
        }

        for (uint32_t i = 0; i < order.size(); ++i)
            process_traces_impl(result, block, std::get<ship::transaction_trace_v0>(ttrace), order, i, f);
    }
}

#define EOSIO_FILTER_ACCOUNT_N(ACCOUNT, ACTION) #ACCOUNT##_h
#define EOSIO_FILTER_ACTION_N(ACCOUNT, ACTION) #ACTION##_h
#define EOSIO_FILTER_ACTION(ACCOUNT, ACTION) ACTION

#define EOSIO_FILTER_ACTIONS_IMPL(FILTER_CLASS, ACCOUNT_ACTION)                                                        \
    if (atrace.act.account == EOSIO_FILTER_ACCOUNT_N ACCOUNT_ACTION &&                                                 \
        atrace.act.name == EOSIO_FILTER_ACTION_N     ACCOUNT_ACTION)                                                   \
                                                                                                                       \
        return execute_trace(block_num, block, ttrace, atrace,                                                  \
                                    &FILTER_CLASS::EOSIO_FILTER_ACTION ACCOUNT_ACTION);

// Executes in dependency order instead of nodeos's execution order. Skips require_recipient notifications.
// Should work well when filters look primarily at inline action notifications, but may backfire when they
// look at non-inline actions or at actions which have side effects.
#define EOSIO_FILTER_ACTIONS(FILTER_CLASS, ...)                                                                        \
    inline void process_trace(uint32_t block_num, ship::signed_block_v1& block,                           \
                              ship::transaction_trace_v0& ttrace,                                      \
                              ship::action_trace_v1&      atrace) {                                         \
        EOSIO_MAP_REUSE_ARG0(EOSIO_FILTER_ACTIONS_IMPL, FILTER_CLASS, __VA_ARGS__)                                     \
    }                                                                                                                  \
    inline void process_traces(ship::get_blocks_result_v0& result) {                                   \
        process_traces_impl(result, process_trace);                                                             \
    }
