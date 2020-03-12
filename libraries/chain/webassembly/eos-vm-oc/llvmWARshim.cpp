#include <llvm/IR/IRBuilder.h>
namespace eosio { namespace chain { namespace eosvmoc {
namespace LLVMJIT {

llvm::Value* CreateInBoundsGEPWAR(llvm::IRBuilder<>& irBuilder, llvm::Value* Ptr, llvm::Value* v1, llvm::Value* v2, llvm::Value* v3) {
   if(!v2 && !v3)
      return irBuilder.CreateInBoundsGEP(Ptr, v1);
   else if(!v3)
      return irBuilder.CreateInBoundsGEP(Ptr, {v1, v2});
   else
      return irBuilder.CreateInBoundsGEP(Ptr, {v1, v2, v3});
}

}

}}}