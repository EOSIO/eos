/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

// if we support wstring make sure that this is correct
typedef wchar_t wint_t;
#define WEOF wchar_t(-1)

struct mbstate_t;

extern "C" {

   // Not implemented yet:

   wchar_t* wcschr(const wchar_t* s, wchar_t c);

   wchar_t* wcspbrk(const wchar_t* s1, const wchar_t* s2);

   wchar_t* wcsstr(const wchar_t*  s1, const wchar_t* s2);

   wchar_t* wcsrchr(const wchar_t*s, wchar_t c);

   wchar_t* wmemchr(const wchar_t* s, wchar_t c, size_t n);

   // why compiler doesn't like 'restrict' here?
    
   wchar_t* wmemmove(wchar_t* s1, const wchar_t* s2, size_t n);

   wchar_t* wmemcpy(wchar_t* s1, const wchar_t* s2, size_t n);

   wchar_t* wmemset(wchar_t *s, wchar_t c, size_t n);
}

