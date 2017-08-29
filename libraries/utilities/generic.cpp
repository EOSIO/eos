#include <string>
#include <string.h>
#include <stdarg.h>
#include <generic.hpp>

using namespace std;

char *va(char *buff, int size, const char *format, ... )
{
   va_list argptr;
   va_start(argptr, format);
   vsnprintf(buff, size, format, argptr);
   va_end(argptr);
   return buff;
}

