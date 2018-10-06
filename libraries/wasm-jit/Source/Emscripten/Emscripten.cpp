#include "Inline/BasicTypes.h"
#include "Logging/Logging.h"
#include "IR/IR.h"
#include "IR/Module.h"
#include "Runtime/Runtime.h"
#include "Runtime/Intrinsics.h"
#include "Emscripten.h"
#include <time.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#ifndef _WIN32
#include <sys/uio.h>
#endif

namespace Emscripten
{
	using namespace IR;
	using namespace Runtime;

	static U32 coerce32bitAddress(Uptr address)
	{
		if(address >= UINT32_MAX) { causeException(Exception::Cause::accessViolation); }
		return (U32)address;
	}

	DEFINE_INTRINSIC_GLOBAL(env,STACKTOP,STACKTOP,i32,false,0);
	DEFINE_INTRINSIC_GLOBAL(env,STACK_MAX,STACK_MAX,i32,false,0);
	DEFINE_INTRINSIC_GLOBAL(env,tempDoublePtr,tempDoublePtr,i32,false,0);
	DEFINE_INTRINSIC_GLOBAL(env,ABORT,ABORT,i32,false,0);
	DEFINE_INTRINSIC_GLOBAL(env,cttz_i8,cttz_i8,i32,false,0);
	DEFINE_INTRINSIC_GLOBAL(env,___dso_handle,___dso_handle,i32,false,0);
	DEFINE_INTRINSIC_GLOBAL(env,_stderr,_stderr,i32,false,0);
	DEFINE_INTRINSIC_GLOBAL(env,_stdin,_stdin,i32,false,0);
	DEFINE_INTRINSIC_GLOBAL(env,_stdout,_stdout,i32,false,0);

	DEFINE_INTRINSIC_MEMORY(env,emscriptenMemory,memory,MemoryType(false,SizeConstraints({256,UINT64_MAX})));
	DEFINE_INTRINSIC_TABLE(env,table,table,TableType(TableElementType::anyfunc,false,SizeConstraints({1024*1024,UINT64_MAX})));

	DEFINE_INTRINSIC_GLOBAL(env,memoryBase,memoryBase,i32,false,1024);
	DEFINE_INTRINSIC_GLOBAL(env,tableBase,tableBase,i32,false,0);

	DEFINE_INTRINSIC_GLOBAL(env,DYNAMICTOP_PTR,DYNAMICTOP_PTR,i32,false,0)
	DEFINE_INTRINSIC_GLOBAL(env,em_environ,_environ,i32,false,0)
	DEFINE_INTRINSIC_GLOBAL(env,EMTSTACKTOP,EMTSTACKTOP,i32,false,0)
	DEFINE_INTRINSIC_GLOBAL(env,EMT_STACK_MAX,EMT_STACK_MAX,i32,false,0)
	DEFINE_INTRINSIC_GLOBAL(env,eb,eb,i32,false,0)

	Platform::Mutex* sbrkMutex = Platform::createMutex();
	bool hasSbrkBeenCalled = false;
	Uptr sbrkNumPages = 0;
	U32 sbrkMinBytes = 0;
	U32 sbrkNumBytes = 0;

	static U32 sbrk(I32 numBytes)
	{
		Platform::Lock sbrkLock(sbrkMutex);

		if(!hasSbrkBeenCalled)
		{
			// Do some first time initialization.
			sbrkNumPages = getMemoryNumPages(emscriptenMemory);
			sbrkMinBytes = sbrkNumBytes = coerce32bitAddress(sbrkNumPages << numBytesPerPageLog2);
			hasSbrkBeenCalled = true;
		}
		else
		{
			// Ensure that nothing else is calling growMemory/shrinkMemory.
			if(getMemoryNumPages(emscriptenMemory) != sbrkNumPages)
			{ causeException(Exception::Cause::unknown); }
		}
		
		const U32 previousNumBytes = sbrkNumBytes;
		
		// Round the absolute value of numBytes to an alignment boundary, and ensure it won't allocate too much or too little memory.
		numBytes = (numBytes + 7) & ~7;
		if(numBytes > 0 && previousNumBytes > UINT32_MAX - numBytes) { causeException(Exception::Cause::accessViolation); }
		else if(numBytes < 0 && previousNumBytes < sbrkMinBytes - numBytes) { causeException(Exception::Cause::accessViolation); }

		// Update the number of bytes allocated, and compute the number of pages needed for it.
		sbrkNumBytes += numBytes;
		const Uptr numDesiredPages = (sbrkNumBytes + numBytesPerPage - 1) >> numBytesPerPageLog2;

		// Grow or shrink the memory object to the desired number of pages.
		if(numDesiredPages > sbrkNumPages) { growMemory(emscriptenMemory,numDesiredPages - sbrkNumPages); }
		else if(numDesiredPages < sbrkNumPages) { shrinkMemory(emscriptenMemory,sbrkNumPages - numDesiredPages); }
		sbrkNumPages = numDesiredPages;

		return previousNumBytes;
	}

	DEFINE_INTRINSIC_FUNCTION1(env,_sbrk,_sbrk,i32,i32,numBytes)
	{
		return sbrk(numBytes);
	}

	DEFINE_INTRINSIC_FUNCTION1(env,_time,_time,i32,i32,address)
	{
		time_t t = time(nullptr);
		if(address)
		{
			memoryRef<I32>(emscriptenMemory,address) = (I32)t;
		}
		return (I32)t;
	}

	DEFINE_INTRINSIC_FUNCTION0(env,___errno_location,___errno_location,i32)
	{
		return 0;
	}

	DEFINE_INTRINSIC_FUNCTION1(env,_sysconf,_sysconf,i32,i32,a)
	{
		enum { sysConfPageSize = 30 };
		switch(a)
		{
		case sysConfPageSize: return IR::numBytesPerPage;
		default: causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic);
		}
	}

	DEFINE_INTRINSIC_FUNCTION2(env,_pthread_cond_wait,_pthread_cond_wait,i32,i32,a,i32,b) { return 0; }
	DEFINE_INTRINSIC_FUNCTION1(env,_pthread_cond_broadcast,_pthread_cond_broadcast,i32,i32,a) { return 0; }
	DEFINE_INTRINSIC_FUNCTION2(env,_pthread_key_create,_pthread_key_create,i32,i32,a,i32,b) { causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic); }
	DEFINE_INTRINSIC_FUNCTION1(env,_pthread_mutex_lock,_pthread_mutex_lock,i32,i32,a) { return 0; }
	DEFINE_INTRINSIC_FUNCTION1(env,_pthread_mutex_unlock,_pthread_mutex_unlock,i32,i32,a) { return 0; }
	DEFINE_INTRINSIC_FUNCTION2(env,_pthread_setspecific,_pthread_setspecific,i32,i32,a,i32,b) { causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic); }
	DEFINE_INTRINSIC_FUNCTION1(env,_pthread_getspecific,_pthread_getspecific,i32,i32,a) { causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic); }
	DEFINE_INTRINSIC_FUNCTION2(env,_pthread_once,_pthread_once,i32,i32,a,i32,b) { causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic); }
	DEFINE_INTRINSIC_FUNCTION2(env,_pthread_cleanup_push,_pthread_cleanup_push,none,i32,a,i32,b) { }
	DEFINE_INTRINSIC_FUNCTION1(env,_pthread_cleanup_pop,_pthread_cleanup_pop,none,i32,a) { }
	DEFINE_INTRINSIC_FUNCTION0(env,_pthread_self,_pthread_self,i32) { return 0; }

	DEFINE_INTRINSIC_FUNCTION0(env,___ctype_b_loc,___ctype_b_loc,i32)
	{
		unsigned short data[384] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,2,8195,8194,8194,8194,8194,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,24577,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,49156,55304,55304,55304,55304,55304,55304,55304,55304,55304,55304,49156,49156,49156,49156,49156,49156,49156,54536,54536,54536,54536,54536,54536,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,50440,49156,49156,49156,49156,49156,49156,54792,54792,54792,54792,54792,54792,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,50696,49156,49156,49156,49156,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		static U32 vmAddress = 0;
		if(vmAddress == 0)
		{
			vmAddress = coerce32bitAddress(sbrk(sizeof(data)));
			memcpy(memoryArrayPtr<U8>(emscriptenMemory,vmAddress,sizeof(data)),data,sizeof(data));
		}
		return vmAddress + sizeof(short)*128;
	}
	DEFINE_INTRINSIC_FUNCTION0(env,___ctype_toupper_loc,___ctype_toupper_loc,i32)
	{
		I32 data[384] = {128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};
		static U32 vmAddress = 0;
		if(vmAddress == 0)
		{
			vmAddress = coerce32bitAddress(sbrk(sizeof(data)));
			memcpy(memoryArrayPtr<U8>(emscriptenMemory,vmAddress,sizeof(data)),data,sizeof(data));
		}
		return vmAddress + sizeof(I32)*128;
	}
	DEFINE_INTRINSIC_FUNCTION0(env,___ctype_tolower_loc,___ctype_tolower_loc,i32)
	{
		I32 data[384] = {128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};
		static U32 vmAddress = 0;
		if(vmAddress == 0)
		{
			vmAddress = coerce32bitAddress(sbrk(sizeof(data)));
			memcpy(memoryArrayPtr<U8>(emscriptenMemory,vmAddress,sizeof(data)),data,sizeof(data));
		}
		return vmAddress + sizeof(I32)*128;
	}
	DEFINE_INTRINSIC_FUNCTION4(env,___assert_fail,___assert_fail,none,i32,condition,i32,filename,i32,line,i32,function)
	{
		causeException(Runtime::Exception::Cause::calledAbort);
	}

	DEFINE_INTRINSIC_FUNCTION3(env,___cxa_atexit,___cxa_atexit,i32,i32,a,i32,b,i32,c)
	{
		return 0;
	}
	DEFINE_INTRINSIC_FUNCTION1(env,___cxa_guard_acquire,___cxa_guard_acquire,i32,i32,address)
	{
		if(!memoryRef<U8>(emscriptenMemory,address))
		{
			memoryRef<U8>(emscriptenMemory,address) = 1;
			return 1;
		}
		else
		{
			return 0;
		}
	}
	DEFINE_INTRINSIC_FUNCTION1(env,___cxa_guard_release,___cxa_guard_release,none,i32,a)
	{}
	DEFINE_INTRINSIC_FUNCTION3(env,___cxa_throw,___cxa_throw,none,i32,a,i32,b,i32,c)
	{
		causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic);
	}
	DEFINE_INTRINSIC_FUNCTION1(env,___cxa_begin_catch,___cxa_begin_catch,i32,i32,a)
	{
		causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic);
	}
	DEFINE_INTRINSIC_FUNCTION1(env,___cxa_allocate_exception,___cxa_allocate_exception,i32,i32,size)
	{
		return coerce32bitAddress(sbrk(size));
	}
	DEFINE_INTRINSIC_FUNCTION0(env,__ZSt18uncaught_exceptionv,__ZSt18uncaught_exceptionv,i32)
	{
		causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic);
	}
	DEFINE_INTRINSIC_FUNCTION0(env,_abort,_abort,none)
	{
		causeException(Runtime::Exception::Cause::calledAbort);
	}
	DEFINE_INTRINSIC_FUNCTION1(env,_exit,_exit,none,i32,code)
	{
		causeException(Runtime::Exception::Cause::calledAbort);
	}
	DEFINE_INTRINSIC_FUNCTION1(env,abort,abort,none,i32,code)
	{
		Log::printf(Log::Category::error,"env.abort(%i)\n",code);
		causeException(Runtime::Exception::Cause::calledAbort);
	}

	static U32 currentLocale = 0;
	DEFINE_INTRINSIC_FUNCTION1(env,_uselocale,_uselocale,i32,i32,locale)
	{
		auto oldLocale = currentLocale;
		currentLocale = locale;
		return oldLocale;
	}
	DEFINE_INTRINSIC_FUNCTION3(env,_newlocale,_newlocale,i32,i32,mask,i32,locale,i32,base)
	{
		if(!base)
		{
			base = coerce32bitAddress(sbrk(4));
		}
		return base;
	}
	DEFINE_INTRINSIC_FUNCTION1(env,_freelocale,_freelocale,none,i32,a)
	{}

	DEFINE_INTRINSIC_FUNCTION5(env,_strftime_l,_strftime_l,i32,i32,a,i32,b,i32,c,i32,d,i32,e) { causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic); }
	DEFINE_INTRINSIC_FUNCTION1(env,_strerror,_strerror,i32,i32,a) { causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic); }

	DEFINE_INTRINSIC_FUNCTION2(env,_catopen,_catopen,i32,i32,a,i32,b) { return (U32)-1; }
	DEFINE_INTRINSIC_FUNCTION4(env,_catgets,_catgets,i32,i32,catd,i32,set_id,i32,msg_id,i32,s) { return s; }
	DEFINE_INTRINSIC_FUNCTION1(env,_catclose,_catclose,i32,i32,a) { return 0; }

	DEFINE_INTRINSIC_FUNCTION3(env,_emscripten_memcpy_big,_emscripten_memcpy_big,i32,i32,a,i32,b,i32,c)
	{
		memcpy(memoryArrayPtr<U8>(emscriptenMemory,a,c),memoryArrayPtr<U8>(emscriptenMemory,b,c),U32(c));
		return a;
	}

	enum class ioStreamVMHandle
	{
		StdErr = 1,
		StdIn = 2,
		StdOut = 3
	};
	FILE* vmFile(U32 vmHandle)
	{
		switch((ioStreamVMHandle)vmHandle)
		{
		case ioStreamVMHandle::StdErr: return stderr;
		case ioStreamVMHandle::StdIn: return stdin;
		case ioStreamVMHandle::StdOut: return stdout;
		default: return stdout;//std::cerr << "invalid file handle " << vmHandle << std::endl; throw;
		}
	}

	DEFINE_INTRINSIC_FUNCTION3(env,_vfprintf,_vfprintf,i32,i32,file,i32,formatPointer,i32,argList)
	{
		causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic);
	}
	DEFINE_INTRINSIC_FUNCTION1(env,_getc,_getc,i32,i32,file)
	{
		return getc(vmFile(file));
	}
	DEFINE_INTRINSIC_FUNCTION2(env,_ungetc,_ungetc,i32,i32,character,i32,file)
	{
		return ungetc(character,vmFile(file));
	}
	DEFINE_INTRINSIC_FUNCTION4(env,_fread,_fread,i32,i32,pointer,i32,size,i32,count,i32,file)
	{
		return (I32)fread(memoryArrayPtr<U8>(emscriptenMemory,pointer,U64(size) * U64(count)),U64(size),U64(count),vmFile(file));
	}
	DEFINE_INTRINSIC_FUNCTION4(env,_fwrite,_fwrite,i32,i32,pointer,i32,size,i32,count,i32,file)
	{
		return (I32)fwrite(memoryArrayPtr<U8>(emscriptenMemory,pointer,U64(size) * U64(count)),U64(size),U64(count),vmFile(file));
	}
	DEFINE_INTRINSIC_FUNCTION2(env,_fputc,_fputc,i32,i32,character,i32,file)
	{
		return fputc(character,vmFile(file));
	}
	DEFINE_INTRINSIC_FUNCTION1(env,_fflush,_fflush,i32,i32,file)
	{
		return fflush(vmFile(file));
	}

	DEFINE_INTRINSIC_FUNCTION1(env,___lock,___lock,none,i32,a)
	{
	}
	DEFINE_INTRINSIC_FUNCTION1(env,___unlock,___unlock,none,i32,a)
	{
	}
	DEFINE_INTRINSIC_FUNCTION1(env,___lockfile,___lockfile,i32,i32,a)
	{
		return 1;
	}
	DEFINE_INTRINSIC_FUNCTION1(env,___unlockfile,___unlockfile,none,i32,a)
	{
	}

	DEFINE_INTRINSIC_FUNCTION2(env,___syscall6,___syscall6,i32,i32,a,i32,b)
	{
		// close
		causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic);
	}

	DEFINE_INTRINSIC_FUNCTION2(env,___syscall54,___syscall54,i32,i32,a,i32,b)
	{
		// ioctl
		return 0;
	}

	DEFINE_INTRINSIC_FUNCTION2(env,___syscall140,___syscall140,i32,i32,a,i32,b)
	{
		// llseek
		causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic);
	}

	DEFINE_INTRINSIC_FUNCTION2(env,___syscall145,___syscall145,i32,i32,file,i32,argsPtr)
	{
		// readv
		causeException(Runtime::Exception::Cause::calledUnimplementedIntrinsic);
	}

	DEFINE_INTRINSIC_FUNCTION2(env,___syscall146,___syscall146,i32,i32,file,i32,argsPtr)
	{
		// writev
		U32* args = memoryArrayPtr<U32>(emscriptenMemory,argsPtr,3);
		U32 iov = args[1];
		U32 iovcnt = args[2];
#ifdef _WIN32
		U32 count = 0;
		for(U32 i = 0; i < iovcnt; i++)
		{
			U32 base = memoryRef<U32>(emscriptenMemory,iov + i * 8);
			U32 len = memoryRef<U32>(emscriptenMemory,iov + i * 8 + 4);
			U32 size = (U32)fwrite(memoryArrayPtr<U8>(emscriptenMemory,base,len), 1, len, vmFile(file));
			count += size;
			if (size < len)
				break;
		}
#else
		struct iovec *native_iovec = new(alloca(sizeof(iovec)*iovcnt)) struct iovec [iovcnt];
		for(U32 i = 0; i < iovcnt; i++)
		{
			U32 base = memoryRef<U32>(emscriptenMemory,iov + i * 8);
			U32 len = memoryRef<U32>(emscriptenMemory,iov + i * 8 + 4);

			native_iovec[i].iov_base = memoryArrayPtr<U8>(emscriptenMemory,base,len);
			native_iovec[i].iov_len = len;
		}
		Iptr count = writev(fileno(vmFile(file)), native_iovec, iovcnt);
#endif
		return count;
	}

	DEFINE_INTRINSIC_FUNCTION1(asm2wasm,f64_to_int,f64-to-int,i32,f64,f) { return (I32)f; }

	static F64 zero = 0.0;

	static F64 makeNaN() { return zero / zero; }
	static F64 makeInf() { return 1.0/zero; }

	DEFINE_INTRINSIC_GLOBAL(global,NaN,NaN,f64,false,makeNaN())
	DEFINE_INTRINSIC_GLOBAL(global,Infinity,Infinity,f64,false,makeInf())

	DEFINE_INTRINSIC_FUNCTION2(asm2wasm,i32_remu,i32u-rem,i32,i32,left,i32,right)
	{
		return (I32)((U32)left % (U32)right);
	}
	DEFINE_INTRINSIC_FUNCTION2(asm2wasm,i32_rems,i32s-rem,i32,i32,left,i32,right)
	{
		return left % right;
	}
	DEFINE_INTRINSIC_FUNCTION2(asm2wasm,i32_divu,i32u-div,i32,i32,left,i32,right)
	{
		return (I32)((U32)left / (U32)right);
	}
	DEFINE_INTRINSIC_FUNCTION2(asm2wasm,i32_divs,i32s-div,i32,i32,left,i32,right)
	{
		return left / right;
	}

	EMSCRIPTEN_API void initInstance(const Module& module,ModuleInstance* moduleInstance)
	{
		// Only initialize the module as an Emscripten module if it uses the emscripten memory by default.
		if(getDefaultMemory(moduleInstance) == emscriptenMemory)
		{
			// Allocate a 5MB stack.
			STACKTOP.reset(coerce32bitAddress(sbrk(5*1024*1024)));
			STACK_MAX.reset(coerce32bitAddress(sbrk(0)));

			// Allocate some 8 byte memory region for tempDoublePtr.
			tempDoublePtr.reset(coerce32bitAddress(sbrk(8)));

			// Setup IO stream handles.
			_stderr.reset(coerce32bitAddress(sbrk(sizeof(U32))));
			_stdin.reset(coerce32bitAddress(sbrk(sizeof(U32))));
			_stdout.reset(coerce32bitAddress(sbrk(sizeof(U32))));
			memoryRef<U32>(emscriptenMemory,_stderr) = (U32)ioStreamVMHandle::StdErr;
			memoryRef<U32>(emscriptenMemory,_stdin) = (U32)ioStreamVMHandle::StdIn;
			memoryRef<U32>(emscriptenMemory,_stdout) = (U32)ioStreamVMHandle::StdOut;

			// Call the establishStackSpace function to set the Emscripten module's internal stack pointers.
			FunctionInstance* establishStackSpace = asFunctionNullable(getInstanceExport(moduleInstance,"establishStackSpace"));
			if(establishStackSpace && getFunctionType(establishStackSpace) == FunctionType::get(ResultType::none,{ValueType::i32,ValueType::i32}))
			{
				std::vector<Runtime::Value> parameters = {Runtime::Value(STACKTOP),Runtime::Value(STACK_MAX)};
				Runtime::invokeFunction(establishStackSpace,parameters);
			}

			// Call the global initializer functions.
			for(Uptr exportIndex = 0;exportIndex < module.exports.size();++exportIndex)
			{
				const Export& functionExport = module.exports[exportIndex];
				if(functionExport.kind == ObjectKind::function && !strncmp(functionExport.name.c_str(),"__GLOBAL__",10))
				{
					FunctionInstance* functionInstance = asFunctionNullable(getInstanceExport(moduleInstance,functionExport.name));
					if(functionInstance) { Runtime::invokeFunction(functionInstance,{}); }
				}
			}
		}
	}

	EMSCRIPTEN_API void injectCommandArgs(const std::vector<const char*>& argStrings,std::vector<Runtime::Value>& outInvokeArgs)
	{
		U8* emscriptenMemoryBase = getMemoryBaseAddress(emscriptenMemory);

		U32* argvOffsets = (U32*)(emscriptenMemoryBase + sbrk((U32)(sizeof(U32) * (argStrings.size() + 1))));
		for(Uptr argIndex = 0;argIndex < argStrings.size();++argIndex)
		{
			auto stringSize = strlen(argStrings[argIndex])+1;
			auto stringMemory = emscriptenMemoryBase + sbrk((U32)stringSize);
			memcpy(stringMemory,argStrings[argIndex],stringSize);
			argvOffsets[argIndex] = (U32)(stringMemory - emscriptenMemoryBase);
		}
		argvOffsets[argStrings.size()] = 0;
		outInvokeArgs = {(U32)argStrings.size(), (U32)((U8*)argvOffsets - emscriptenMemoryBase) };
	}
}
