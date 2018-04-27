#pragma once

#include <set>
#include <regex>
#include <algorithm>
#include <memory>
#include <map>
#include <fstream>
#include <sstream>

#include <eosio/chain/contracts/abi_serializer.hpp>
#include <eosio/chain/contracts/types.hpp>
#include <fc/io/json.hpp>

//clashes with something deep in the AST includes in clang 6 and possibly other versions of clang
#pragma push_macro("N")
#undef N

#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Sema/Sema.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/MacroArgs.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Core/QualTypeNames.h"
#include "llvm/Support/raw_ostream.h"
#include <boost/algorithm/string.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

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
   
   class ricardian_contracts {
      public:
         ricardian_contracts() = default;
         ricardian_contracts( const string& context, const string& contract, const vector<string>& actions ) {
            ifstream clauses_file( context+"/"+contract+"_rc.md");
            if ( !clauses_file.good() )
               wlog("Warning, no ricardian clauses found for ${con}\n", ("con", contract));
            else
               parse_clauses( clauses_file );

            for ( auto act : actions ) {
               ifstream contract_file( context+"/"+contract+"."+act+"_rc.md" );
               if ( !contract_file.good() )
                  wlog("Warning, no ricardian contract found for ${act}\n", ("act", act));
               else {
                  parse_contract( contract_file );
               }
            }   
         }

         vector<clause_pair> get_clauses() {
            return _clauses;
         }
         string operator[]( string key ) {
            return _contracts[key];
         }
      private:
         inline string is_clause_decl( string line ) {
            smatch match;
            if ( regex_match( line, match, regex("(###[ ]+CLAUSE[ ]+NAME[ ]*:[ ]*)(.*)", regex_constants::ECMAScript) ) ) {
               FC_ASSERT( match.size() == 3, "Error, malformed clause declaration" );
               return match[2].str();
            }
            return {};
         }

         inline string is_action_decl( string line ) {
            smatch match;
            if ( regex_match( line, match, regex("(##[ ]+ACTION[ ]+NAME[ ]*:[ ]*)(.*)", regex_constants::ECMAScript) ) ) {
               FC_ASSERT( match.size() == 3, "Error, malformed action declaration" );
               return match[2].str();
            }
            return {};
         }

         void parse_contract( ifstream& contract_file ) {
            string       line;
            string       name;
            string       _name;
            stringstream body;
            bool first_time = true;
            while ( contract_file.peek() != EOF ) {
               getline( contract_file, line );
               body << line;
               if ( !(_name = is_action_decl( line )).empty() ) {
                  name = _name;
                  first_time = false;
               }
               else
                  if ( !first_time )
                     body << line << '\n';
            }

            _contracts.emplace(name, body.str());
         }

         void parse_clauses( ifstream& clause_file ) {
            string        line;
            string        name;
            string        _name;
            stringstream  body;
            bool first_time = true;
            while ( clause_file.peek() != EOF ) {
               getline( clause_file, line );

               if ( !(_name = is_clause_decl( line )).empty() ) {
                  if ( !first_time ) {
                     if (body.str().empty() ) {
                        FC_ASSERT( false, "Error, invalid input in ricardian clauses, no body found" );
                     }
                     _clauses.emplace_back( name, body.str() );
                     body.str("");
                  }
                  name = _name;
                  first_time = false;
               }
               else
                  if ( !first_time )
                     body << line << '\n';
            }

         }
         vector<clause_pair> _clauses;
         map<string, string> _contracts;
   };

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
         clang::ASTContext*     ast_context;   
         string                 target_contract;
         vector<string>         target_actions;
         ricardian_contracts    rc;
      public:

         enum optimization {
            OPT_SINGLE_FIELD_STRUCT
         };

         abi_generator()
         :optimizations(0)
         , output(nullptr)
         , compiler_instance(nullptr)
         , ast_context(nullptr)
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
          * @brief Set the ricardian_contracts object with parsed contracts and clauses 
          * @param ricardian_contracts contracts
          */
         void set_ricardian_contracts(const ricardian_contracts& contracts);


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

         void set_target_contract(const string& contract, const vector<string>& actions);

      private:
         bool inspect_type_methods_for_actions(const Decl* decl);

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

         bool is_typedef(const clang::QualType& qt);
         QualType add_typedef(const clang::QualType& qt);

         bool is_vector(const clang::QualType& qt);
         bool is_vector(const string& type_name);
         string add_vector(const clang::QualType& qt);

         bool is_struct(const clang::QualType& qt);
         string add_struct(const clang::QualType& qt, string full_type_name="");

         string get_type_name(const clang::QualType& qt, bool no_namespace);
         string add_type(const clang::QualType& tqt);

         bool is_elaborated(const clang::QualType& qt);
         bool is_struct_specialization(const clang::QualType& qt);

         QualType get_vector_element_type(const clang::QualType& qt);
         string get_vector_element_type(const string& type_name);

         clang::QualType get_named_type_if_elaborated(const clang::QualType& qt);

         const clang::RecordDecl::field_range get_struct_fields(const clang::QualType& qt);
         clang::CXXRecordDecl::base_class_range get_struct_bases(const clang::QualType& qt);
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

   struct find_eosio_abi_macro_action : public PreprocessOnlyAction {

         string& contract;
         vector<string>& actions;
         const string& abi_context;

         find_eosio_abi_macro_action(string& contract, vector<string>& actions, const string& abi_context
            ): contract(contract),
            actions(actions), abi_context(abi_context) {
         }

         struct callback_handler : public PPCallbacks {

            CompilerInstance& compiler_instance;
            find_eosio_abi_macro_action& act;

            callback_handler(CompilerInstance& compiler_instance, find_eosio_abi_macro_action& act)
            : compiler_instance(compiler_instance), act(act) {}

            void MacroExpands (const Token &token, const MacroDefinition &md, SourceRange range, const MacroArgs *args) override {

               auto* id = token.getIdentifierInfo();
               if( id == nullptr ) return;
               if( id->getName() != "EOSIO_ABI" ) return;

               const auto& sm = compiler_instance.getSourceManager();
               auto file_name = sm.getFilename(range.getBegin());
               if ( !act.abi_context.empty() && !file_name.startswith(act.abi_context) ) {
                  return;
               }

               ABI_ASSERT( md.getMacroInfo()->getNumArgs() == 2 );

               clang::SourceLocation b(range.getBegin()), _e(range.getEnd());
               clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, sm, compiler_instance.getLangOpts()));
               auto macrostr = string(sm.getCharacterData(b), sm.getCharacterData(e)-sm.getCharacterData(b));

               regex r(R"(EOSIO_ABI\s*\(\s*(.+?)\s*,((?:.+?)*)\s*\))");
               smatch smatch;
               auto res = regex_search(macrostr, smatch, r);
               ABI_ASSERT( res );

               act.contract = smatch[1].str();

               auto actions_str = smatch[2].str();
               boost::trim(actions_str);
               actions_str = actions_str.substr(1);
               actions_str.pop_back();
               boost::remove_erase_if(actions_str, boost::is_any_of(" ("));

               boost::split(act.actions, actions_str, boost::is_any_of(")"));
            }
         };

         void ExecuteAction() override {
            getCompilerInstance().getPreprocessor().addPPCallbacks(
               llvm::make_unique<callback_handler>(getCompilerInstance(), *this)
            );
            PreprocessOnlyAction::ExecuteAction();
         };

   };

  
   class generate_abi_action : public ASTFrontendAction {

      private:
         set<string> parsed_templates;
         abi_generator abi_gen;

      public:

         generate_abi_action(bool verbose, bool opt_sfs, string abi_context,
                             abi_def& output, const string& contract, const vector<string>& actions) {
            
            ricardian_contracts rc( abi_context, contract, actions );
            abi_gen.set_output(output);
            abi_gen.set_verbose(verbose);
            abi_gen.set_abi_context(abi_context);
            abi_gen.set_target_contract(contract, actions);
            abi_gen.set_ricardian_contracts( rc );
            output.ricardian_clauses = rc.get_clauses();
          
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

#pragma pop_macro("N")
