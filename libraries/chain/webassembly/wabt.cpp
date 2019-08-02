#include <eosio/chain/webassembly/wabt.hpp>
#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/wasm_eosio_constraints.hpp>

//wabt includes
#include <src/interp.h>
#include <src/binary-reader-interp.h>
#include <src/error-formatter.h>

namespace eosio { namespace chain { namespace webassembly { namespace wabt_runtime {

//yep 🤮
static wabt_apply_instance_vars* static_wabt_vars;

using namespace wabt;
using namespace wabt::interp;
namespace wasm_constraints = eosio::chain::wasm_constraints;

class wabt_instantiated_module : public wasm_instantiated_module_interface {
   public:
      wabt_instantiated_module(std::unique_ptr<interp::Environment> e, std::vector<uint8_t> initial_mem, interp::DefinedModule* mod) :
         _env(move(e)), _instatiated_module(mod), _initial_memory(initial_mem),
         _executor(_env.get(), nullptr, Thread::Options(64*1024,
                                                        wasm_constraints::maximum_call_depth+2))
      {
         for(Index i = 0; i < _env->GetGlobalCount(); ++i) {
            if(_env->GetGlobal(i)->mutable_ == false)
               continue;
            _initial_globals.emplace_back(_env->GetGlobal(i), _env->GetGlobal(i)->typed_value);
         }

         if(_env->GetMemoryCount())
            _initial_memory_configuration = _env->GetMemory(0)->page_limits;
      }

      void apply(apply_context& context) override {
         //reset mutable globals
         for(const auto& mg : _initial_globals)
            mg.first->typed_value = mg.second;

         wabt_apply_instance_vars this_run_vars{nullptr, context};
         static_wabt_vars = &this_run_vars;

         //reset memory to inital size & copy back in initial data
         if(_env->GetMemoryCount()) {
            Memory* memory = this_run_vars.memory = _env->GetMemory(0);
            memory->page_limits = _initial_memory_configuration;
            memory->data.resize(_initial_memory_configuration.initial * WABT_PAGE_SIZE);
            memcpy(memory->data.data(), _initial_memory.data(), _initial_memory.size());
            memset(memory->data.data() + _initial_memory.size(), 0, memory->data.size() - _initial_memory.size());
         }

         _params[0].set_i64(context.get_receiver().to_uint64_t());
         _params[1].set_i64(context.get_action().account.to_uint64_t());
         _params[2].set_i64(context.get_action().name.to_uint64_t());

         ExecResult res = _executor.RunStartFunction(_instatiated_module);
         EOS_ASSERT( res.result == interp::Result::Ok, wasm_execution_error, "wabt start function failure (${s})", ("s", ResultToString(res.result)) );

         res = _executor.RunExportByName(_instatiated_module, "apply", _params);
         EOS_ASSERT( res.result == interp::Result::Ok, wasm_execution_error, "wabt execution failure (${s})", ("s", ResultToString(res.result)) );
      }

   private:
      std::unique_ptr<interp::Environment>              _env;
      DefinedModule*                                    _instatiated_module;  //this is owned by the Environment
      std::vector<uint8_t>                              _initial_memory;
      TypedValues                                       _params{3, TypedValue(Type::I64)};
      std::vector<std::pair<Global*, TypedValue>>       _initial_globals;
      Limits                                            _initial_memory_configuration;
      Executor                                          _executor;
};

wabt_runtime::wabt_runtime() {}

std::unique_ptr<wasm_instantiated_module_interface> wabt_runtime::instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t> initial_memory) {
   std::unique_ptr<interp::Environment> env = std::make_unique<interp::Environment>();
   for(auto it = intrinsic_registrator::get_map().begin() ; it != intrinsic_registrator::get_map().end(); ++it) {
      interp::HostModule* host_module = env->AppendHostModule(it->first);
      for(auto itf = it->second.begin(); itf != it->second.end(); ++itf) {
         host_module->AppendFuncExport(itf->first, itf->second.sig, [fn=itf->second.func](const auto* f, const auto* fs, const auto& args, auto& res) {
            TypedValue ret = fn(*static_wabt_vars, args);
            if(ret.type != Type::Void)
               res[0] = ret;
            return interp::Result::Ok;
         });
      }
   }

   interp::DefinedModule* instantiated_module = nullptr;
   wabt::Errors errors;

   wabt::Result res = ReadBinaryInterp(env.get(), code_bytes, code_size, read_binary_options, &errors, &instantiated_module);
   EOS_ASSERT( Succeeded(res), wasm_execution_error, "Error building wabt interp: ${e}", ("e", wabt::FormatErrorsToString(errors, Location::Type::Binary)) );

   return std::make_unique<wabt_instantiated_module>(std::move(env), initial_memory, instantiated_module);
}

void wabt_runtime::immediately_exit_currently_running_module() {
   throw wasm_exit();
}

digest_type calc_memory_hash( const Memory& mem ) {
   digest_type::encoder enc;
   enc.write( mem.data.data(), mem.data.size() );
   return enc.result();
}

}}}}
