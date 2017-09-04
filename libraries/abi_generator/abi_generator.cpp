#include <set>
#include <regex>

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

#include <fc/io/json.hpp>
#include <eos/types/types.hpp>
using namespace clang;

bool generateAbi = true;
std::string destinationFile;
std::string abiContext;

namespace {

class RecordConsumer : public ASTConsumer {
  CompilerInstance& instance;
  std::set<std::string> parsedTemplates;
  eos::types::Abi abi;

public:
  RecordConsumer(CompilerInstance& instance,
                         std::set<std::string> parsedTemplates)
      : instance(instance), parsedTemplates(parsedTemplates) {
        if( boost::filesystem::exists(destinationFile) )
          abi = fc::json::from_file<eos::types::Abi>(destinationFile);
      }
  
  void HandleTranslationUnit(ASTContext& ctx) override {
     //std::cout << "termino:" << generateAbi << std::endl;
     if(generateAbi) {
       fc::json::save_to_file<eos::types::Abi>(abi, destinationFile);
     } else if( boost::filesystem::exists(destinationFile) ) {
       boost::filesystem::remove(destinationFile);
     }
  }

  std::string RemoveNamespace(const std::string& fullTypeName) {
    std::string typeName = fullTypeName;
    auto pos = typeName.find_last_of("::");
    if(pos != std::string::npos)
      typeName = typeName.substr(pos+1);
    return typeName;
  }

  std::string TranslateType(const std::string& typeName) {
    std::string builtInType = typeName;

    if      (typeName == "unsigned __int128" || typeName == "uint128_t") builtInType = "UInt128";
    else if (typeName == "__int128"          || typeName == "int128_t")  builtInType = "Int128";

    else if (typeName == "unsigned long long" || typeName == "uint64_t") builtInType = "UInt64";
    else if (typeName == "unsigned long"      || typeName == "uint32_t") builtInType = "UInt32";
    else if (typeName == "unsigned short"     || typeName == "uint16_t") builtInType = "UInt16";
    else if (typeName == "unsigned char"      || typeName == "uint8_t")  builtInType = "UInt8";
    
    else if (typeName == "long long"          || typeName == "int64_t")  builtInType = "Int64";
    else if (typeName == "long"               || typeName == "int32_t")  builtInType = "Int32";
    else if (typeName == "short"              || typeName == "int16_t")  builtInType = "Int16";
    else if (typeName == "char"               || typeName == "int8_t")   builtInType = "Int8";

    return builtInType;
  }

  std::string GetNameFromRecordDecl(const CXXRecordDecl* recordDecl) {
    clang::PrintingPolicy policy = instance.getLangOpts();
    auto name = recordDecl->getTypeForDecl()->getCanonicalTypeInternal().getAsString(policy);
    return RemoveNamespace(name);
  }

  void HandleTagDeclDefinition(TagDecl* tagDecl) override {
    
    if(!generateAbi) return;

    const CXXRecordDecl* recordDecl = dyn_cast<CXXRecordDecl>(tagDecl);
    if(!recordDecl) return;

    ASTContext& ctx = recordDecl->getASTContext();
    const RawComment* rawComment = ctx.getRawCommentForDeclNoCache(tagDecl);
    
    if(!rawComment) return;

    SourceManager& sm = ctx.getSourceManager();
    std::string rawText = rawComment->getRawText(sm);

    std::regex r(R"(@abi ([a-zA-Z0-9]+) (action|table)((?: [a-z0-9]+)*))");

    std::smatch smatch;
    while(regex_search(rawText, smatch, r))
    {
      if(smatch.size() == 4) {

        auto context = smatch[1].str();
        if(context != abiContext) return;

        auto type = smatch[2].str();
        
        std::vector<std::string> params;
        auto sparams = smatch[3].str();
        boost::trim(sparams);
        if(!sparams.empty())
          boost::split(params, sparams, boost::is_any_of(" "));

        auto name = GetNameFromRecordDecl(recordDecl);

        if(type == "action") {
          if(params.size()==0) {
            abi.actions.push_back({boost::algorithm::to_lower_copy(name), name});
          }
          else {
            for(const auto& action : params)
              abi.actions.push_back({action, name});
          }
        } 
        else if (type == "table") {
          if(params.size()==0) {
            abi.tables.push_back({boost::algorithm::to_lower_copy(name), name});
          } else {
            abi.tables.push_back({params[0], name});
          }
        }

        AddRecord(recordDecl, name);
      }
    
      rawText = smatch.suffix();
    }

  }

  bool AddRecord(const clang::CXXRecordDecl* recordDecl, const std::string& full_name) {

    //TODO: handle this better
    if(!recordDecl) {
      generateAbi = false;
      llvm::errs() << "Record declaration is NULL!!!\n";
      return false;
    }

    std::string name = RemoveNamespace(full_name);
    for( const auto& st : abi.structs ) 
      if(st.name == name) return false;

    //llvm::errs() << "Adding " << name << "\n";
    
    auto bases = recordDecl->bases();
    auto total_bases = std::distance(bases.begin(), bases.end());
    if( total_bases > 1 ) {
      llvm::errs() << "Unable to generate ABI, type \"" << name << "\" inherits from multiple types\n";
      generateAbi = false;
      return false;
    }

    std::string baseName;
    if( total_bases == 1 ) {
      auto const* baseRecordType = bases.begin()->getType()->getAs<clang::RecordType>();
      auto const* baseRecord = clang::cast_or_null<clang::CXXRecordDecl>(baseRecordType->getDecl()->getDefinition());
      baseName = GetNameFromRecordDecl(baseRecord);
      AddRecord(baseRecord, baseName);
    }

    eos::types::Struct abiStruct;
    for (const auto& field : recordDecl->fields()) {
      
      eos::types::Field structField;
      clang::QualType qt = field->getType().getUnqualifiedType();

      auto field_type = RemoveNamespace(qt.getAsString());

      auto const* fieldRecordType =qt->getAs<clang::RecordType>();
      
      if(fieldRecordType) {
        auto const* fieldRecord = clang::cast_or_null<clang::CXXRecordDecl>(fieldRecordType->getDecl()->getDefinition());
        
        if( AddRecord(fieldRecord, field_type) ) {
          field_type = abi.structs.back().fields[0].type;
          abi.structs.pop_back();
        }
      }

      //llvm::errs() << name << "::" << qt.getAsString() << " " << field->getNameAsString() << "\"\n";
      
      structField.name = field->getNameAsString();
      structField.type = TranslateType(field_type);

      abiStruct.fields.push_back(structField);
    }

    abiStruct.name = name;
    abiStruct.base = baseName;
    abi.structs.push_back(abiStruct);

    bool one_field_no_base = total_bases == 0 && abiStruct.fields.size() == 1;
    return one_field_no_base;
  }


};

class GenerateAbiAction : public PluginASTAction {
  std::set<std::string> parsedTemplates;
  const std::string destinationFileParam = "-destination-file=";
  const std::string abiContextParam = "-context=";

protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &ci,
                                                 llvm::StringRef) override {
    return llvm::make_unique<RecordConsumer>(ci, parsedTemplates);
  }

  bool ParseArgs(const CompilerInstance &ci,
                 const std::vector<std::string> &args) override {
    
    size_t totalArgs = args.size();
    for (size_t i=0; i<totalArgs; ++i) {
      if(args[i].find(destinationFileParam) == 0) 
        destinationFile = args[i].substr(destinationFileParam.size());
      else if(args[i].find(abiContextParam) == 0) 
        abiContext = args[i].substr(abiContextParam.size());

    }

    // std::cout << "destinationFile: [" << destinationFile << "]" << std::endl;
    // std::cout << "abiContext: [" << abiContext << "]" << std::endl;
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
    //std::cout << "Pragma!!" << std::endl;
    generateAbi = false;
  }
};

static PragmaHandlerRegistry::Add<NoAbiPragmaHandler> Y("no-abi","No ABI");
static FrontendPluginRegistry::Add<GenerateAbiAction> X("generate-abi", "Generate ABI");