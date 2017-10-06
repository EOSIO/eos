#include <eos/abi_generator/abi_generator.hpp>

using namespace eos::abi_generator;

std::unique_ptr<FrontendActionFactory> createFactory(bool verbose, string abi_context) {
  class MyFrontendActionFactory : public FrontendActionFactory {
    bool    verbose;
    string  abi_context;
  public:
    MyFrontendActionFactory(bool verbose, string abi_context) : verbose(verbose), abi_context(abi_context) {}
    clang::FrontendAction *create() override { return new GenerateAbiAction(verbose, abi_context); }
  };

  return std::unique_ptr<FrontendActionFactory>(
      new MyFrontendActionFactory(verbose, abi_context));
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

int main(int argc, const char **argv) {


  // runToolOnCode returns whether the action was correctly run over the
  // given code.
  //auto x = runToolOnCode(new GenerateAbiAction(true,"pete"), "class X {};");

    CommonOptionsParser op(argc, argv, AbiGeneratorCategory);
    ClangTool Tool(op.getCompilations(), op.getSourcePathList());

    int result = Tool.run(createFactory(ABIVerbose, ABIContext).get());

    auto& abigen = AbiGenerator::get();

    auto s = fc::json::to_string<eos::types::Abi>(abigen.abi);
    cout << s << endl;

    return result;
}
