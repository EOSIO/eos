#pragma once

#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/webassembly/runtime_interface.hpp>
#include <eosio/chain/wasm_eosio_injection.hpp>
//#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fc/scoped_exit.hpp>

#include <eosio/chain/webassembly/wavm.hpp>
#include <eosio/chain/webassembly/wabt.hpp>

#include "IR/Module.h"
#include "Runtime/Intrinsics.h"
#include "Platform/Platform.h"
#include "WAST/WAST.h"
#include "IR/Validate.h"
#include <eosiolib_native/vm_api.h>

#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

using namespace fc;
using namespace eosio::chain::webassembly;
using namespace IR;
using namespace Runtime;

namespace eosio { namespace chain {

   struct code_info {
      digest_type code_id;
      vector<uint8_t> code;
   };

   class blocking_queue {

      public:
      void push(const code_info& value) { // push
         std::lock_guard<std::mutex> lock(queue_mutex);
         queue.push(std::move(value));
         code_map[value.code_id] = 1;
         condition.notify_one();
      }

      bool try_pop(code_info& value) { // non-blocking pop
         std::lock_guard<std::mutex> lock(queue_mutex);
         if (queue.empty()) return false;
         value = std::move(queue.front());
         queue.pop();
         code_map.erase(value.code_id);
         return true;
      }

      bool wait_pop(code_info& value) { // blocking pop
         std::unique_lock<std::mutex> lock(queue_mutex);
          condition.wait(lock, [this]{return !queue.empty() || stopped; });
         if (stopped) {
            return false;
         }
         value = std::move(queue.front());
         queue.pop();
         code_map.erase(value.code_id);
         return true;
      }

      bool wait_front(code_info& value) { // blocking pop
         std::unique_lock<std::mutex> lock(queue_mutex);
          condition.wait(lock, [this]{return !queue.empty() || stopped; });
         if (stopped) {
            return false;
         }
         value = std::move(queue.front());
         return true;
      }

      bool contains(const digest_type& code_id) {
         std::lock_guard<std::mutex> lock(queue_mutex);
         return code_map.find(code_id) != code_map.end();
      }

      void pop(const digest_type& code_id) {
         std::lock_guard<std::mutex> lock(queue_mutex);
         queue.pop();
         code_map.erase(code_id);
      }

      bool empty() { // queue is empty?
         std::lock_guard<std::mutex> lock(queue_mutex);
         return queue.empty();
      }

      void clear() { // remove all items
         code_info item;
         while (try_pop(item));
      }

      void stop() {
         stopped = true;
         condition.notify_one();
      }

      bool stopped = false;
      private:
         std::mutex queue_mutex;
         std::queue<code_info> queue;
         std::map<digest_type, int> code_map;
         std::condition_variable condition;
   };

   static void loading_proc(wasm_interface_impl& imp);
   struct wasm_interface_impl {
      wasm_interface_impl(wasm_interface::vm_type vm) {
         _vm_type = vm;
         if(vm == wasm_interface::vm_type::wavm) {
            loading_thread = std::make_unique<std::thread>(loading_proc, std::ref(*this));
            runtime_interface = std::make_unique<webassembly::wavm::wavm_runtime>();
         }
         else if(vm == wasm_interface::vm_type::wabt)
            runtime_interface = std::make_unique<webassembly::wabt_runtime::wabt_runtime>();
         else
            EOS_THROW(wasm_exception, "wasm_interface_impl fall through");
      }

      ~wasm_interface_impl() {
         if(_vm_type == wasm_interface::vm_type::wavm) {
            stop_thread();
         }
      }

      std::vector<uint8_t> parse_initial_memory(const Module& module) {
         std::vector<uint8_t> mem_image;

         for(const DataSegment& data_segment : module.dataSegments) {
            EOS_ASSERT(data_segment.baseOffset.type == InitializerExpression::Type::i32_const, wasm_exception, "");
            EOS_ASSERT(module.memories.defs.size(), wasm_exception, "");
            const U32 base_offset = data_segment.baseOffset.i32;
            const Uptr memory_size = (module.memories.defs[0].type.size.min << IR::numBytesPerPageLog2);
            if(base_offset >= memory_size || base_offset + data_segment.data.size() > memory_size)
               FC_THROW_EXCEPTION(wasm_execution_error, "WASM data segment outside of valid memory range");
            if(base_offset + data_segment.data.size() > mem_image.size())
               mem_image.resize(base_offset + data_segment.data.size(), 0x00);
            memcpy(mem_image.data() + base_offset, data_segment.data.data(), data_segment.data.size());
         }

         return mem_image;
      }

      bool has_module( const digest_type& code_id )
      {
         std::lock_guard<std::mutex> guard(cache_mutex);
         auto it = instantiation_cache.find(code_id);
         return it != instantiation_cache.end();
      }

      std::unique_ptr<wasm_instantiated_module_interface>& get_instantiated_module( const digest_type& code_id,
                                                                                    const shared_string& code)
      {
         map<digest_type, std::unique_ptr<wasm_instantiated_module_interface>>::iterator it;
         {
            std::lock_guard<std::mutex> guard(cache_mutex);
            it = instantiation_cache.find(code_id);
         }

         if(it == instantiation_cache.end()) {
            auto timer_pause = fc::make_scoped_exit([&](){
               get_chain_api_cpp()->resume_billing_timer();
            });
            get_chain_api_cpp()->pause_billing_timer();
            IR::Module module;
            try {
               Serialization::MemoryInputStream stream((const U8*)code.data(), code.size());
               WASM::serialize(stream, module);
               module.userSections.clear();
            } catch(const Serialization::FatalSerializationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            } catch(const IR::ValidationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            }

            wasm_injections::wasm_binary_injection injector(module);
            injector.inject();

            std::vector<U8> bytes;
            try {
               Serialization::ArrayOutputStream outstream;
               WASM::serialize(outstream, module);
               bytes = outstream.getBytes();
            } catch(const Serialization::FatalSerializationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            } catch(const IR::ValidationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            }
            std::unique_ptr<wasm_instantiated_module_interface> instance = runtime_interface->instantiate_module((const char*)bytes.data(), bytes.size(), parse_initial_memory(module));
            {
               std::lock_guard<std::mutex> guard(cache_mutex);
               it = instantiation_cache.emplace(code_id, std::move(instance)).first;
            }
         }
         return it->second;
      }

      void load_module( const digest_type& code_id, const shared_string& code) {
         if (loading_queue.contains(code_id)) {
            return;
         }
         vector<uint8_t> _code(code.data(), code.data()+code.size());
         struct code_info info = {code_id, _code};
         loading_queue.push(info);
      }

      void load_module( const digest_type& code_id, const vector<uint8_t>& code)
      {
         map<digest_type, std::unique_ptr<wasm_instantiated_module_interface>>::iterator it;
         {
            std::lock_guard<std::mutex> guard(cache_mutex);
            it = instantiation_cache.find(code_id);
         }
         if(it == instantiation_cache.end()) {
            IR::Module module;
            try {
               Serialization::MemoryInputStream stream((const U8*)code.data(), code.size());
               WASM::serialize(stream, module);
               module.userSections.clear();
            } catch(const Serialization::FatalSerializationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            } catch(const IR::ValidationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            }

            wasm_injections::wasm_binary_injection injector(module);
            injector.inject();

            std::vector<U8> bytes;
            try {
               Serialization::ArrayOutputStream outstream;
               WASM::serialize(outstream, module);
               bytes = outstream.getBytes();
            } catch(const Serialization::FatalSerializationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            } catch(const IR::ValidationException& e) {
               EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
            }
            std::unique_ptr<wasm_instantiated_module_interface> instance = runtime_interface->instantiate_module((const char*)bytes.data(), bytes.size(), parse_initial_memory(module));
            {
               std::lock_guard<std::mutex> guard(cache_mutex);
               instantiation_cache.emplace(code_id, std::move(instance));
            }
         }
      }
      
      bool load_module_async() {
         code_info info;
         bool ret = loading_queue.wait_front(info);
         if (!ret) {
            return false;
         }
         vmdlog("+++loading module begin\n");
         load_module(info.code_id, info.code);
         vmdlog("+++loading module finished\n");
         loading_queue.pop(info.code_id);
         return true;
      }
      
      void stop_thread() {
         loading_queue.stop();
         loading_thread->join();
      }

      bool is_busy() {
         return !loading_queue.empty();
      }

      bool thread_stopped = false;
      bool busy_loading = false;
      wasm_interface::vm_type _vm_type;
      blocking_queue loading_queue;
      std::mutex cache_mutex;
      std::unique_ptr<std::thread> loading_thread;
      std::unique_ptr<wasm_runtime_interface> runtime_interface;
      map<digest_type, std::unique_ptr<wasm_instantiated_module_interface>> instantiation_cache;
   };

   static void loading_proc(wasm_interface_impl& imp)
   {
      while(true) {
         bool ret = imp.load_module_async();
         if (!ret) {
            break;
         }
      }
   }

#define _REGISTER_INTRINSIC_EXPLICIT(CLS, MOD, METHOD, WASM_SIG, NAME, SIG)\
   _REGISTER_WAVM_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, NAME, SIG)\
   _REGISTER_WABT_INTRINSIC(CLS, MOD, METHOD, WASM_SIG, NAME, SIG)

#define _REGISTER_INTRINSIC4(CLS, MOD, METHOD, WASM_SIG, NAME, SIG)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, MOD, METHOD, WASM_SIG, NAME, SIG )

#define _REGISTER_INTRINSIC3(CLS, MOD, METHOD, WASM_SIG, NAME)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, MOD, METHOD, WASM_SIG, NAME, decltype(&CLS::METHOD) )

#define _REGISTER_INTRINSIC2(CLS, MOD, METHOD, WASM_SIG)\
   _REGISTER_INTRINSIC_EXPLICIT(CLS, MOD, METHOD, WASM_SIG, BOOST_PP_STRINGIZE(METHOD), decltype(&CLS::METHOD) )

#define _REGISTER_INTRINSIC1(CLS, MOD, METHOD)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":" BOOST_PP_STRINGIZE(METHOD) " without a signature");

#define _REGISTER_INTRINSIC0(CLS, MOD, METHOD)\
   static_assert(false, "Cannot register " BOOST_PP_STRINGIZE(CLS) ":<unknown> without a method name and signature");

#define _UNWRAP_SEQ(...) __VA_ARGS__

#define _EXPAND_ARGS(CLS, MOD, INFO)\
   ( CLS, MOD, _UNWRAP_SEQ INFO )

#define _REGISTER_INTRINSIC(R, CLS, INFO)\
   BOOST_PP_CAT(BOOST_PP_OVERLOAD(_REGISTER_INTRINSIC, _UNWRAP_SEQ INFO) _EXPAND_ARGS(CLS, "env", INFO), BOOST_PP_EMPTY())

#define REGISTER_INTRINSICS(CLS, MEMBERS)\
   BOOST_PP_SEQ_FOR_EACH(_REGISTER_INTRINSIC, CLS, _WRAPPED_SEQ(MEMBERS))

#define _REGISTER_INJECTED_INTRINSIC(R, CLS, INFO)\
   BOOST_PP_CAT(BOOST_PP_OVERLOAD(_REGISTER_INTRINSIC, _UNWRAP_SEQ INFO) _EXPAND_ARGS(CLS, EOSIO_INJECTED_MODULE_NAME, INFO), BOOST_PP_EMPTY())

#define REGISTER_INJECTED_INTRINSICS(CLS, MEMBERS)\
   BOOST_PP_SEQ_FOR_EACH(_REGISTER_INJECTED_INTRINSIC, CLS, _WRAPPED_SEQ(MEMBERS))

} } // eosio::chain
