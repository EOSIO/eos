#include <eosio/chain/webassembly/binaryen.hpp>
#include <eosio/chain/apply_context.hpp>

#include <wasm-binary.h>


namespace eosio { namespace chain { namespace webassembly { namespace binaryen {

class binaryen_instantiated_module : public wasm_instantiated_module_interface {
   public:
      binaryen_instantiated_module(linear_memory_type& shared_linear_memory,
                                   std::vector<uint8_t> initial_memory,
                                   call_indirect_table_type table,
                                   import_lut_type import_lut,
                                   unique_ptr<Module>&& module) :
         _shared_linear_memory(shared_linear_memory),
         _initial_memory(initial_memory),
         _table(forward<decltype(table)>(table)),
         _import_lut(forward<decltype(import_lut)>(import_lut)),
         _module(forward<decltype(module)>(module)) {

      }

      void apply(apply_context& context) override {
         LiteralList args = {Literal(uint64_t(context.receiver)),
	                     Literal(uint64_t(context.act.account)),
                             Literal(uint64_t(context.act.name))};
         call("apply", args, context);
      }

   private:
      linear_memory_type&        _shared_linear_memory;      
      std::vector<uint8_t>       _initial_memory;
      call_indirect_table_type   _table;
      import_lut_type            _import_lut;
      unique_ptr<Module>          _module;

      void call(const string& entry_point, LiteralList& args, apply_context& context){
         const unsigned initial_memory_size = _module->memory.initial*Memory::kPageSize;
         interpreter_interface local_interface(_shared_linear_memory, _table, _import_lut, initial_memory_size, context);

         //zero out the initial pages
         memset(_shared_linear_memory.data, 0, initial_memory_size);
         //copy back in the initial data
         memcpy(_shared_linear_memory.data, _initial_memory.data(), _initial_memory.size());
         
         //be aware that construction of the ModuleInstance implictly fires the start function
         ModuleInstance instance(*_module.get(), &local_interface);
         instance.callExport(Name(entry_point), args);
      }
};

binaryen_runtime::binaryen_runtime() {

}

std::unique_ptr<wasm_instantiated_module_interface> binaryen_runtime::instantiate_module(const char* code_bytes, size_t code_size, std::vector<uint8_t> initial_memory) {
   try {
      vector<char> code(code_bytes, code_bytes + code_size);
      unique_ptr<Module> module(new Module());
      WasmBinaryBuilder builder(*module, code, false);
      builder.read();

      FC_ASSERT(module->memory.initial * Memory::kPageSize <= wasm_constraints::maximum_linear_memory);

      // create a temporary globals to  use
      TrivialGlobalManager globals;
      for (auto& global : module->globals) {
         globals[global->name] = ConstantExpressionRunner<TrivialGlobalManager>(globals).visit(global->init).value;
      }

      call_indirect_table_type table;
      table.resize(module->table.initial);
      for (auto& segment : module->table.segments) {
         Address offset = ConstantExpressionRunner<TrivialGlobalManager>(globals).visit(segment.offset).value.geti32();
         FC_ASSERT(offset + segment.data.size() <= module->table.initial);
         for (size_t i = 0; i != segment.data.size(); ++i) {
            table[offset + i] = segment.data[i];
         }
      }

      // initialize the import lut
      import_lut_type import_lut;
      import_lut.reserve(module->imports.size());
      for (auto& import : module->imports) {
         std::string full_name = string(import->module.c_str()) + "." + string(import->base.c_str());
         if (import->kind == ExternalKind::Function) {
            auto& intrinsic_map = intrinsic_registrator::get_map();
            auto intrinsic_itr = intrinsic_map.find(full_name);
            if (intrinsic_itr != intrinsic_map.end()) {
               import_lut.emplace(make_pair((uintptr_t)import.get(), intrinsic_itr->second));
               continue;
            }
         }

         FC_ASSERT( !"unresolvable", "${module}.${export}", ("module",import->module.c_str())("export",import->base.c_str()) );
      }

      return std::make_unique<binaryen_instantiated_module>(_memory, initial_memory, move(table), move(import_lut), move(module));
   } catch (const ParseException &e) {
      FC_THROW_EXCEPTION(wasm_execution_error, "Error building interpreter: ${s}", ("s", e.text));
   }
}

}}}}
