#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include <set>
#include <regex>
#include <algorithm>

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
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <eos/types/AbiSerializer.hpp>
#include <fc/io/json.hpp>
#include <eos/types/types.hpp>

using namespace clang;
using namespace std;
using namespace clang::tooling;
namespace cl = llvm::cl;

namespace bfs = boost::filesystem;

struct AbiGenerator {

  const char*  NOABI_VARNAME = "NOABI";

  bool                      enabled;
  bool                      disabled_by_user;
  bool                      error_found;
  bool                      verbose;
  eos::types::Abi           abi;
  clang::ASTContext*        context;
  CompilerInstance*         compiler_instance;
  map<string, uint64_t>     type_size;
  map<string, string>       full_type_name;
  string                    destination_file;
  string                    abi_context;
  
  AbiGenerator() : enabled(true), disabled_by_user(false), error_found(false), verbose(false)  {
    remove(destination_file + ".abi.done");
    auto no_abi = std::getenv(NOABI_VARNAME);
    if(no_abi && boost::lexical_cast<bool>(no_abi) ) {
     onUserDisable();
     cerr << "ABI Generation disabled ("<< NOABI_VARNAME << "=" << no_abi << ")\n";
    }
  }

  ~AbiGenerator() {
    
    if(verbose) {
      cerr << "ABI Generation finished disabled_by_user:" << 
      disabled_by_user << ",error_found:" << error_found << "\n";
    }
    
    if(disabled_by_user) {
      touch(destination_file + ".abi.done");
    } else if(!error_found) {
      fc::json::save_to_file<eos::types::Abi>(abi, destination_file + ".abi");
      touch(destination_file + ".abi.done");
    } else {
      //should we remove the .abi file?
      remove(destination_file + ".abi");
    }
  }
  
  void touch(const string& fileName) {
    bfs::ofstream(fileName.c_str());  
  }

  void remove(const string& fileName) {
    if(bfs::exists(fileName.c_str()))
      bfs::remove(fileName.c_str());  
  }

  static AbiGenerator& get() {
    static AbiGenerator generator;
    return generator;
  }

  bool isEnabled() {
    return enabled;
  }

  void onUserDisable(){
    disableGeneration();
    disabled_by_user = true;
  }

  void setContext(clang::ASTContext& context) {
    this->context = &context;
  }

  void setCompilerInstance(CompilerInstance& compiler_instance) {
    this->compiler_instance = &compiler_instance;
  }
 
  void onError(const string& error_message) {
    disableGeneration();
    error_found = true;
    elog(error_message.c_str());
  }

  void disableGeneration() {
    enabled = false;
  }

  string removeNamespace(const string& full_type_name) {
    string type_name = full_type_name;
    auto pos = type_name.find_last_of("::");
    if(pos != string::npos)
      type_name = type_name.substr(pos+1);
    pos = type_name.find_last_of(" ");
    if(pos != string::npos)
      type_name = type_name.substr(pos+1);
    return type_name;
  }

  bool isBuiltInType(const string& type_name) {
    eos::types::AbiSerializer serializer;
    return serializer.isBuiltInType(type_name);
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

  string getNameFromRecordDecl(const CXXRecordDecl* record_decl) {
    clang::PrintingPolicy policy = compiler_instance->getLangOpts();
    return record_decl->getTypeForDecl()->getCanonicalTypeInternal().getAsString(policy);
  }
  
  void handleTranslationUnit(ASTContext& context) {
    //mmm ...
  }

  void handleTagDeclDefinition(TagDecl* tag_decl) {
    try {
      handleTagDeclDefinitionEx(tag_decl);
    } catch(fc::exception& e) {
      onError(e.to_string());
    }    
  }

  void handleTagDeclDefinitionEx(TagDecl* tag_decl) { try {
    
    if(!isEnabled()) return;

    const CXXRecordDecl* recordDecl = dyn_cast<CXXRecordDecl>(tag_decl);
    if(!recordDecl) return;

    if(verbose) {
      cerr << "handleTagDeclDefinitionEx:" << tag_decl->getKindName().str() << " " << getNameFromRecordDecl(recordDecl) << "\n";
    }


    ASTContext& ctx = recordDecl->getASTContext();
    const RawComment* raw_comment = ctx.getRawCommentForDeclNoCache(tag_decl);
    
    if(!raw_comment) return;

    SourceManager& source_manager = ctx.getSourceManager();
    string rawText = raw_comment->getRawText(source_manager);

    regex r(R"(@abi ([a-zA-Z0-9]+) (action|table)((?: [a-z0-9]+)*))");

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

        auto full_type_name = getNameFromRecordDecl(recordDecl);
        FC_ASSERT(tag_decl->isStruct() || tag_decl->isClass(), "Only struct and class are supported. ${type} ${full_type_name}",("type",type)("full_type_name",full_type_name));

        if(verbose) {
          cerr << "About to add " << full_type_name << "\n";
        }

        addStruct(recordDecl, full_type_name);
        auto type_name = removeNamespace(full_type_name);

        eos::types::Struct s;
        FC_ASSERT(findStruct(type_name, s), "Unable to find type ${type}", ("type",type_name));

        if(type == "action") {
          if(params.size()==0) {
            abi.actions.push_back( eos::types::Action{boost::algorithm::to_lower_copy(type_name), type_name} );
          } else {
            for(const auto& action : params)
              abi.actions.push_back({action, type_name});
          }
        } else if (type == "table") {
          eos::types::Table table;
          table.table = boost::algorithm::to_lower_copy(type_name);
          table.type = type_name;
          
          if(params.size() >= 1)
            table.indextype = params[0];
          else
            guessIndexType(type_name, table, s);

          guessKeyNames(type_name, table, s);

          abi.tables.push_back(table);
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
      FC_ASSERT(findStruct(s.base, base), "Unable to find base type ${type}",("type",s.base));
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

  void guessIndexType(const string& type_name, eos::types::Table& table, const eos::types::Struct s) {

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
      FC_ASSERT(false, "Unable to guess index type for ${type}.", ("type",type_name));
    }
  }

  void guessKeyNames(const string& type_name, eos::types::Table& table, const eos::types::Struct s) {

    vector<eos::types::Field> fields;
    getAllFields(s, fields);

    if( table.indextype == "i64i64i64" && isi64i64i64(fields) ) {
      table.keynames  = vector<eos::types::FieldName>{fields[0].name, fields[1].name, fields[2].name};
      table.keytype   = vector<eos::types::TypeName>{fields[0].type, fields[1].type, fields[2].type};
    } else if( table.indextype == "i128i128" && isi128i128(fields) ) {
      table.keynames  = vector<eos::types::FieldName>{fields[0].name, fields[1].name};
      table.keytype   = vector<eos::types::TypeName>{fields[0].type, fields[1].type};
    } else if( table.indextype == "i64" && isi64(fields) ) {
      table.keynames  = vector<eos::types::FieldName>{fields[0].name};
      table.keytype   = vector<eos::types::TypeName>{fields[0].type};
    } else if( table.indextype == "str" && isstr(fields) ) {
      table.keynames  = vector<eos::types::FieldName>{fields[0].name};
      table.keytype   = vector<eos::types::TypeName>{fields[0].type};
    } else {
      FC_ASSERT(false, "Unable to guess key names for ${type}.", ("type",type_name));
    }
  }

  bool findStruct(const string& name, eos::types::Struct& s) {
    for( const auto& st : abi.structs ) {
      if(st.name == name) {
        s = st;
        return true;
      }
    }
    return false;
  }

  bool isOneFieldNoBase(const string& type_name) {
    eos::types::Struct s;
    bool found = findStruct(type_name, s);
    return found && s.base.size() == 0 && s.fields.size() == 1;
  }

  void addStruct(const clang::CXXRecordDecl* recordDecl, const string& full_name) {
    
    //TODO: handle this better
    if(!recordDecl) {
      FC_ASSERT(false, "Null CXXRecordDecl for ${type}.", ("type",full_name));
    }

    string name = removeNamespace(full_name);

    // FC_ASSERT(name.size() <= sizeof(eos::types::TypeName),
    //   "Type name > ${maxsize}, ${name}",
    //   ("type",full_name)("name",name)("maxsize",sizeof(eos::types::TypeName)));

    // auto itr = full_type_name.find(name);
    // if(itr != full_type_name.end()) {
    //   FC_ASSERT(itr->second == full_name, "Unable to add type '${full_name}' because '${conflict}' is already in.", ("full_name",full_name)("conflict",itr->second));
    // }

    eos::types::Struct s;
    if( findStruct(name, s) ) return;
    
    auto bases = recordDecl->bases();
    auto total_bases = distance(bases.begin(), bases.end());
    if( total_bases > 1 ) {
      FC_ASSERT(false, "Multiple inheritance not supported - ${type}", ("type",full_name));
    }

    string base_name;
    if( total_bases == 1 ) {
      auto const* base_record_type = bases.begin()->getType()->getAs<clang::RecordType>();
      auto const* base_record_type_decl = clang::cast_or_null<clang::CXXRecordDecl>(base_record_type->getDecl()->getDefinition());
      base_name = getNameFromRecordDecl(base_record_type_decl);
      addStruct(base_record_type_decl, base_name);
    }

    eos::types::Struct abi_struct;
    for (const auto& field : recordDecl->fields()) {
      
      eos::types::Field struct_field;
      clang::QualType qt = field->getType().getUnqualifiedType();

      //cout << "full name type " << qt.getAsString() << " = " << context->getTypeSize(qt) << "\n";

      auto field_type = removeNamespace(qt.getAsString());
      auto const* field_record_type = qt->getAs<clang::RecordType>();
      
      if(field_record_type) {
        auto const* field_record = clang::cast_or_null<clang::CXXRecordDecl>(field_record_type->getDecl()->getDefinition());
        addStruct(field_record, field_type);
        if( isOneFieldNoBase(field_type) ) {
          //cerr << "isOneFieldNoBase == TRUE : [" << field_type << "]\n";
          field_type = abi.structs.back().fields[0].type;
          abi.structs.pop_back();
        }
      }
      
      auto struct_field_name = field->getNameAsString();
      auto struct_field_type = translateType(field_type);

      // FC_ASSERT(struct_field_name.size() <= sizeof(decltype(struct_field.name)) , 
      //   "Field name > ${maxsize}, ${type}::${name}", ("type",full_name)("name",struct_field_name)("maxsize", sizeof(decltype(struct_field.name))));
      
      // FC_ASSERT(struct_field_type.size() <= sizeof(decltype(struct_field.type)),
      //   "Type name > ${maxsize}, ${type}::${name}", ("type",full_name)("name",struct_field_type)("maxsize",sizeof(decltype(struct_field.type))));

      struct_field.name = struct_field_name;
      struct_field.type = struct_field_type;

      type_size[string(struct_field.type)] = context->getTypeSize(qt);

      eos::types::Struct dummy;
      FC_ASSERT(isBuiltInType(struct_field.type) || findStruct(struct_field.type, dummy), "Unknown type ${type_name}", ("type_name",struct_field.type));

      abi_struct.fields.push_back(struct_field);
    }

    abi_struct.name = name;
    abi_struct.base = removeNamespace(base_name);
    abi.structs.push_back(abi_struct);

    if(verbose) {    
      cerr << "Adding type " << name << " (" << full_name << ")\n";
    }

    full_type_name[name] = full_name;
  }

};

class RecordConsumer : public ASTConsumer {
  set<string>        parsed_templates;
  AbiGenerator&      generator;
public:
  
  void Initialize(clang::ASTContext& context) override {
    AbiGenerator::get().setContext(context);
  }

  RecordConsumer(CompilerInstance& compiler_instance, set<string> parsed_templates) : 
      parsed_templates(parsed_templates), generator(AbiGenerator::get()) {
    generator.setCompilerInstance(compiler_instance);
  }

  void HandleTranslationUnit(ASTContext& context) override {
    generator.handleTranslationUnit(context);
  }

  void HandleTagDeclDefinition(TagDecl* tag_decl) override { 
    generator.handleTagDeclDefinition(tag_decl);
  }

};

class GenerateAbiAction : public PluginASTAction {
  set<string>   parsed_templates;
public:
  GenerateAbiAction() {}

protected:
  unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &compiler_instance,
                                                 llvm::StringRef) override {
    return llvm::make_unique<RecordConsumer>(compiler_instance, parsed_templates);
  }

  bool ParseArgs(const CompilerInstance &CI, const vector<string>& args) override {
    return true;
  }
};

class NoAbiPragmaHandler : public PragmaHandler {
  AbiGenerator& generator;
public:
  NoAbiPragmaHandler() : PragmaHandler("no_abi"), generator(AbiGenerator::get()) { }
  void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer,
                    Token &PragmaTok) {

    generator.onUserDisable();
    cerr << "ABI Generation disabled (#pragma no_abi)\n";
  }
};

static cl::OptionCategory AbiGeneratorCategory("ABI generator options");

static cl::opt<std::string> ABIContext(
    "context",
    cl::desc("ABI context"),
    cl::cat(AbiGeneratorCategory));

static cl::opt<std::string> ABIDestination(
    "destination-file",
    cl::desc("destination json file"),
    cl::cat(AbiGeneratorCategory));

static cl::opt<bool> ABIVerbose(
    "verbose",
    cl::desc("show debug info"),
    cl::cat(AbiGeneratorCategory));

int main(int argc, const char **argv) {

    CommonOptionsParser op(argc, argv, AbiGeneratorCategory);
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());

    auto& generator = AbiGenerator::get();
    generator.abi_context = ABIContext;
    generator.destination_file = ABIDestination;
    generator.verbose = ABIVerbose;

    int result = Tool.run(newFrontendActionFactory<GenerateAbiAction>().get());

    return result;
}
