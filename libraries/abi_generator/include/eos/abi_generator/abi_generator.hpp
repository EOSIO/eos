#pragma once

#include <set>
#include <regex>
#include <algorithm>

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

#include <eos/types/abi_serializer.hpp>
#include <eos/types/types.hpp>
#include <fc/io/json.hpp>

using namespace clang;
using namespace std;
using namespace clang::tooling;
namespace cl = llvm::cl;

namespace eosio { namespace abi_generator {

FC_DECLARE_EXCEPTION( abi_generation_exception, 999999, "Unable to generate abi" );

#define ABI_ASSERT( TEST, ... ) \
  FC_EXPAND_MACRO( \
    FC_MULTILINE_MACRO_BEGIN \
      if( UNLIKELY(!(TEST)) ) \
      {                                                                      \
        if( fc::enable_record_assert_trip )                                  \
           fc::record_assert_trip( __FILE__, __LINE__, #TEST );              \
        FC_THROW_EXCEPTION( eosio::abi_generator::abi_generation_exception, #TEST ": "  __VA_ARGS__ ); \
      }                                                                      \
    FC_MULTILINE_MACRO_END \
  )

/**
* @brief Generates eosio::types::abi struct handling events from ASTConsumer
*/
class abi_generator {

  bool                   verbose;
  int                    optimizations;
  types::abi*            output;
  CompilerInstance*      compiler_instance;
  map<string, uint64_t>  type_size;
  map<string, string>    full_types;
  string                 abi_context;

  string remove_namespace(const string& full_name) {
    string type_name = full_name;
    auto pos = type_name.find_last_of("::");
    if(pos != string::npos)
      type_name = type_name.substr(pos+1);
    return type_name;
  }

  bool is_builtin_type(const string& type_name) {
    types::abi_serializer serializer;
    auto rtype = resolve_type(type_name);
    return serializer.is_builtin_type(translate_type(rtype));
  }

  string translate_type(const string& type_name) {
    string built_in_type = type_name;

    if (type_name == "unsigned __int128" || type_name == "uint128_t") built_in_type = "uint128";
    else if (type_name == "__int128"          || type_name == "int128_t")  built_in_type = "unt128";

    else if (type_name == "unsigned long long" || type_name == "uint64_t") built_in_type = "uint64";
    else if (type_name == "unsigned long"      || type_name == "uint32_t") built_in_type = "uint32";
    else if (type_name == "unsigned short"     || type_name == "uint16_t") built_in_type = "uint16";
    else if (type_name == "unsigned char"      || type_name == "uint8_t")  built_in_type = "uint8";
    
    else if (type_name == "long long"          || type_name == "int64_t")  built_in_type = "int64";
    else if (type_name == "long"               || type_name == "int32_t")  built_in_type = "int32";
    else if (type_name == "short"              || type_name == "int16_t")  built_in_type = "int16";
    else if (type_name == "char"               || type_name == "int8_t")   built_in_type = "int8";

    return built_in_type;
  }

  void handle_decl(const Decl* decl) { try {

    ABI_ASSERT(decl != nullptr);
    ABI_ASSERT(output != nullptr);

    ASTContext& ctx = decl->getASTContext();
    const RawComment* raw_comment = ctx.getRawCommentForDeclNoCache(decl);

    if(!raw_comment) return;

    SourceManager& source_manager = ctx.getSourceManager();
    auto file_name = source_manager.getFilename(raw_comment->getLocStart());
    if ( !abi_context.empty() && !file_name.startswith(abi_context) ) {
      return;
    }

    string raw_text = raw_comment->getRawText(source_manager);
    regex r(R"(@abi (action|table)((?: [a-z0-9]+)*))");

    smatch smatch;
    while(regex_search(raw_text, smatch, r))
    {
      if(smatch.size() == 3) {

        auto type = smatch[1].str();
        
        vector<string> params;
        auto string_params = smatch[2].str();
        boost::trim(string_params);
        if(!string_params.empty())
          boost::split(params, string_params, boost::is_any_of(" "));

        if(type == "action") {

          const auto* action_decl = dyn_cast<CXXRecordDecl>(decl);
          ABI_ASSERT(action_decl != nullptr);

          auto qt = action_decl->getTypeForDecl()->getCanonicalTypeInternal();
          
          auto type_name = remove_namespace(add_struct(qt));
          ABI_ASSERT(!is_builtin_type(type_name), 
            "A built-in type with the same name exists, try using another name: ${type_name}", ("type_name",type_name));

          if(params.size()==0) {
            params.push_back( boost::algorithm::to_lower_copy(boost::erase_all_copy(type_name, "_")) );
          }

          for(const auto& action : params) {
            const auto* ac = find_action(action);  
            if( ac ) {
              ABI_ASSERT(ac->type == type_name, "Same action name with different type ${action}",("action",action));
              continue;
            }
            output->actions.push_back({action, type_name});
          }

        } else if (type == "table") {

          const auto* table_decl = dyn_cast<CXXRecordDecl>(decl);
          ABI_ASSERT(table_decl != nullptr);

          auto qt = table_decl->getTypeForDecl()->getCanonicalTypeInternal();
          auto type_name = remove_namespace(add_struct(qt));

          ABI_ASSERT(!is_builtin_type(type_name), 
            "A built-in type with the same name exists, try using another name: ${type_name}", ("type_name",type_name));

          const auto* s = find_struct(type_name);
          ABI_ASSERT(s, "Unable to find type ${type}", ("type",type_name));

          types::table table;
          table.table_name = boost::algorithm::to_lower_copy(boost::erase_all_copy(type_name, "_"));
          table.type = type_name;
          
          if(params.size() >= 1)
            table.index_type = params[0];
          else { try {
            guess_index_type(table, *s);
          } FC_CAPTURE_AND_RETHROW( (type_name) ) }

          if(params.size() >= 2) {
            table.table_name = params[1];
          }
          
          try {
            guess_key_names(table, *s);
          } FC_CAPTURE_AND_RETHROW( (type_name) )

          //TODO: assert that we are adding the same table
          const auto* ta = find_table(table.table_name);
          if(!ta) {
            output->tables.push_back(table);  
          }
        }
      }
    
      raw_text = smatch.suffix();
    } 

  } FC_CAPTURE_AND_RETHROW() }

  bool is_64bit(const string& type) {
    return type_size[type] == 64;
  }

  bool is_128bit(const string& type) {
    return type_size[type] == 128;
  }

  bool is_string(const string& type) {
    return type == "String" || type == "string";
  }

  void get_all_fields(const types::struct_t& s, vector<types::field>& fields) {
    types::abi_serializer abis(*output);
    for(const auto& field : s.fields) {
      fields.push_back(field);
    }
    if(s.base.size()) {
      const auto* base = find_struct(s.base);
      ABI_ASSERT(base, "Unable to find base type ${type}",("type",s.base));
      get_all_fields(*base, fields);
    }
  }

  bool is_i64i64i64_index(const vector<types::field>& fields) {
    return fields.size() >= 3 && is_64bit(fields[0].type) && is_64bit(fields[1].type) && is_64bit(fields[2].type);
  }

  bool is_i64_index(const vector<types::field>& fields) {
    return fields.size() >= 1 && is_64bit(fields[0].type);
  }

  bool is_i128i128_index(const vector<types::field>& fields) {
    return fields.size() >= 2 && is_128bit(fields[0].type) && is_128bit(fields[1].type);
  }

  bool is_str_index(const vector<types::field>& fields) {
    return fields.size() == 2 && is_string(fields[0].type) && fields[0].name == "key" && fields[1].name == "value"; 
  }

  void guess_index_type(types::table& table, const types::struct_t s) {

    vector<types::field> fields;
    get_all_fields(s, fields);
    
    if( is_str_index(fields) ) {
      table.index_type = "str";
    } else if ( is_i64i64i64_index(fields) ) {
      table.index_type = "i64i64i64";
    } else if( is_i128i128_index(fields) ) {
      table.index_type = "i128i128";
    } else if( is_i64_index(fields) ) {
      table.index_type = "i64";
    } else {
      ABI_ASSERT(false, "Unable to guess index type");
    }
  }

  void guess_key_names(types::table& table, const types::struct_t s) {

    vector<types::field> fields;
    get_all_fields(s, fields);

    if( table.index_type == "i64i64i64" && is_i64i64i64_index(fields) ) {
      table.key_names  = vector<types::field_name>{fields[0].name, fields[1].name, fields[2].name};
      table.key_types  = vector<types::type_name>{fields[0].type, fields[1].type, fields[2].type};
    } else if( table.index_type == "i128i128" && is_i128i128_index(fields) ) {
      table.key_names  = vector<types::field_name>{fields[0].name, fields[1].name};
      table.key_types  = vector<types::type_name>{fields[0].type, fields[1].type};
    } else if( table.index_type == "i64" && is_i64_index(fields) ) {
      table.key_names  = vector<types::field_name>{fields[0].name};
      table.key_types  = vector<types::type_name>{fields[0].type};
    } else if( table.index_type == "str" && is_str_index(fields) ) {
      table.key_names  = vector<types::field_name>{fields[0].name};
      table.key_types  = vector<types::type_name>{fields[0].type};
    } else {
      ABI_ASSERT(false, "Unable to guess key names");
    }
  }

  const types::table* find_table(const types::name& name) {
    for( const auto& ta : output->tables ) {
      if(ta.table_name == name) {
        return &ta;
      }
    }
    return nullptr;
  }

  const types::type_def* find_type(const types::type_name& new_type_name) {
    for( const auto& td : output->types ) {
      if(td.new_type_name == new_type_name) {
        return &td;
      }
    }
    return nullptr;
  }

  const types::action* find_action(const types::name& name) {
    for( const auto& ac : output->actions ) {
      if(ac.action_name == name) {
        return &ac;
      }
    }
    return nullptr;
  }

  const types::struct_t* find_struct(const types::type_name& name) {
    auto rname = resolve_type(name);
    for( const auto& st : output->structs ) {
      if(st.name == rname) {
        return &st;
      }
    }
    return nullptr;
  }

  types::type_name resolve_type(const types::type_name& type){
    const auto* td = find_type(type);
    if( td ) {
      return resolve_type(td->type);
    }
    return type;
  }

  bool is_one_filed_no_base(const string& type_name) {
    const auto* s = find_struct(type_name);
    return s && s->base.size() == 0 && s->fields.size() == 1;
  }
  
  string decl_to_string(clang::Decl *d) {
      ASTContext& ctx = d->getASTContext();
      const auto& sm = ctx.getSourceManager();
      clang::SourceLocation b(d->getLocStart()), _e(d->getLocEnd());
      clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, sm, compiler_instance->getLangOpts()));
      return string(sm.getCharacterData(b),
          sm.getCharacterData(e)-sm.getCharacterData(b));
  }

  void add_typedef(const clang::TypedefType* typeDef) {

    vector<types::type_def> types_to_add;
    while(typeDef != nullptr) {
      const auto* typedef_decl = typeDef->getDecl();
      auto qt = typedef_decl->getUnderlyingType().getUnqualifiedType();

      auto full_name = translate_type(qt.getAsString(compiler_instance->getLangOpts()));

      //HACK: We need to think another way to stop importing the "typedef chain"
      if( full_name.find("<") != string::npos ) {
        break;
      }

      auto new_type_name = translate_type(typedef_decl->getName().str());
      if(is_builtin_type(new_type_name)) {
        break;
      }

      //TODO: ABI_ASSERT TypeName length for typedef
      types::type_def abi_typedef;
      abi_typedef.new_type_name = new_type_name;
      abi_typedef.type = remove_namespace( full_name );

      const auto* td = find_type(abi_typedef.new_type_name);
      if( td ) {
        ABI_ASSERT(abi_typedef.type == td->type);
        typeDef = qt->getAs<clang::TypedefType>();
        continue;
      }

      //ilog("addTypeDef => ${a} ${b} [${c}]", ("a",abi_typedef.new_type_name)("b",abi_typedef.type)("c",getFullScope(qt)));
      ABI_ASSERT(abi_typedef.new_type_name != abi_typedef.type, 
          "Unable to export typedef `${td}` at ${at}",
          ("td",decl_to_string(typeDef->getDecl()))
          ("at",typeDef->getDecl()->getLocation().printToString(typeDef->getDecl()->getASTContext().getSourceManager()))
      );

      types_to_add.push_back(abi_typedef);
      typeDef = qt->getAs<clang::TypedefType>();
    }

    std::reverse(types_to_add.begin(), types_to_add.end());
    output->types.insert(output->types.end(), types_to_add.begin(), types_to_add.end());
  }

  string get_name_to_add(const clang::QualType& qual_type) {

    auto type_name = qual_type.getAsString(compiler_instance->getLangOpts());
    const auto* typedef_type = qual_type->getAs<clang::TypedefType>();
    if( !is_builtin_type(type_name) && typedef_type != nullptr) {
      add_typedef(typedef_type);
    } 

    return type_name;
  }

  string add_struct(const clang::QualType& qual_type) {

    ABI_ASSERT(output != nullptr);

    const auto* record_type = qual_type->getAs<clang::RecordType>();
    ABI_ASSERT(record_type != nullptr);
    
    const auto* record_decl = clang::cast_or_null<clang::CXXRecordDecl>(record_type->getDecl()->getDefinition());
    ABI_ASSERT(record_decl != nullptr);

    ASTContext& ctx = record_decl->getASTContext();

    auto full_name = get_name_to_add(qual_type);

    auto name = remove_namespace(full_name);

    //Only export user defined types
    if( is_builtin_type(name) ) {
      return name;
    }

    const auto* type = qual_type.getTypePtr();

    ABI_ASSERT(type->isStructureType() || type->isClassType(), "Only struct and class are supported. ${full_name}",("full_name",full_name));

    ABI_ASSERT(name.size() <= sizeof(types::type_name),
      "Type name > ${maxsize}, ${name}",
      ("type",full_name)("name",name)("maxsize",sizeof(types::type_name)));

    if( find_struct(name) ) {
      auto itr = full_types.find(resolve_type(name));
      if(itr != full_types.end()) {
        ABI_ASSERT(itr->second == full_name, "Unable to add type '${full_name}' because '${conflict}' is already in.\n${t}", ("full_name",full_name)("conflict",itr->second)("t",output->types));
      }
      return name;
    }

    auto bases = record_decl->bases();
    auto total_bases = distance(bases.begin(), bases.end());
    if( total_bases > 1 ) {
      ABI_ASSERT(false, "Multiple inheritance not supported - ${type}", ("type",full_name));
    }

    string base_name;
    if( total_bases == 1 ) {
      auto qt = bases.begin()->getType();
      base_name = add_struct(qt);
    }

    types::struct_t abi_struct;
    for (const auto& field : record_decl->fields()) {
      
      types::field struct_field;

      clang::QualType qt = field->getType();

      auto field_type = translate_type(remove_namespace(get_name_to_add(qt.getUnqualifiedType())));

      ABI_ASSERT(field_type.size() <= sizeof(decltype(struct_field.type)),
        "Type name > ${maxsize}, ${type}::${name}", ("type",full_name)("name",field_type)("maxsize",sizeof(decltype(struct_field.type))));

      ABI_ASSERT(field->getNameAsString().size() <= sizeof(decltype(struct_field.name)) , 
        "Field name > ${maxsize}, ${type}::${name}", ("type",full_name)("name",field->getNameAsString())("maxsize", sizeof(decltype(struct_field.name))));
      
      if( qt->getAs<clang::RecordType>() ) {
        add_struct(qt);
        //TODO: simplify if OPT_SINGLE_FIELD_STRUCT is enabled
      }
      
      struct_field.name = field->getNameAsString();
      struct_field.type = field_type;

      ABI_ASSERT(is_builtin_type(struct_field.type) || find_struct(struct_field.type), "Unknown type ${type} ${name} ${ttt} ${sss}", ("type",struct_field.type)("name",struct_field.name)("types", output->types)("structs",output->structs));

      type_size[string(struct_field.type)] = ctx.getTypeSize(qt);
      abi_struct.fields.push_back(struct_field);
    }

    abi_struct.name = resolve_type(name);
    abi_struct.base = remove_namespace(base_name);
    
    output->structs.push_back(abi_struct);

    if(verbose) {    
     cerr << "Adding type " << resolve_type(name) << " (" << full_name << ")\n";
    }

    full_types[name] = full_name;
    return full_name;
  }

public:
  
  enum optimization {
    OPT_SINGLE_FIELD_STRUCT
  };

  abi_generator() : optimizations(0), output(nullptr), compiler_instance(nullptr) {  
  }

  ~abi_generator() {}

 /**
  * @brief Enable optimization when generating ABI
  * @param o optimization to enable
  */
  void enable_optimizaton(optimization o) {
    optimizations |= o;
  } 

 /**
  * @brief Check if an optimization is enabled
  * @param o optimization to check
  */
  bool is_opt_enabled(optimization o) {
    return (optimizations & o) != 0;
  }

 /**
  * @brief Set the destination ABI struct to write
  * @param output ABI destination
  */
  void set_output(types::abi& output) {
    this->output = &output;
  }

 /**
  * @brief Enable/Disable verbose status messages
  * @param verbose enable/disable flag 
  */
  void set_verbose(bool verbose) {
    this->verbose = verbose;
  }

 /**
  * @brief Set the root folder that limits where types will be imported. Types declared in header files located in child sub-folders will also be exported
  * @param abi_context folder 
  */
  void set_abi_context(const string& abi_context) {
    this->abi_context = abi_context;
  }
 
 /**
  * @brief Set the single instance of the Clang compiler
  * @param compiler_instance compiler instance 
  */
  void set_compiler_instance(CompilerInstance& compiler_instance) {
    this->compiler_instance = &compiler_instance;
  }

  /**
  * @brief Handle declaration of struct/union/enum
  * @param tag_decl declaration to handle
  */
  void handle_tagdecl_definition(TagDecl* tag_decl) { 
    ASTContext& ctx = tag_decl->getASTContext();
    auto decl_location = tag_decl->getLocation().printToString(ctx.getSourceManager());
    try {    
    handle_decl(tag_decl);
  } FC_CAPTURE_AND_RETHROW((decl_location)) }


};

struct abi_generator_astconsumer : public ASTConsumer {

  abi_generator& abi_gen;

  abi_generator_astconsumer(CompilerInstance& compiler_instance, abi_generator& abi_gen) : abi_gen(abi_gen) {
    abi_gen.set_compiler_instance(compiler_instance);
  }

  void HandleTagDeclDefinition(TagDecl* tag_decl) override {
    abi_gen.handle_tagdecl_definition(tag_decl);
  }

};

class generate_abi_action : public ASTFrontendAction {
  set<string> parsed_templates;
  abi_generator abi_gen;
public:
  generate_abi_action(bool verbose, bool opt_sfs, string abi_context, types::abi& output) {
    abi_gen.set_output(output);
    abi_gen.set_verbose(verbose);
    abi_gen.set_abi_context(abi_context);

    if(opt_sfs)
      abi_gen.enable_optimizaton(abi_generator::OPT_SINGLE_FIELD_STRUCT);
  }

protected:
  unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& compiler_instance,
                                                 llvm::StringRef) override {
    return llvm::make_unique<abi_generator_astconsumer>(compiler_instance, abi_gen);
  }

};

} } //ns eosio::abi_generator
