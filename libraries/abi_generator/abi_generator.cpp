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
#include "llvm/Support/raw_ostream.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <fc/io/json.hpp>
#include <eos/types/types.hpp>

using namespace clang;
using namespace std;
namespace bfs = boost::filesystem;

bool enable_abi_generation = true;
string destination_file;
string abi_contex;

string getDisabledAbiFile() {
  return destination_file + ".disabled";
}

void touch(const string& fileName) {
  bfs::ofstream(fileName.c_str());  
}

void setEnabled(bool enabled) {
  enable_abi_generation = enabled;
}

namespace {

class RecordConsumer : public ASTConsumer {

  CompilerInstance&         compiler_instance;
  set<string>               parsed_templates;
  eos::types::Abi           abi;
  clang::ASTContext*        context;
  map<string, uint64_t>     type_size;
public:
  
  void Initialize(clang::ASTContext& context) override {
    this->context = &context;
  }

  RecordConsumer(CompilerInstance& compiler_instance,
                         set<string> parsed_templates)
      : compiler_instance(compiler_instance), parsed_templates(parsed_templates) {
        
        setEnabled(!bfs::exists(getDisabledAbiFile()) && !bfs::exists(getErrorAbiFile()));
        
        if( isEnabled() && bfs::exists(getTempAbiFile()) )
          abi = fc::json::from_file<eos::types::Abi>(getTempAbiFile());
      }
  
  void HandleTranslationUnit(ASTContext& context) override {
     if( isEnabled() ) {
       fc::json::save_to_file<eos::types::Abi>(abi, getTempAbiFile());
     }
  }

  bool isEnabled() {
    return enable_abi_generation;
  }

  string getTempAbiFile() {
    return destination_file + ".tmp";
  }

  string getErrorAbiFile() {
    return destination_file + ".error";
  }
  
  void onError(const string& error_message) {
    setEnabled(false);
    touch(getErrorAbiFile());
    llvm::errs() << error_message << "\n";
  }

  string removeNamespace(const string& full_type_name) {
    string type_name = full_type_name;
    auto pos = type_name.find_last_of("::");
    if(pos != string::npos)
      type_name = type_name.substr(pos+1);
    return type_name;
  }

  bool isBuiltInType(const string& type_name) {
    //HACK: Using AbiSerializer causes linking error (fPIC required for openssl-dev)
    static const vector<string> built_in_types = vector<string> {
      "PublicKey", "Asset", "Price", "String", "Time", "Signature",
      "Checksum", "FieldName", "FixedString32", "FixedString16", "TypeName",
      "Bytes", "UInt8", "UInt16", "UInt32", "UInt64", "UInt128", "UInt256",
      "Int8", "Int16", "Int32", "Int64", "Int128", "Int256", "uint128_t",
      "Name", "Field", "Struct", "Fields", "AccountName", "PermissionName",
      "FuncName", "MessageName", "TypeName", "AccountPermission", "Message",
      "AccountPermissionWeight", "Transaction", "SignedTransaction",
      "KeyPermissionWeight", "Authority", "BlockchainConfiguration",
      "TypeDef", "Action", "Table", "Abi"
    };

    return find(built_in_types.begin(), built_in_types.end(), type_name) != built_in_types.end();
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
    clang::PrintingPolicy policy = compiler_instance.getLangOpts();
    auto name = record_decl->getTypeForDecl()->getCanonicalTypeInternal().getAsString(policy);
    return removeNamespace(name);
  }

  void HandleTagDeclDefinition(TagDecl* tag_decl) override { 
    try {
      handleTagDeclDefinitionEx(tag_decl);
    } catch(fc::exception& e) {
      onError(e.to_detail_string());
    }
  }

  void handleTagDeclDefinitionEx(TagDecl* tag_decl) { try {
    
    if(!isEnabled()) return;

    const CXXRecordDecl* recordDecl = dyn_cast<CXXRecordDecl>(tag_decl);
    if(!recordDecl) return;

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
        if(context != abi_contex) return;

        auto type = smatch[2].str();
        
        vector<string> params;
        auto string_params = smatch[3].str();
        boost::trim(string_params);
        if(!string_params.empty())
          boost::split(params, string_params, boost::is_any_of(" "));

        auto type_name = getNameFromRecordDecl(recordDecl);
        FC_ASSERT(tag_decl->isStruct() || tag_decl->isClass(), "Only struct and class are supported. ${type} ${type_name}",("type",type)("type_name",type_name));

        addStruct(recordDecl, type_name);

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
          table.type = type_name;
          
          if(params.size() >= 1)
            table.table = params[0];
          else
            table.table = boost::algorithm::to_lower_copy(type_name);

          if(params.size() >= 2)
            table.indextype = params[1];
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

  bool isCString(const string& type) {
    return type == "CString";
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
    return fields.size() == 2 && isCString(fields[0].type) && isCString(fields[1].type); 
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

    string name = removeNamespace(full_name);

    //TODO: handle this better
    if(!recordDecl) {
      FC_ASSERT(false, "Null CXXRecordDecl for ${type}.", ("type",name));
    }

    eos::types::Struct s;
    if( findStruct(name, s) ) return;
    
    auto bases = recordDecl->bases();
    auto total_bases = distance(bases.begin(), bases.end());
    if( total_bases > 1 ) {
      FC_ASSERT(false, "Multiple inheritance not supported - ${type}", ("type",name));
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
          field_type = abi.structs.back().fields[0].type;
          abi.structs.pop_back();
        }
      }
      
      struct_field.name = field->getNameAsString();
      struct_field.type = translateType(field_type);
      type_size[string(struct_field.type)] = context->getTypeSize(qt);

      eos::types::Struct dummy;
      FC_ASSERT(isBuiltInType(struct_field.type) || findStruct(struct_field.type, dummy), "Unknown type ${type_name}", ("type_name",struct_field.type));

      abi_struct.fields.push_back(struct_field);
    }

    abi_struct.name = name;
    abi_struct.base = base_name;
    abi.structs.push_back(abi_struct);
  }
};

class GenerateAbiAction : public PluginASTAction {
  set<string> parsed_templates;
  const string destination_file_param = "-destination-file=";
  const string abi_contex_param = "-context=";

protected:
  unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &compiler_instance,
                                                 llvm::StringRef) override {
    return llvm::make_unique<RecordConsumer>(compiler_instance, parsed_templates);
  }

  bool ParseArgs(const CompilerInstance &compiler_instance,
                 const vector<string> &args) override {
    
    size_t total_args = args.size();
    for (size_t i=0; i<total_args; ++i) {
      if(args[i].find(destination_file_param) == 0) 
        destination_file = args[i].substr(destination_file_param.size());
      else if(args[i].find(abi_contex_param) == 0) 
        abi_contex = args[i].substr(abi_contex_param.size());

    }

    return true;
  }

  void PrintHelp(llvm::raw_ostream& ros) {
    ros << "Help\n";
  }

};

}


class NoAbiPragmaHandler : public PragmaHandler {
public:
  NoAbiPragmaHandler() : PragmaHandler("no_abi") { }
  void HandlePragma(Preprocessor &PP, PragmaIntroducerKind Introducer,
                    Token &PragmaTok) {

    OnDisable();
  }

  void OnDisable() {
    setEnabled(false);
    touch(getDisabledAbiFile());
  }
};

static PragmaHandlerRegistry::Add<NoAbiPragmaHandler> Y("no-abi","No ABI");
static FrontendPluginRegistry::Add<GenerateAbiAction> X("generate-abi", "Generate ABI");