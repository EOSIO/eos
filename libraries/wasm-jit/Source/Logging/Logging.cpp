#include "Logging.h"
#include "Platform/Platform.h"

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

namespace Log
{
	static bool categoryEnabled[(Uptr)Category::num] =
	{
		true, // error
		#ifdef _DEBUG // debug
			true,
		#else
			false,
		#endif
		WAVM_METRICS_OUTPUT != 0 // metrics
	};
	void setCategoryEnabled(Category category,bool enable)
	{
		WAVM_ASSERT_THROW(category < Category::num);
		categoryEnabled[(Uptr)category] = enable;
	}
	bool isCategoryEnabled(Category category)
	{
		WAVM_ASSERT_THROW(category < Category::num);
		return categoryEnabled[(Uptr)category];
	}
	void printf(Category category,const char* format,...)
	{
		if(categoryEnabled[(Uptr)category])
		{
			va_list varArgs;
			va_start(varArgs,format);
			vfprintf(stdout,format,varArgs);
			fflush(stdout);
			va_end(varArgs);
		}
	}
}
