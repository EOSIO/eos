#include <eosio/chain/controller.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/exceptions.hpp>

#include <eosio/chain/account_object.hpp>

#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/resource_limits.hpp>

#include <cyberway/chaindb/controller.hpp>
#include <cyberway/chaindb/names.hpp>

#include <cyberway/chain/cyberway_contract.hpp>
#include <cyberway/chain/cyberway_contract_types.hpp>

namespace cyberway { namespace chain {

void apply_cyber_providebw(apply_context& context) {
    auto args = context.act.data_as<providebw>();
    context.require_authorization(args.provider);
}

// TODO: requestbw
//void apply_cyber_requestbw(apply_context& context) {
//   auto args = context.act.data_as<requestbw>();
//   context.require_authorization(args.account);
//}

void apply_cyber_provideram(apply_context& context) {
    auto args = context.act.data_as<provideram>();
    context.require_authorization(args.provider);
}

void apply_cyber_domain_newdomain(apply_context& context) {
    auto op = context.act.data_as<newdomain>();
    try {
        context.require_authorization(op.creator);
        validate_domain_name(op.name);             // TODO: can move validation to domain_name deserializer
        auto& chaindb = context.chaindb;
        auto exists = chaindb.find<domain_object, by_name>(op.name);
        EOS_ASSERT(exists == nullptr, eosio::chain::domain_exists_exception,
            "Cannot create domain named ${n}, as that name is already taken", ("n", op.name));
        chaindb.emplace<domain_object>(context.get_storage_payer(op.creator), [&](auto& d) {
            d.owner = op.creator;
            d.creation_date = context.control.pending_block_time();
            d.name = op.name;
        });
    } FC_CAPTURE_AND_RETHROW((op))
}

void apply_cyber_domain_passdomain(apply_context& context) {
    auto op = context.act.data_as<passdomain>();
    try {
        context.require_authorization(op.from);   // TODO: special case if nobody owns domain
        validate_domain_name(op.name);
        const auto& domain = context.control.get_domain(op.name);
        EOS_ASSERT(op.from == domain.owner, eosio::chain::action_validate_exception, "Only owner can pass domain name");
        context.chaindb.modify(domain, context.get_storage_payer(op.to), [&](auto& d) {
            d.owner = op.to;
        });
    } FC_CAPTURE_AND_RETHROW((op))
}

void apply_cyber_domain_linkdomain(apply_context& context) {
    auto op = context.act.data_as<linkdomain>();
    try {
        context.require_authorization(op.owner);
        validate_domain_name(op.name);
        const auto& domain = context.control.get_domain(op.name);
        EOS_ASSERT(op.owner == domain.owner, eosio::chain::action_validate_exception, "Only owner can change domain link");
        EOS_ASSERT(op.to != domain.linked_to, eosio::chain::action_validate_exception, "Domain name already linked to the same account");
        context.chaindb.modify(domain, context.get_storage_payer(domain.owner), [&](auto& d) {
            d.linked_to = op.to;
        });
    } FC_CAPTURE_AND_RETHROW((op))
}

void apply_cyber_domain_unlinkdomain(apply_context& context) {
    auto op = context.act.data_as<unlinkdomain>();
    try {
        context.require_authorization(op.owner);
        validate_domain_name(op.name);
        const auto& domain = context.control.get_domain(op.name);
        EOS_ASSERT(op.owner == domain.owner, eosio::chain::action_validate_exception, "Only owner can unlink domain");
        EOS_ASSERT(domain.linked_to != account_name(), eosio::chain::action_validate_exception, "Domain name already unlinked");
        context.chaindb.modify(domain, context.get_storage_payer(domain.owner), [&](auto& d) {
            d.linked_to = account_name();
        });
    } FC_CAPTURE_AND_RETHROW((op))
}

void apply_cyber_domain_newusername(apply_context& context) {
    auto op = context.act.data_as<newusername>();
    try {
        context.require_authorization(op.creator);
        validate_username(op.name);               // TODO: can move validation to username deserializer
        auto& chaindb = context.chaindb;
        auto exists = chaindb.find<username_object, by_scope_name>(boost::make_tuple(op.creator, op.name));
        EOS_ASSERT(exists == nullptr, eosio::chain::username_exists_exception,
            "Cannot create username ${n} in scope ${s}, as it's already taken", ("n", op.name)("s", op.creator));
        auto owner = chaindb.find<eosio::chain::account_object, by_name>(op.owner);
        EOS_ASSERT(owner, eosio::chain::account_name_exists_exception, "Username owner (${o}) must exist", ("o", op.owner));
        chaindb.emplace<username_object>(context.get_storage_payer(op.creator), [&](auto& d) {
            d.owner = op.owner;
            d.scope = op.creator;
            d.name = op.name;
        });
    } FC_CAPTURE_AND_RETHROW((op))
}

#define _CYBERWAY_OBJECT_QUERY_CHECK(_CHECK, _OP)  \
    EOS_ASSERT((_CHECK), eosio::chain::object_query_exception, \
        "Object with the primary key ${scope}:${pk} doesn't exist in the table ${table}", \
        ("pk", (_OP).pk)("scope", chaindb::get_scope_name((_OP).scope))("table", chaindb::get_full_table_name(_OP)))

template <typename Operation>
chaindb::cache_object_ptr get_cache_object(apply_context& context, const Operation& op) {
    auto find = context.chaindb.lower_bound({op.code, op.scope, op.table}, op.pk);
    _CYBERWAY_OBJECT_QUERY_CHECK(find.pk == op.pk, op);

    auto cache = context.chaindb.get_cache_object({op.code, find.cursor}, false);
    _CYBERWAY_OBJECT_QUERY_CHECK(!!cache, op);
    return cache;
}

#undef _CYBERWAY_OBJECT_QUERY_CHECK


void apply_cyber_setrampayer(apply_context& context) {
    auto op = context.act.data_as<set_ram_payer>();
    try {
        context.require_authorization(op.new_payer);

        auto cache = get_cache_object(context, op);
        auto owner = cache->service().owner;
        context.require_authorization(owner);

        EOS_ASSERT(op.new_payer != cache->service().payer, eosio::chain::object_ram_payer_exception,
            "Object with the primary key ${scope}:${pk} in the table ${table} already has the RAM payer ${payer}",
            ("pk", op.pk)("scope", chaindb::get_scope_name(op.scope))("table", chaindb::get_full_table_name(op))
            ("payer", op.new_payer));

        context.chaindb.recalc_ram_usage(*cache.get(), context.get_storage_payer(owner, op.new_payer));
    } FC_CAPTURE_AND_RETHROW((op))
}

void apply_set_ram_state(
    apply_context& context, const set_ram_state& op,
    const std::function<void(const service_state&)>& validate
) {
    auto  cache   = get_cache_object(context, op);
    auto& service = cache->service();

    validate(service);

    EOS_ASSERT(op.in_ram != cache->service().in_ram, eosio::chain::object_ram_state_exception,
        "Object with the primary key ${scope}:${pk} in the table ${table} already has RAM state = ${state}",
        ("pk", op.pk)("scope", chaindb::get_scope_name(op.scope))("table", chaindb::get_full_table_name(op))
        ("state", op.in_ram));

    auto info = context.get_storage_payer(service.owner, service.payer);
    info.in_ram  = op.in_ram;
    context.chaindb.recalc_ram_usage(*cache.get(), info);
}

void apply_cyber_setramstate(apply_context& context) {
    auto op = context.act.data_as<set_ram_state>();
    try {
        apply_set_ram_state(context, op, [&](const service_state& service) {
            if (service.owner == service.payer || !context.weak_require_authorization(service.payer)) {
                context.require_authorization(service.owner);
            }
        });
    } FC_CAPTURE_AND_RETHROW((op))
}

} } // namespace cyberway::chain
