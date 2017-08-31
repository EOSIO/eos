#include <set>

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"
using namespace clang;

typedef std::set<clang::DeclaratorDecl const *> Variables;
//typedef std::set<clang::CXXMethodDecl const *> Methods;

namespace {
//https://github.com/rizsotto/Constantine

class RecordConsumer : public ASTConsumer {
  CompilerInstance &Instance;
  std::set<std::string> ParsedTemplates;

public:
  RecordConsumer(CompilerInstance &Instance,
                         std::set<std::string> ParsedTemplates)
      : Instance(Instance), ParsedTemplates(ParsedTemplates) {}

  void HandleTagDeclDefinition(TagDecl *D) override {
    if (const CXXRecordDecl *RD = dyn_cast<CXXRecordDecl>(D)) {
      auto v = GetVariablesFromRecord(RD);
    }
  }
  
  std::set<clang::CXXRecordDecl const *> AllBase(clang::CXXRecordDecl const * Record) {
    std::set<clang::CXXRecordDecl const *> Result;

    llvm::SmallVector<clang::CXXRecordDecl const *, 8> Queue;
    Queue.push_back(Record);

    while (! Queue.empty()) {
        auto const Current = Queue.pop_back_val();
        for (const auto & BaseIt : Current->bases()) {
            if (auto const * Record = BaseIt.getType()->getAs<clang::RecordType>()) {
                if (auto const * Base = clang::cast_or_null<clang::CXXRecordDecl>(Record->getDecl()->getDefinition())) {
                  //llvm::errs() << "VAR: \"" << Base->getNameAsString() << "\"\n";
                  Queue.push_back(Base);
                }
            }
        }
        //llvm::errs() << "VAR mio: \"" << Current->getNameAsString() << "\"\n";
        Result.insert(Current);
    }
    return Result;
  }

  // method to copy variables out from class declaration
  Variables GetVariablesFromRecord(clang::CXXRecordDecl const * const Record) {
    Variables Result;
    for (auto const & RecordIt : AllBase(Record)) {
      llvm::errs() << "Record: \"" << RecordIt->getNameAsString() << "\"\n";
      for (const auto & FieldIt : RecordIt->fields()) {
          const clang::QualType qt = FieldIt->getType();
          llvm::errs() << "  \"" << qt.getAsString() << " " << FieldIt->getNameAsString() << "\"\n";  
          Result.insert(FieldIt);
        }
    }
    return Result;
  }

};

class GenerateAbiAction : public PluginASTAction {
  std::set<std::string> ParsedTemplates;
protected:
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 llvm::StringRef) override {
    return llvm::make_unique<RecordConsumer>(CI, ParsedTemplates);
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {
    return true;
  }
  void PrintHelp(llvm::raw_ostream& ros) {
    ros << "Help\n";
  }

};

}

static FrontendPluginRegistry::Add<GenerateAbiAction> X("generate-abi", "Generate ABI");