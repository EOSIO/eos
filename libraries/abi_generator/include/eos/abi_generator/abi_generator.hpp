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

#include <eos/types/AbiSerializer.hpp>
#include <eos/types/types.hpp>
#include <fc/io/json.hpp>

using namespace clang;
using namespace std;
using namespace clang::tooling;
namespace cl = llvm::cl;

namespace eos { namespace abi_generator {

FC_DECLARE_EXCEPTION( abi_generation_exception, 999999, "Unable to generate abi" );

#define ABI_ASSERT( TEST, ... ) \
  FC_EXPAND_MACRO( \
    FC_MULTILINE_MACRO_BEGIN \
      if( UNLIKELY(!(TEST)) ) \
      {                                                                      \
        if( fc::enable_record_assert_trip )                                  \
           fc::record_assert_trip( __FILE__, __LINE__, #TEST );              \
        FC_THROW_EXCEPTION( eos::abi_generator::abi_generation_exception, #TEST ": "  __VA_ARGS__ ); \
      }                                                                      \
    FC_MULTILINE_MACRO_END \
  )

struct AbiGenerator {

  enum optimization {
    OPT_SINGLE_FIELD_STRUCT
  };

  bool                      verbose;
  int                       optimizations;
  eos::types::Abi*          output;
  clang::ASTContext*        context;
  CompilerInstance*         compiler_instance;
  map<string, uint64_t>     type_size;
  map<string, string>       full_types;
  string                    abi_context;

  AbiGenerator() : optimizations(0), output(nullptr), compiler_instance(nullptr) {
  
  }

  void Initialize(clang::ASTContext& context) {
    this->context = &context;
  }

  ~AbiGenerator() {}

  void setOptimizaton(optimization o) {
    optimizations |= o;
  } 
  
  bool isEnabledOpt(optimization o) {
    return (optimizations & o) != 0;
  }

  void setOutput(eos::types::Abi& output) {
    this->output = &output;
  }

  void setVerbose(bool verbose) {
    this->verbose = verbose;
  }

  void setAbiContext(const string& abi_context) {
    this->abi_context = abi_context;
  }

  void setContext(clang::ASTContext& context) {
    this->context = &context;
  }

  void setCompilerInstance(CompilerInstance& compiler_instance) {
    this->compiler_instance = &compiler_instance;
  }
 
  string extractTypeNameOnly(const string& full_type_name) {
    string type_name = full_type_name;
    auto pos = type_name.find_last_of("::");
    
    if(pos != string::npos)
      type_name = type_name.substr(pos+1);
    
    pos = type_name.find("struct ");
    if(pos == 0)
      type_name = type_name.substr(pos+7);
    else {
      pos = type_name.find("class ");
      if(pos != string::npos)
        type_name = type_name.substr(pos+6);
    }
    return type_name;
  }

  bool isBuiltInType(const string& type_name) {
    eos::types::AbiSerializer serializer;
    return serializer.isBuiltInType(resolveType(type_name));
  }

  string translateType(const string& type_name) {
    string built_in_type = type_name;

    if      (type_name == "unsigned __int128" || type_name == "uint128_t") built_in_type = "UInt128";
    else if (type_name == "__int128"          || type_name == "int128_t")  built_in_type = "Int128";

    else if (type_name == "unsigned long long" || type_name == "uint64_t") built_in_type = "UInt64";
    else if (type_name == "unsigned long"      || type_name == "uint32_t") built_in_type = "UInt32";
    else if (type_name == "unsigned short"     || type_name == "uint16_t") built_in_type = "UInt16";
    else if (type_name == "unsigned char"      || type_name == "uint8_t")  built_in_type = "UInt8";
    
    else if (type_name == "long long"          || type_name == "int64_t")  built_in_type = "Int64";
    else if (type_name == "long"               || type_name == "int32_t")  built_in_type = "Int32";
    else if (type_name == "short"              || type_name == "int16_t")  built_in_type = "Int16";
    else if (type_name == "char"               || type_name == "int8_t")   built_in_type = "Int8";

    return built_in_type;
  }

  string getCanonicalTypeName(const Type* type) {
    clang::PrintingPolicy policy = compiler_instance->getLangOpts();
    return type->getCanonicalTypeInternal().getAsString(policy);
  }
  
  void handleTranslationUnit(ASTContext& context) {
    //ilog("AbiGenerator::handleTranslationUnit");
  }
  
  bool handleTopLevelDecl(DeclGroupRef decl_group) {
    for( auto it = decl_group.begin(); it != decl_group.end(); ++it ) {
      handleDecl(*it);        
    }
    return true;
  }

  void handleTagDeclDefinition(TagDecl* tag_decl) {
    handleDecl(tag_decl);
  }

  void handleDecl(const Decl* decl) { try {

    ABI_ASSERT(decl != nullptr);
    ABI_ASSERT(output != nullptr);

    ASTContext& ctx = decl->getASTContext();
    const RawComment* raw_comment = ctx.getRawCommentForDeclNoCache(decl);

    if(!raw_comment) return;

    SourceManager& source_manager = ctx.getSourceManager();
    string rawText = raw_comment->getRawText(source_manager);

    regex r(R"(@abi ([a-zA-Z0-9]+) (action|table|type)((?: [a-z0-9]+)*))");

    smatch smatch;
    while(regex_search(rawText, smatch, r))
    {
      if(smatch.size() == 4) {

        auto context = smatch[1].str();
        if(context != abi_context) return;

        auto type = smatch[2].str();
        
        vector<string> params;
        auto string_params = smatch[3].str();
        boost::trim(string_params);
        if(!string_params.empty())
          boost::split(params, string_params, boost::is_any_of(" "));

        if(type == "type") {
          
          const TypedefDecl* typeDefDecl = dyn_cast<TypedefDecl>(decl);
          ABI_ASSERT(typeDefDecl != nullptr);

          clang::QualType qt = typeDefDecl->getUnderlyingType().getUnqualifiedType();
          
          eos::types::TypeDef type_def;
          type_def.newTypeName = extractTypeNameOnly(typeDefDecl->getName());
          type_def.type = extractTypeNameOnly(qt.getAsString());

          eos::types::TypeDef td;
          if( findType(type_def.newTypeName, td) ) {
            ABI_ASSERT(type_def.type == td.type);
            continue;
          }

          // eos::types::Struct dummy;
          // ABI_ASSERT(isBuiltInType(type_def.type) || findStruct(type_def.type, dummy) || findType(type_def.newTypeName, td), "Unable to add TypeDef ${type} is unknown",("type",type_def.type));
          output->types.push_back(type_def);

        } else if(type == "action") {

          const auto* actionDecl = dyn_cast<CXXRecordDecl>(decl);
          ABI_ASSERT(actionDecl != nullptr);
          
          auto type_name = addStruct(actionDecl);

          if(params.size()==0) {
            params.push_back( boost::algorithm::to_lower_copy(type_name) );
          }

          for(const auto& action : params) {
            eos::types::Action ac; 
            if( findAction(action, ac) ) {
              ABI_ASSERT(ac.type == type_name, "Same action name with different type ${action}",("action",action));
              continue;
            } 

            output->actions.push_back({action, type_name});
          }

        } else if (type == "table") {

          const auto* tableDecl = dyn_cast<CXXRecordDecl>(decl);
          ABI_ASSERT(tableDecl != nullptr);

          auto type_name = addStruct(tableDecl);
          
          eos::types::Struct s;
          ABI_ASSERT(findStruct(type_name, s), "Unable to find type ${type}", ("type",type_name));

          eos::types::Table table;
          table.table = boost::algorithm::to_lower_copy(type_name);
          table.type = type_name;
          
          if(params.size() >= 1)
            table.indextype = params[0];
          else { try {
            guessIndexType(table, s);
          } FC_CAPTURE_AND_RETHROW( (type_name) ) }

          try {
            guessKeyNames(table, s);
          } FC_CAPTURE_AND_RETHROW( (type_name) )

          //TODO: assert that we are adding the same table
          eos::types::Table ta;
          if( !findTable(table.table, ta) ) {
            output->tables.push_back(table);  
          }

        }

      }
    
      rawText = smatch.suffix();
    } 

  } FC_CAPTURE_AND_RETHROW() }

  bool is64bit(const string& type) {
    return type_size[type] == 64;
  }

  bool is128bit(const string& type) {
    return type_size[type] == 128;
  }

  bool isString(const string& type) {
    return type == "String";
  }

  void getAllFields(const eos::types::Struct s, vector<eos::types::Field>& fields) {
    for(const auto& field : s.fields) {
      fields.push_back(field);
    }
    if(s.base.size()) {
      eos::types::Struct base;
      ABI_ASSERT(findStruct(s.base, base), "Unable to find base type ${type}",("type",s.base));
      getAllFields(base, fields);
    }
  }

  bool isi64i64i64(const vector<eos::types::Field>& fields) {
    return fields.size() >= 3 && is64bit(fields[0].type) && is64bit(fields[1].type) && is64bit(fields[2].type);
  }

  bool isi64(const vector<eos::types::Field>& fields) {
    return fields.size() >= 1 && is64bit(fields[0].type);
  }

  bool isi128i128(const vector<eos::types::Field>& fields) {
    return fields.size() >= 2 && is128bit(fields[0].type) && is128bit(fields[1].type);
  }

  bool isstr(const vector<eos::types::Field>& fields) {
    return fields.size() == 2 && isString(fields[0].type) && fields[0].name == "key" && fields[1].name == "value"; 
  }

  void guessIndexType(eos::types::Table& table, const eos::types::Struct s) {

    vector<eos::types::Field> fields;
    getAllFields(s, fields);

    if( isi64i64i64(fields) ) {
      table.indextype = "i64i64i64";
    } else if( isi128i128(fields) ) {
      table.indextype = "i128i128";
    } else if( isi64(fields) ) {
      table.indextype = "i64";
    } else if( isstr(fields) ) {
      table.indextype = "str";
    } else {
      ABI_ASSERT(false, "Unable to guess index type");
    }
  }

  void guessKeyNames(eos::types::Table& table, const eos::types::Struct s) {

    vector<eos::types::Field> fields;
    getAllFields(s, fields);

    if( table.indextype == "i64i64i64" && isi64i64i64(fields) ) {
      table.keynames  = vector<eos::types::FieldName>{fields[0].name, fields[1].name, fields[2].name};
      table.keytypes  = vector<eos::types::TypeName>{fields[0].type, fields[1].type, fields[2].type};
    } else if( table.indextype == "i128i128" && isi128i128(fields) ) {
      table.keynames  = vector<eos::types::FieldName>{fields[0].name, fields[1].name};
      table.keytypes  = vector<eos::types::TypeName>{fields[0].type, fields[1].type};
    } else if( table.indextype == "i64" && isi64(fields) ) {
      table.keynames  = vector<eos::types::FieldName>{fields[0].name};
      table.keytypes  = vector<eos::types::TypeName>{fields[0].type};
    } else if( table.indextype == "str" && isstr(fields) ) {
      table.keynames  = vector<eos::types::FieldName>{fields[0].name};
      table.keytypes  = vector<eos::types::TypeName>{fields[0].type};
    } else {
      ABI_ASSERT(false, "Unable to guess key names");
    }
  }

  bool findTable(const types::Name& name, eos::types::Table& table) {
    for( const auto& ta : output->tables ) {
      if(ta.table == name) {
        table = ta;
        return true;
      }
    }
    return false;
  }

  bool findType(const string& newTypeName, eos::types::TypeDef& type_def) {
    for( const auto& td : output->types ) {
      if(td.newTypeName == newTypeName) {
        type_def = td;
        return true;
      }
    }
    return false;
  }

  bool findAction(const string& name, eos::types::Action& action) {
    for( const auto& ac : output->actions ) {
      if(ac.action == name) {
        action = ac;
        return true;
      }
    }
    return false;
  }

  bool findStruct(const types::TypeName& name, eos::types::Struct& s) {
    auto rname = resolveType(name);
    //ilog("resolved => ${a} / ${b}", ("a",name)("b",rname));
    for( const auto& st : output->structs ) {
      if(st.name == rname) {
        s = st;
        return true;
      }
    }
    return false;
  }

  types::TypeName resolveType( const types::TypeName& type ){
    eos::types::TypeDef td;
    if ( findType(type, td) )
      return resolveType(td.type);
    return type;
  }

  bool isOneFieldNoBase(const string& type_name) {
    eos::types::Struct s;
    bool found = findStruct(type_name, s);
    return found && s.base.size() == 0 && s.fields.size() == 1;
  }

  string addStruct(const clang::CXXRecordDecl* recordDecl, string full_type_name=string()) {
    ABI_ASSERT(recordDecl != nullptr, "Null CXXRecordDecl");
    ABI_ASSERT(output != nullptr);

    //TypedefNameDecl *tt = recordDecl->getTypedefNameForAnonDecl();

    const auto* type = recordDecl->getTypeForDecl();

    if( full_type_name.empty() ) {
      full_type_name = getCanonicalTypeName(type);
    }
    
    ABI_ASSERT(type->isStructureType() || type->isClassType(), "Only struct and class are supported. ${full_type_name}",("full_type_name",full_type_name));

    auto name = extractTypeNameOnly(full_type_name);

    ABI_ASSERT(name.size() <= sizeof(eos::types::TypeName),
      "Type name > ${maxsize}, ${name}",
      ("type",full_type_name)("name",name)("maxsize",sizeof(eos::types::TypeName)));

    eos::types::Struct s;
    if( findStruct(name, s) ) {
      auto itr = full_types.find(name);
      if(itr != full_types.end()) {
        ABI_ASSERT(itr->second == full_type_name, "Unable to add type '${full_type_name}' because '${conflict}' is already in.", ("full_type_name",full_type_name)("conflict",itr->second));
      }
      return name;
    } 

    auto bases = recordDecl->bases();
    auto total_bases = distance(bases.begin(), bases.end());
    if( total_bases > 1 ) {
      ABI_ASSERT(false, "Multiple inheritance not supported - ${type}", ("type",full_type_name));
    }

    string base_name;
    if( total_bases == 1 ) {
       
       auto qt = bases.begin()->getType();
       base_name = extractTypeNameOnly(qt.getAsString());

       const auto* base_record_type = qt->getAs<clang::RecordType>();
       const auto* base_record_type_decl = clang::cast_or_null<clang::CXXRecordDecl>(base_record_type->getDecl()->getDefinition());

        if(qt.getAsString().find_last_of("::") != string::npos) {
          addStruct(base_record_type_decl);
        }
        else {
          addStruct(base_record_type_decl, resolveType(base_name));
        }
    }

    eos::types::Struct abi_struct;
    for (const auto& field : recordDecl->fields()) {
      
      eos::types::Field struct_field;

      clang::QualType qt = field->getType().getUnqualifiedType();

      auto field_type = extractTypeNameOnly(qt.getAsString());

      ABI_ASSERT(field_type.size() <= sizeof(decltype(struct_field.type)),
        "Type name > ${maxsize}, ${type}::${name}", ("type",full_type_name)("name",field_type)("maxsize",sizeof(decltype(struct_field.type))));

      ABI_ASSERT(field->getNameAsString().size() <= sizeof(decltype(struct_field.name)) , 
        "Field name > ${maxsize}, ${type}::${name}", ("type",full_type_name)("name",field->getNameAsString())("maxsize", sizeof(decltype(struct_field.name))));

      const auto* field_record_type = qt->getAs<clang::RecordType>();
      
      if(field_record_type) {
        const auto* field_record = clang::cast_or_null<clang::CXXRecordDecl>(field_record_type->getDecl()->getDefinition());

        //Only import user defined structs
        auto is_user_type = !isBuiltInType(field_type);
        if(is_user_type) {
          if(qt.getAsString().find_last_of("::") != string::npos) {
            addStruct(field_record);
          }
          else {
            addStruct(field_record, resolveType(field_type));
          }
        }

        //TODO: fix this
        // if( isEnabledOpt(OPT_SINGLE_FIELD_STRUCT) && isOneFieldNoBase(field_type) ) {
        //   field_type = output->structs.back().fields[0].type;
        //   output->structs.pop_back();
        // }

      }
      
      struct_field.name = field->getNameAsString();
      struct_field.type = translateType(field_type);

      type_size[string(struct_field.type)] = context->getTypeSize(qt);

      eos::types::Struct dummy;
      ABI_ASSERT(isBuiltInType(struct_field.type) || findStruct(struct_field.type, dummy), "Unknown type ${type} ${name} ${ttt} ${sss}", ("type",struct_field.type)("name",struct_field.name)("ttt", output->types)("sss",output->structs));

      abi_struct.fields.push_back(struct_field);
    }

    abi_struct.name = name;
    abi_struct.base = extractTypeNameOnly(base_name);
    output->structs.push_back(abi_struct);

    if(verbose) {    
      cerr << "Adding type " << name << " (" << full_type_name << ")\n";
    }

    full_types[name] = full_type_name;
    return name;
  }

};

struct AbiGeneratorASTConsumer : public ASTConsumer {

  AbiGenerator& abi_gen;

  AbiGeneratorASTConsumer(CompilerInstance& compiler_instance, AbiGenerator& abi_gen) : abi_gen(abi_gen) {
    abi_gen.setCompilerInstance(compiler_instance);
  }

  void Initialize(clang::ASTContext& context) override {
    abi_gen.Initialize(context);
  }

  void HandleTranslationUnit(ASTContext& context) override {
    abi_gen.handleTranslationUnit(context);
  }

  bool HandleTopLevelDecl( DeclGroupRef  decl_group ) override {
    return abi_gen.handleTopLevelDecl(decl_group);
  }  

  void HandleTagDeclDefinition(TagDecl* tag_decl) override {
    abi_gen.handleTagDeclDefinition(tag_decl);
  }

};

class GenerateAbiAction : public ASTFrontendAction {
  set<string>  parsed_templates;
  AbiGenerator abi_gen;
public:
  GenerateAbiAction(bool verbose, bool opt_sfs, string abi_context, eos::types::Abi& output) {
    abi_gen.setOutput(output);
    abi_gen.setVerbose(verbose);
    abi_gen.setAbiContext(abi_context);

    if(opt_sfs)
      abi_gen.setOptimizaton(AbiGenerator::OPT_SINGLE_FIELD_STRUCT);
  }

protected:
  unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& compiler_instance,
                                                 llvm::StringRef) override {
    return llvm::make_unique<AbiGeneratorASTConsumer>(compiler_instance, abi_gen);
  }

};

} } //ns eos::abi_generator
