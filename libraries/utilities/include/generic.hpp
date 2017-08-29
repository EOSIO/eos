#pragma once

#include <string>
#include <string.h>
#include <stdarg.h>

extern char *va(char *buff, int size, const char *format, ... );
#define V(s...) va(tb, sizeof(tb), ## s)

