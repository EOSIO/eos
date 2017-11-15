#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>
#include <eos/abi_generator/abi_generator.hpp>
#include <eos/types/abi_serializer.hpp>

using namespace eosio;

std::unique_ptr<FrontendActionFactory> create_factory(bool verbose, bool opt_sfs, string abi_context, types::abi& output) {
  class abi_frontend_action_factory : public FrontendActionFactory {
    bool             verbose;
    bool             opt_sfs;
    string           abi_context;
    types::abi& output;
  public:
    abi_frontend_action_factory(bool verbose, bool opt_sfs, string abi_context, types::abi& output) : verbose(verbose), abi_context(abi_context), output(output) {}
    clang::FrontendAction *create() override { return new generate_abi_action(verbose, opt_sfs, abi_context, output); }
  };

  return std::unique_ptr<FrontendActionFactory>(
      new abi_frontend_action_factory(verbose, opt_sfs, abi_context, output));
}

static cl::OptionCategory abi_generator_category("ABI generator options");

static cl::opt<std::string> abi_context(
    "context",
    cl::desc("ABI context"),
    cl::cat(abi_generator_category));

static cl::opt<std::string> abi_destination(
    "destination-file",
    cl::desc("destination json file"),
    cl::cat(abi_generator_category));

static cl::opt<bool> abi_verbose(
    "verbose",
    cl::desc("show debug info"),
    cl::cat(abi_generator_category));

static cl::opt<bool> abi_opt_sfs(
    "optimize-sfs",
    cl::desc("Optimize single field struct"),
    cl::cat(abi_generator_category));

int main(int argc, const char **argv) { try {
   CommonOptionsParser op(argc, argv, abi_generator_category);
   ClangTool Tool(op.getCompilations(), op.getSourcePathList());

   types::abi abi;

   int result = Tool.run(create_factory(abi_verbose, abi_opt_sfs, abi_context, abi).get());
   if(!result) {
      types::abi_serializer(abi).validate();
      fc::json::save_to_file<types::abi>(abi, abi_destination, true);
   }
   return result;
} FC_CAPTURE_AND_LOG(()); return -1; }
