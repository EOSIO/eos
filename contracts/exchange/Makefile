exchange.wast: exchange.cpp Makefile ../eoslib/eos.hpp exchange.hpp
	 /usr/local/Cellar/llvm/4.0.0_1/bin/clang-4.0 -emit-llvm -O3 --std=c++14 --target=wasm32 -c exchange.cpp  -I.. -fno-threadsafe-statics -fno-rtti -fno-exceptions -o .exchange.bc -I /usr/local/Cellar/llvm/4.0.0_1/include/c++/v1/
	 /Users/dlarimer/Downloads/llvm/build/bin/llc -asm-verbose=false .exchange.bc
	 /Users/dlarimer/eos/libraries/binaryen/bin/s2wasm -s 1024 .exchange.s > exchange.wast
	 wc -l exchange.wast
	 echo '#pragma once' > exchange.wast.hpp
	 echo 'const char* exchange_wast = R"=====(' >> exchange.wast.hpp
	 cat exchange.wast >> exchange.wast.hpp
	 echo ')=====";' >> exchange.wast.hpp

#	 /usr/local/Cellar/llvm/4.0.0_1/bin/clang-4.0 -emit-llvm -O3 --std=c++14 --target=wasm32 -nostdinc -c currency.cpp  -I.. -fno-threadsafe-statics -fno-rtti -fno-exceptions -o .currency.bc
#	 /usr/local/Cellar/llvm/4.0.0_1/bin/clang-4.0 -emit-llvm  --std=c++14 --target=wasm32 -nostdinc -c currency.cpp  -I.. -fno-threadsafe-statics -fno-rtti -fno-exceptions -o .currency.bc

test.wast: test.cpp Makefile eos.hpp
	 /usr/local/Cellar/llvm/4.0.0_1/bin/clang-4.0 -emit-llvm -O3 --std=c++14 --target=wasm32 -nostdinc -c test.cpp  -I.. -fno-threadsafe-statics -fno-rtti -fno-exceptions
	 /Users/dlarimer/Downloads/llvm/build/bin/llc -asm-verbose=false test.bc
	 /Users/dlarimer/eos/libraries/binaryen/bin/s2wasm test.s -s 1024  > test.wast
	 cat test.wast

