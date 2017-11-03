#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <eos/abi_generator/abi_generator.hpp>
#include <eos/types/AbiSerializer.hpp>
using namespace eos::abi_generator;

std::unique_ptr<FrontendActionFactory> createFactory(bool verbose, bool opt_sfs, string abi_context, eos::types::Abi& output) {
  class MyFrontendActionFactory : public FrontendActionFactory {
    bool             verbose;
    bool             opt_sfs;
    string           abi_context;
    eos::types::Abi& output;
  public:
    MyFrontendActionFactory(bool verbose, bool opt_sfs, string abi_context, eos::types::Abi& output) : verbose(verbose), abi_context(abi_context), output(output) {}
    clang::FrontendAction *create() override { return new GenerateAbiAction(verbose, opt_sfs, abi_context, output); }
  };

  return std::unique_ptr<FrontendActionFactory>(
      new MyFrontendActionFactory(verbose, opt_sfs, abi_context, output));
}

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

static cl::opt<bool> ABIOptSFS(
    "optimize-sfs",
    cl::desc("Optimize single field struct"),
    cl::cat(AbiGeneratorCategory));

int main(int argc, const char **argv) { try {
   CommonOptionsParser op(argc, argv, AbiGeneratorCategory);
   ClangTool Tool(op.getCompilations(), op.getSourcePathList());

   eos::types::Abi abi;

   int result = Tool.run(createFactory(ABIVerbose, ABIOptSFS, ABIContext, abi).get());
   if(!result) {
      eos::types::AbiSerializer(abi).validate();
      fc::json::save_to_file<eos::types::Abi>(abi, ABIDestination, true);
   }
   return result;
} FC_CAPTURE_AND_LOG(()); return -1; }
