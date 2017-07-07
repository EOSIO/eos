currency.wast: currency.cpp Makefile eos.hpp currency.hpp
	 /usr/local/Cellar/llvm/4.0.0_1/bin/clang-4.0 -emit-llvm -O3 --std=c++14 --target=wasm32 -nostdinc -c currency.cpp  -I.. -fno-threadsafe-statics -fno-rtti -fno-exceptions -o .currency.bc
	 /Users/dlarimer/Downloads/llvm/build/bin/llc -asm-verbose=false .currency.bc
	 /Users/dlarimer/eos/libraries/binaryen/bin/s2wasm -s 1024 .currency.s > currency.wast
	 wc -l currency.wast
	 echo '#pragma once' > currency.wast.hpp
	 echo 'const char* currency_wast = R"=====(' >> currency.wast.hpp
	 cat currency.wast >> currency.wast.hpp
	 echo ')=====";' >> currency.wast.hpp


test.wast: test.cpp Makefile eos.hpp
	 /usr/local/Cellar/llvm/4.0.0_1/bin/clang-4.0 -emit-llvm -O3 --std=c++14 --target=wasm32 -nostdinc -c test.cpp  -I.. -fno-threadsafe-statics -fno-rtti -fno-exceptions
	 /Users/dlarimer/Downloads/llvm/build/bin/llc -asm-verbose=false test.bc
	 /Users/dlarimer/eos/libraries/binaryen/bin/s2wasm test.s -s 1024  > test.wast
	 cat test.wast

