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

  bool                   verbose;
  int                    optimizations;
  types::Abi*            output;
  clang::ASTContext*     context;
  CompilerInstance*      compiler_instance;
  //map<string, uint64_t>  type_size;
  map<string, string>    full_types;
  string                 abi_context;

  AbiGenerator() : optimizations(0), output(nullptr), context(nullptr), compiler_instance(nullptr) {
  
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

  void setOutput(types::Abi& output) {
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
 
  string removeNamespace(const string& full_name) {
    string type_name = full_name;
    auto pos = type_name.find_last_of("::");
    if(pos != string::npos)
      type_name = type_name.substr(pos+1);
    return type_name;
  }

  bool isBuiltInType(const string& type_name) {
    types::AbiSerializer serializer;
    auto rtype = resolveType(type_name);
    //ilog("isBuiltInType: type_name:${type_name}, rtype:${rtype}",("type_name",type_name)("rtype",rtype));
    return serializer.isBuiltInType(translateType(rtype));
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
    else if (type_name == "string" ) built_in_type = "String";

    return built_in_type;
  }

  void handleTranslationUnit(ASTContext& context) {
    //ilog("AbiGenerator::handleTranslationUnit");
  }
  
  void handleTagDeclDefinition(TagDecl* tag_decl) { 
    auto decl_location = tag_decl->getLocation().printToString(context->getSourceManager());
    try {    
    handleDecl(tag_decl);
  } FC_CAPTURE_AND_RETHROW((decl_location)) }

  void handleDecl(const Decl* decl) { try {

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

    string rawText = raw_comment->getRawText(source_manager);
    regex r(R"(@abi (action|table)((?: [a-z0-9]+)*))");

    smatch smatch;
    while(regex_search(rawText, smatch, r))
    {
      if(smatch.size() == 3) {

        auto type = smatch[1].str();
        
        vector<string> params;
        auto string_params = smatch[2].str();
        boost::trim(string_params);
        if(!string_params.empty())
          boost::split(params, string_params, boost::is_any_of(" "));

        if(type == "action") {

          const auto* actionDecl = dyn_cast<CXXRecordDecl>(decl);
          ABI_ASSERT(actionDecl != nullptr);

          auto qt = actionDecl->getTypeForDecl()->getCanonicalTypeInternal();
          auto type_name = removeNamespace(addStruct(qt));

          if(params.size()==0) {
            params.push_back( boost::algorithm::to_lower_copy(type_name) );
          }

          for(const auto& action : params) {
            const auto* ac = findAction(action);  
            if( ac ) {
              ABI_ASSERT(ac->type == type_name, "Same action name with different type ${action}",("action",action));
              continue;
            }
            output->actions.push_back({action, type_name});
          }

        } else if (type == "table") {

          const auto* tableDecl = dyn_cast<CXXRecordDecl>(decl);
          ABI_ASSERT(tableDecl != nullptr);

          auto qt = tableDecl->getTypeForDecl()->getCanonicalTypeInternal();
          auto type_name = removeNamespace(addStruct(qt));

          const auto* s = findStruct(type_name);
          ABI_ASSERT(s, "Unable to find type ${type}", ("type",type_name));

          types::Table table;
          table.table = boost::algorithm::to_lower_copy(type_name);
          table.type = type_name;
          
          if(params.size() >= 1)
            table.indextype = params[0];
          else { try {
            guessIndexType(table, *s);
          } FC_CAPTURE_AND_RETHROW( (type_name) ) }

          if(params.size() >= 2) {
            table.table = params[1];
          }
          
          try {
            guessKeyNames(table, *s);
          } FC_CAPTURE_AND_RETHROW( (type_name) )

          //TODO: assert that we are adding the same table
          const auto* ta = findTable(table.table);
          if(!ta) {
            output->tables.push_back(table);  
          }
        }
      }
    
      rawText = smatch.suffix();
    } 

  } FC_CAPTURE_AND_RETHROW() }

  bool is64bit(const string& type) {
    types::AbiSerializer abis(*output);
    return abis.isInteger(type) && abis.getIntegerSize(type) == 64;
    //return type_size[type] == 64;
  }

  bool is128bit(const string& type) {
    types::AbiSerializer abis(*output);
    return abis.isInteger(type) && abis.getIntegerSize(type) == 128;
    //return type_size[type] == 128;
  }

  bool isString(const string& type) {
    return type == "String" || type == "string";
  }

  void getAllFields(const types::Struct& s, vector<types::Field>& fields) {
    for(const auto& field : s.fields) {
      fields.push_back(field);
    }
    if(s.base.size()) {
      const auto* base = findStruct(s.base);
      ABI_ASSERT(base, "Unable to find base type ${type}",("type",s.base));
      getAllFields(*base, fields);
    }
  }

  bool isi64i64i64(const vector<types::Field>& fields) {
    return fields.size() >= 3 && is64bit(fields[0].type) && is64bit(fields[1].type) && is64bit(fields[2].type);
  }

  bool isi64(const vector<types::Field>& fields) {
    return fields.size() >= 1 && is64bit(fields[0].type);
  }

  bool isi128i128(const vector<types::Field>& fields) {
    return fields.size() >= 2 && is128bit(fields[0].type) && is128bit(fields[1].type);
  }

  bool isstr(const vector<types::Field>& fields) {
    return fields.size() == 2 && isString(fields[0].type) && fields[0].name == "key" && fields[1].name == "value"; 
  }

  void guessIndexType(types::Table& table, const types::Struct s) {

    vector<types::Field> fields;
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

  void guessKeyNames(types::Table& table, const types::Struct s) {

    vector<types::Field> fields;
    getAllFields(s, fields);

    if( table.indextype == "i64i64i64" && isi64i64i64(fields) ) {
      table.keynames  = vector<types::FieldName>{fields[0].name, fields[1].name, fields[2].name};
      table.keytypes  = vector<types::TypeName>{fields[0].type, fields[1].type, fields[2].type};
    } else if( table.indextype == "i128i128" && isi128i128(fields) ) {
      table.keynames  = vector<types::FieldName>{fields[0].name, fields[1].name};
      table.keytypes  = vector<types::TypeName>{fields[0].type, fields[1].type};
    } else if( table.indextype == "i64" && isi64(fields) ) {
      table.keynames  = vector<types::FieldName>{fields[0].name};
      table.keytypes  = vector<types::TypeName>{fields[0].type};
    } else if( table.indextype == "str" && isstr(fields) ) {
      table.keynames  = vector<types::FieldName>{fields[0].name};
      table.keytypes  = vector<types::TypeName>{fields[0].type};
    } else {
      ABI_ASSERT(false, "Unable to guess key names");
    }
  }

  const types::Table* findTable(const types::Name& name) {
    for( const auto& ta : output->tables ) {
      if(ta.table == name) {
        return &ta;
      }
    }
    return nullptr;
  }

  const types::TypeDef* findType(const types::TypeName& newTypeName) {
    for( const auto& td : output->types ) {
      if(td.newTypeName == newTypeName) {
        return &td;
      }
    }
    return nullptr;
  }

  const types::Action* findAction(const types::Name& name) {
    for( const auto& ac : output->actions ) {
      if(ac.action == name) {
        return &ac;
      }
    }
    return nullptr;
  }

  const types::Struct* findStruct(const types::TypeName& name) {
    auto rname = resolveType(name);
    for( const auto& st : output->structs ) {
      if(st.name == rname) {
        return &st;
      }
    }
    return nullptr;
  }

  types::TypeName resolveType(const types::TypeName& type){
    const auto* td = findType(type);
    if( td ) {
      return resolveType(td->type);
    }
    return type;
  }

  bool isOneFieldNoBase(const string& type_name) {
    const auto* s = findStruct(type_name);
    return s && s->base.size() == 0 && s->fields.size() == 1;
  }
  
  clang::DeclContext* getTypeDeclContext( const clang::QualType& type )
  {
    DeclContext* context = nullptr;
    if( CXXRecordDecl* decl = type.getTypePtr()->getAsCXXRecordDecl() ){
        context = Decl::castToDeclContext( decl );
    }
    else if( const CXXRecordDecl* decl = type.getTypePtr()->getPointeeCXXRecordDecl() ){
        context = Decl::castToDeclContext( decl );
    }
    else if( const TypedefType* typedefType = type.getTypePtr()->getAs<TypedefType>() ){
        context = Decl::castToDeclContext( typedefType->getDecl() );
    }
    
    if( context != nullptr ){
        if( type.getTypePtr()->getTypeClass() == Type::TypeClass::Typedef ){
            context = context->getParent();
        }
        else if( strcmp( context->getDeclKindName(), "ClassTemplateSpecialization" ) == 0 ){
            context = context->getParent();
        }
    }
    return context;
  }

  // std::string getFullScope( const clang::QualType& type ){
  //     string scope = "";
  //     if( DeclContext* context = getTypeDeclContext( type ) ){
  //         scope = getFullScope( context );
  //     }
  //     return scope;
  // }

  // std::string getFullScope( DeclContext* declarationContext ){
   
  //     string scope = "";
  //     DeclContext* context = declarationContext;
      
  //     deque<string> contextNames;
  //     while( context != nullptr && !context->isTranslationUnit() ) {
  //         if( context->isNamespace() ){
  //             NamespaceDecl* ns = static_cast<NamespaceDecl*>( context );
  //             string nsName = ns->getNameAsString();
  //             contextNames.push_front( nsName );
  //         }
  //         context = context->getParent();
  //     }
      
  //     for( int i = 0; i < contextNames.size(); i++ ){
  //       scope += ( i > 0 ? "::" : "" ) + contextNames[i];
  //     }
      
  //     return scope;
  // }

  string declToString(clang::Decl *d) {
      const auto& sm = context->getSourceManager();
      clang::SourceLocation b(d->getLocStart()), _e(d->getLocEnd());
      clang::SourceLocation e(clang::Lexer::getLocForEndOfToken(_e, 0, sm, compiler_instance->getLangOpts()));
      return string(sm.getCharacterData(b),
          sm.getCharacterData(e)-sm.getCharacterData(b));
  }

  void addTypeDef(const clang::TypedefType* typeDef) {

    vector<types::TypeDef> typesToAdd;
    while(typeDef != nullptr) {
      const auto* typeDefDecl = typeDef->getDecl();
      auto qt = typeDefDecl->getUnderlyingType().getUnqualifiedType();

      auto full_name = translateType(qt.getAsString(compiler_instance->getLangOpts()));

      //HACK: We need to think another way to stop importing the "typedef chain"
      if( full_name.find("<") != string::npos ) {
        break;
      }

      auto newTypeName = translateType(typeDefDecl->getName().str());
      if(isBuiltInType(newTypeName)) {
        break;
      }

      //TODO: ABI_ASSERT TypeName length for typedef
      types::TypeDef abiTypeDef;
      abiTypeDef.newTypeName = newTypeName;
      abiTypeDef.type = removeNamespace( full_name );

      const auto* td = findType(abiTypeDef.newTypeName);
      if( td ) {
        ABI_ASSERT(abiTypeDef.type == td->type);
        typeDef = qt->getAs<clang::TypedefType>();
        continue;
      }

      //ilog("addTypeDef => ${a} ${b} [${c}]", ("a",abiTypeDef.newTypeName)("b",abiTypeDef.type)("c",getFullScope(qt)));
      ABI_ASSERT(abiTypeDef.newTypeName != abiTypeDef.type, 
          "Unable to export typedef `${td}` at ${at}",
          ("td",declToString(typeDef->getDecl()))
          ("at",typeDef->getDecl()->getLocation().printToString(context->getSourceManager()))
      );

      typesToAdd.push_back(abiTypeDef);
      typeDef = qt->getAs<clang::TypedefType>();
    }

    std::reverse(typesToAdd.begin(), typesToAdd.end());
    output->types.insert(output->types.end(), typesToAdd.begin(), typesToAdd.end());
  }

  string getNameToAdd(const clang::QualType& qualType) {

    auto typeName = qualType.getAsString(compiler_instance->getLangOpts());
    const auto* typeDef = qualType->getAs<clang::TypedefType>();
    if( !isBuiltInType(typeName) && typeDef != nullptr) {
      addTypeDef(typeDef);
    } 

    return typeName;
  }

  string addStruct(const clang::QualType& qualType) {

    ABI_ASSERT(output != nullptr);

    const auto* recordType = qualType->getAs<clang::RecordType>();
    ABI_ASSERT(recordType != nullptr);
    
    const auto* recordDecl = clang::cast_or_null<clang::CXXRecordDecl>(recordType->getDecl()->getDefinition());
    ABI_ASSERT(recordDecl != nullptr);

    auto full_name = getNameToAdd(qualType);

    auto name = removeNamespace(full_name);

    //Only export user defined types
    if( isBuiltInType(name) ) {
      return name;
    }

    const auto* type = qualType.getTypePtr();
    
    ABI_ASSERT(type->isStructureType() || type->isClassType(), "Only struct and class are supported. ${full_name}",("full_name",full_name));

    ABI_ASSERT(name.size() <= sizeof(types::TypeName),
      "Type name > ${maxsize}, ${name}",
      ("type",full_name)("name",name)("maxsize",sizeof(types::TypeName)));

    if( findStruct(name) ) {
      auto itr = full_types.find(resolveType(name));
      if(itr != full_types.end()) {
        ABI_ASSERT(itr->second == full_name, "Unable to add type '${full_name}' because '${conflict}' is already in.\n${t}", ("full_name",full_name)("conflict",itr->second)("t",output->types));
      }
      return name;
    }

    auto bases = recordDecl->bases();
    auto total_bases = distance(bases.begin(), bases.end());
    if( total_bases > 1 ) {
      ABI_ASSERT(false, "Multiple inheritance not supported - ${type}", ("type",full_name));
    }

    string base_name;
    if( total_bases == 1 ) {
      auto qt = bases.begin()->getType();
      base_name = addStruct(qt);
    }

    types::Struct abi_struct;
    for (const auto& field : recordDecl->fields()) {
      
      types::Field struct_field;

      clang::QualType qt = field->getType();

      auto field_type = translateType(removeNamespace(getNameToAdd(qt.getUnqualifiedType())));

      ABI_ASSERT(field_type.size() <= sizeof(decltype(struct_field.type)),
        "Type name > ${maxsize}, ${type}::${name}", ("type",full_name)("name",field_type)("maxsize",sizeof(decltype(struct_field.type))));

      ABI_ASSERT(field->getNameAsString().size() <= sizeof(decltype(struct_field.name)) , 
        "Field name > ${maxsize}, ${type}::${name}", ("type",full_name)("name",field->getNameAsString())("maxsize", sizeof(decltype(struct_field.name))));
      
      if( qt->getAs<clang::RecordType>() ) {
        addStruct(qt);
        //TODO: 
        // if( isEnabledOpt(OPT_SINGLE_FIELD_STRUCT) && isOneFieldNoBase(field_type) ) {
        //   field_type = output->structs.back().fields[0].type;
        //   output->structs.pop_back();
        // }
      }
      
      struct_field.name = field->getNameAsString();
      struct_field.type = field_type;

      ABI_ASSERT(isBuiltInType(struct_field.type) || findStruct(struct_field.type), "Unknown type ${type} ${name} ${ttt} ${sss}", ("type",struct_field.type)("name",struct_field.name)("ttt", output->types)("sss",output->structs));

      //type_size[string(struct_field.type)] = context->getTypeSize(qt);

      abi_struct.fields.push_back(struct_field);
    }

    abi_struct.name = resolveType(name);
    abi_struct.base = removeNamespace(base_name);
    
    output->structs.push_back(abi_struct);

    if(verbose) {    
     cerr << "Adding type " << resolveType(name) << " (" << full_name << ")\n";
    }

    full_types[name] = full_name;
    return full_name;
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

  void HandleTagDeclDefinition(TagDecl* tag_decl) override {
    abi_gen.handleTagDeclDefinition(tag_decl);
  }

};

class GenerateAbiAction : public ASTFrontendAction {
  set<string>  parsed_templates;
  AbiGenerator abi_gen;
public:
  GenerateAbiAction(bool verbose, bool opt_sfs, string abi_context, types::Abi& output) {
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
