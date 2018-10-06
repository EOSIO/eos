#pragma once

#include <chrono>

#include "Logging/Logging.h"

namespace Timing
{
	// Encapsulates a timer that starts when constructed and stops when read.
	struct Timer
	{
		Timer(): startTime(std::chrono::high_resolution_clock::now()), isStopped(false) {}
		void stop() { endTime = std::chrono::high_resolution_clock::now(); }
		U64 getMicroseconds()
		{
			if(!isStopped) { stop(); }
			return std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
		}
		F64 getMilliseconds() { return getMicroseconds() / 1000.0; }
		F64 getSeconds() { return getMicroseconds() / 1000000.0; }
	private:
		std::chrono::high_resolution_clock::time_point startTime;
		std::chrono::high_resolution_clock::time_point endTime;
		bool isStopped;
	};
	
	// Helpers for printing timers.
	inline void logTimer(const char* context,Timer& timer) { Log::printf(Log::Category::metrics,"%s in %.2fms\n",context,timer.getMilliseconds()); }
	inline void logRatePerSecond(const char* context,Timer& timer,F64 numerator,const char* numeratorUnit)
	{
		Log::printf(Log::Category::metrics,"%s in %.2fms (%f %s/s)\n",
			context,
			timer.getMilliseconds(),
			numerator / timer.getSeconds(),
			numeratorUnit
			);
	}
}