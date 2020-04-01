#pragma once

#include <eosio/chain/types.hpp>
#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/eos-vm.hpp>

#include <eosio/vm/backend.hpp>
#include <eosio/vm/host_function.hpp>

namespace eosio { namespace chain { namespace webassembly {
   namespace detail {
      template <typename T>
      constexpr std::true_type is_legacy_ptr(legacy_ptr<T>);
      template <typename T>
      constexpr std::false_type is_legacy_ptr(T);
      template <typename T>
      constexpr std::true_type is_legacy_array_ptr(legacy_array_ptr<T>);
      template <typename T>
      constexpr std::false_type is_legacy_array_ptr(T);
      template <typename T>
      constexpr std::true_type is_unvalidated_ptr(unvalidated_ptr<T>);
      template <typename T>
      constexpr std::false_type is_unvalidated_ptr(T);

      template <typename T>
      struct is_whitelisted_legacy_type {
         static constexpr bool value = std::is_same_v<float128_t, T> ||
                                       std::is_same_v<null_terminated_ptr, T> ||
                                       std::is_same_v<decltype(is_legacy_ptr(std::declval<T>())), std::true_type> ||
                                       std::is_same_v<decltype(is_legacy_array_ptr(std::declval<T>())), std::true_type> ||
                                       std::is_same_v<decltype(is_unvalidated_ptr(std::declval<T>())), std::true_type> ||
                                       std::is_same_v<name, T> ||
                                       std::is_arithmetic_v<T>;
      };
      template <typename T>
      struct is_whitelisted_type {
         static constexpr bool value = std::is_arithmetic_v<std::decay_t<T>> ||
                                       std::is_same_v<std::decay_t<T>, name> ||
                                       std::is_pointer_v<T>    ||
                                       std::is_lvalue_reference_v<T> ||
                                       std::is_same_v<std::decay_t<T>, float128_t> ||
                                       eosio::vm::is_span_type_v<std::decay_t<T>>;
      };
   }

   template <typename T>
   inline static constexpr bool is_whitelisted_type_v = detail::is_whitelisted_type<T>::value;

   template <typename T>
   inline static constexpr bool is_whitelisted_legacy_type_v = detail::is_whitelisted_legacy_type<T>::value;

   template <typename... Ts>
   inline static constexpr bool are_whitelisted_types_v = true; //(... && detail::is_whitelisted_type<Ts>::value);

   template <typename... Ts>
   inline static constexpr bool are_whitelisted_legacy_types_v = (... && detail::is_whitelisted_legacy_type<Ts>::value);

   template <typename T, typename U>
   inline static bool is_aliasing(const T& a, const U& b) {
      std::uintptr_t a_ui = reinterpret_cast<std::uintptr_t>(a.data());
      std::uintptr_t b_ui = reinterpret_cast<std::uintptr_t>(b.data());
      return a_ui < b_ui ? a_ui + a.size() >= b_ui : b_ui + b.size() >= a_ui;
   }
   inline static bool is_nan( const float32_t f ) {
      return f32_is_nan( f );
   }
   inline static bool is_nan( const float64_t f ) {
      return f64_is_nan( f );
   }
   inline static bool is_nan( const float128_t& f ) {
      return f128_is_nan( f );
   }

   EOS_VM_PRECONDITION(early_validate_pointers,
         EOS_VM_INVOKE_ON_ALL([&](auto arg, auto&&... rest) {
            using namespace eosio::vm;
            using arg_t = decltype(arg);
            if constexpr (is_reference_proxy_type_v<arg_t>)
               if constexpr (is_span_type_v<dependent_type_t<arg_t>>) {
                  using dep_t = dependent_type_t<arg_t>;
                  const auto& s = (dep_t&)arg;
                  EOS_ASSERT( s.size() <= std::numeric_limits<wasm_size_t>::max() / (wasm_size_t)sizeof(dependent_type_t<dep_t>),
                        wasm_exception, "length will overflow" );
                  volatile auto check = *(reinterpret_cast<const char*>(s.data()) + s.size_bytes() - 1);
                  ignore_unused_variable_warning(check);
               }
         }));

   EOS_VM_PRECONDITION(context_free_check,
         EOS_VM_INVOKE_ONCE([&](auto&&...) {
            EOS_ASSERT(ctx.get_host().get_context().is_context_free(), unaccessible_api, "only context free api's can be used in this context");
         }));

   EOS_VM_PRECONDITION(alias_check,
         EOS_VM_INVOKE_ON_ALL(([&](auto arg, auto&&... rest) {
            using namespace eosio::vm;
            using arg_t = decltype(arg);
            if constexpr (is_span_type_v<arg_t>) {
               // check alignment while we are here
               EOS_ASSERT( reinterpret_cast<std::uintptr_t>(arg.data()) % alignof(dependent_type_t<arg_t>) == 0,
                     wasm_exception, "memory not aligned" );
               eosio::vm::invoke_on<false, eosio::vm::invoke_on_all_t>([&](auto narg, auto&&... nrest) {
                  using nested_arg_t = decltype(arg);
                  if constexpr (eosio::vm::is_span_type_v<nested_arg_t>)
                     EOS_ASSERT(!is_aliasing(arg, narg), wasm_exception, "arrays not allowed to alias");
               }, rest...);
            }
         })));

   EOS_VM_PRECONDITION(is_nan_check,
         EOS_VM_INVOKE_ON(float, [&](auto arg, auto&&... rest) {
            EOS_ASSERT(!is_nan(arg), transaction_exception, "NaN is not an allowed value for a secondary key");
         });
         EOS_VM_INVOKE_ON(double, [&](auto arg, auto&&... rest) {
            EOS_ASSERT(!is_nan(arg), transaction_exception, "NaN is not an allowed value for a secondary key");
         });
         EOS_VM_INVOKE_ON(const float128_t&, [&](const auto& arg, auto&&... rest) {
            EOS_ASSERT(!is_nan(arg), transaction_exception, "NaN is not an allowed value for a secondary key");
         })
         );

   EOS_VM_PRECONDITION(legacy_static_check_wl_args,
         EOS_VM_INVOKE_ONCE([&](auto&&... args) {
            static_assert( are_whitelisted_legacy_types_v<std::decay_t<decltype(args)>...>, "legacy whitelisted type violation");
         }));

   EOS_VM_PRECONDITION(static_check_wl_args,
         EOS_VM_INVOKE_ONCE([&](auto&&... args) {
            static_assert( are_whitelisted_types_v<std::decay_t<decltype(args)>...>, "whitelisted type violation");
         }));
}}} // ns eosio::chain::webassembly
