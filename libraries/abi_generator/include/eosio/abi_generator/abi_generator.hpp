#pragma once

#include <set>
#include <regex>
#include <algorithm>
#include <memory>

#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "llvm/Support/raw_ostream.h"
#include <boost/algorithm/string.hpp>

#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain/contracts/types.hpp>
#include <fc/io/json.hpp>

using namespace clang;
using namespace std;
using namespace clang::tooling;
using namespace eosio::chain::contracts;
namespace cl = llvm::cl;

namespace eosio {

   FC_DECLARE_EXCEPTION( abi_generation_exception, 999999, "Unable to generate abi" );
   
   #define ABI_ASSERT( TEST, ... ) \
      FC_EXPAND_MACRO( \
       FC_MULTILINE_MACRO_BEGIN \
         if( UNLIKELY(!(TEST)) ) \
         {                                                                      \
           if( fc::enable_record_assert_trip )                                  \
              fc::record_assert_trip( __FILE__, __LINE__, #TEST );              \
           FC_THROW_EXCEPTION( eosio::abi_generation_exception, #TEST ": "  __VA_ARGS__ ); \
         }                                                                      \
       FC_MULTILINE_MACRO_END \
      )
   
   /**
     * @brief Generates eosio::abi_def struct handling events from ASTConsumer
     */
   class abi_generator {
      private: 
         bool                   verbose;
         int                    optimizations;
         abi_def*               output;
         CompilerInstance*      compiler_instance;
         map<string, uint64_t>  type_size;
         map<string, string>    full_types;
         string                 abi_context;
   
      public:
      
         enum optimization {
            OPT_SINGLE_FIELD_STRUCT
         };
   
         abi_generator()
         :optimizations(0)
         , output(nullptr)
         , compiler_instance(nullptr) 
         {}
      
         ~abi_generator() {}
   
         /**
           * @brief Enable optimization when generating ABI
           * @param o optimization to enable
           */
         void enable_optimizaton(optimization o);
   
         /**
           * @brief Check if an optimization is enabled
           * @param o optimization to check
           */
         bool is_opt_enabled(optimization o);
   
         /**
          * @brief Set the destination ABI struct to write
          * @param output ABI destination
          */
         void set_output(abi_def& output);

         /**
          * @brief Enable/Disable verbose status messages
          * @param verbose enable/disable flag
          */
         void set_verbose(bool verbose);

         /**
          * @brief Set the root folder that limits where types will be imported. Types declared in header files located in child sub-folders will also be exported
          * @param abi_context folder
          */
         void set_abi_context(const string& abi_context);

         /**
          * @brief Set the single instance of the Clang compiler
          * @param compiler_instance compiler instance
          */
         void set_compiler_instance(CompilerInstance& compiler_instance);

         /**
          * @brief Handle declaration of struct/union/enum
          * @param tag_decl declaration to handle
          */
         void handle_tagdecl_definition(TagDecl* tag_decl);

      private:

         string remove_namespace(const string& full_name);

         bool is_builtin_type(const string& type_name);

         string translate_type(const string& type_name);

         void handle_decl(const Decl* decl);

         bool is_64bit(const string& type);

         bool is_128bit(const string& type);

         bool is_string(const string& type);

         void get_all_fields(const struct_def& s, vector<field_def>& fields);

         bool is_i64i64i64_index(const vector<field_def>& fields);

         bool is_i64_index(const vector<field_def>& fields);

         bool is_i128i128_index(const vector<field_def>& fields);

         bool is_str_index(const vector<field_def>& fields);

         void guess_index_type(table_def& table, const struct_def s);

         void guess_key_names(table_def& table, const struct_def s);

         const table_def* find_table(const table_name& name);

         const type_def* find_type(const type_name& new_type_name);

         const action_def* find_action(const action_name& name);

         const struct_def* find_struct(const type_name& name);

         type_name resolve_type(const type_name& type);

         bool is_one_filed_no_base(const string& type_name);

         string decl_to_string(clang::Decl* d);

         void add_typedef(const clang::TypedefType* typeDef);

         string get_name_to_add(const clang::QualType& qual_type);

         string add_struct(const clang::QualType& qual_type);
   };
   
   struct abi_generator_astconsumer : public ASTConsumer {
      abi_generator& abi_gen;
   
      abi_generator_astconsumer(CompilerInstance& compiler_instance, abi_generator& abi_gen)
      :abi_gen(abi_gen)
      {
         abi_gen.set_compiler_instance(compiler_instance);
      }
   
      void HandleTagDeclDefinition(TagDecl* tag_decl) override {
         abi_gen.handle_tagdecl_definition(tag_decl);
      }
   };
   
   class generate_abi_action : public ASTFrontendAction {
      private:
         set<string> parsed_templates;
         abi_generator abi_gen;

      public:
         generate_abi_action(bool verbose, bool opt_sfs, string abi_context, abi_def& output) {
            abi_gen.set_output(output);
            abi_gen.set_verbose(verbose);
            abi_gen.set_abi_context(abi_context);

            if(opt_sfs)
               abi_gen.enable_optimizaton(abi_generator::OPT_SINGLE_FIELD_STRUCT);
         }

      protected:
         std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& compiler_instance,
                                                        llvm::StringRef) override {
            return llvm::make_unique<abi_generator_astconsumer>(compiler_instance, abi_gen);
         }
   };

} //ns eosio
