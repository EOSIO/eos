#pragma once

#include <llvm/IR/IRBuilder.h>

namespace LLVMJIT {

llvm::Value* CreateInBoundsGEPWAR(llvm::IRBuilder<>& irBuilder, llvm::Value* Ptr, llvm::Value* v1, llvm::Value* v2 = nullptr);

}