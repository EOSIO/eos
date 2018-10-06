/**
 *  @file
 *  @copyright defined in arisen/LICENSE.txt
 */
#include <arisen/chain/wast_to_wasm.hpp>
#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>
#include <sstream>
#include <iomanip>
#include <fc/exception/exception.hpp>
#include <arisen/chain/exceptions.hpp>

namespace arisen { namespace chain {

   std::vector<uint8_t> wast_to_wasm( const std::string& wast ) 
   { 
      std::stringstream ss;
      
      try {
      IR::Module module;
      std::vector<WAST::Error> parse_errors;
      WAST::parseModule(wast.c_str(),wast.size(),module,parse_errors);
      if(parse_errors.size())
      {
         // Print any parse errors;
         ss << "Error parsing WebAssembly text file:" << std::endl;
         for(auto& error : parse_errors)
         {
            ss << ":" << error.locus.describe() << ": " << error.message.c_str() << std::endl;
            ss << error.locus.sourceLine << std::endl;
            ss << std::setw(error.locus.column(8)) << "^" << std::endl;
         }
         RSN_ASSERT( false, wasm_exception, "error parsing wast: ${msg}", ("msg",ss.str()) );
      }

      for(auto sectionIt = module.userSections.begin();sectionIt != module.userSections.end();++sectionIt)
      {
         if(sectionIt->name == "name") { module.userSections.erase(sectionIt); break; }
      }

      try
      {
         // Serialize the WebAssembly module.
         Serialization::ArrayOutputStream stream;
         WASM::serialize(stream,module);
         return stream.getBytes();
      }
      catch(const Serialization::FatalSerializationException& exception)
      {
         ss << "Error serializing WebAssembly binary file:" << std::endl;
         ss << exception.message << std::endl;
         RSN_ASSERT( false, wasm_exception, "error converting to wasm: ${msg}", ("msg",ss.get()) );
      } catch(const IR::ValidationException& e) {
         ss << "Error validating WebAssembly binary file:" << std::endl;
         ss << e.message << std::endl;
         RSN_ASSERT( false, wasm_exception, "error converting to wasm: ${msg}", ("msg",ss.get()) );
      }

   } FC_CAPTURE_AND_RETHROW( (wast) ) }  /// wast_to_wasm

   std::string     wasm_to_wast( const std::vector<uint8_t>& wasm, bool strip_names ) {
      return wasm_to_wast( wasm.data(), wasm.size(), strip_names );
   } /// wasm_to_wast

   std::string     wasm_to_wast( const uint8_t* data, uint64_t size, bool strip_names ) 
   { try {
       IR::Module module;
       Serialization::MemoryInputStream stream((const U8*)data,size);
       WASM::serialize(stream,module);
       if(strip_names)
          module.userSections.clear();
        // Print the module to WAST.
       return WAST::print(module);
   } FC_CAPTURE_AND_RETHROW() } /// wasm_to_wast


} } // arisen::chain
