#pragma once

#include <bits/wchar.h>
#include <bits/stdint.h>

typedef size_t ssize_t;

typedef __builtin_va_list __isoc_va_list;

typedef unsigned long wctype_t;

typedef struct __mbstate_t { unsigned __opaque1, __opaque2; } mbstate_t;

typedef int regoff_t; //questionable (_Addr in alltypes.h.in)

struct __locale_struct;

typedef struct __locale_struct * locale_t;

typedef struct _IO_FILE FILE;

typedef int64_t off_t;

typedef unsigned long wctype_t;
