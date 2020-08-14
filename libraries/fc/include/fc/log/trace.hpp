#pragma once

#ifdef ENABLE_CPPKIN

#include <fc/scoped_exit.hpp>
#include <fc/log/logger.hpp>
#include <cppkin.h>

/// Simple wrappers for cppkin tracing

inline bool fc_cppkin_trace_enabled = false;


/// @param TRACE_STR const char* identifier for trace
/// @return implementation defined type RAII object that submits trace on exit of scope
#define fc_create_trace( TRACE_STR ) \
  ::fc::make_wrapped_scoped_exit([](::std::optional<::cppkin::Trace>& t) { \
        if(t) t->Submit(); \
      }, \
      fc_cppkin_trace_enabled ? ::std::optional<::cppkin::Trace>( ::cppkin::Trace( TRACE_STR ) ) \
                              : ::std::optional<::cppkin::Trace>() \
  )

/// @param TRACE_VARNAME variable returned from fc_create_trace
/// @param SPAN_STR const char* indentifier
/// @return implementation defined type RAII object that submits span on exit of scope
#define fc_create_span( TRACE_VARNAME, SPAN_STR ) \
  ::fc::make_wrapped_scoped_exit([](::std::optional<::cppkin::Span>& s) { \
        if(s) s->Submit(); \
      }, \
      fc_cppkin_trace_enabled ? ::std::optional<::cppkin::Span>( TRACE_VARNAME.get_wrapped()->CreateSpan(SPAN_STR) ) \
                              : ::std::optional<::cppkin::Span>() \
  )

/// @param SPAN_VARNAME variable returned from fc_create_span
/// @param TAG_KEY_STR const char* key
/// @param TAG_VALUE int, float, bool, const char*
#define fc_add_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE) \
  FC_MULTILINE_MACRO_BEGIN \
    if( SPAN_VARNAME.get_wrapped() ) SPAN_VARNAME.get_wrapped()->AddSimpleTag(TAG_KEY_STR, TAG_VALUE); \
  FC_MULTILINE_MACRO_END

/// @param SPAN_VARNAME variable returned from fc_create_span
/// @param TAG_KEY_STR const char* key
/// @param TAG_VALUE_STR std::string value
#define fc_add_str_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE_STR) \
  FC_MULTILINE_MACRO_BEGIN \
    if( SPAN_VARNAME.get_wrapped() ) SPAN_VARNAME.get_wrapped()->AddSimpleTag(TAG_KEY_STR, (TAG_VALUE_STR).c_str()); \
  FC_MULTILINE_MACRO_END

#else // ENABLE_CPPKIN

#define fc_create_trace( TRACE_STR ) 1
#define fc_create_span( TRACE_VARNAME, SPAN_STR ) \
  TRACE_VARNAME

#define fc_add_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE) \
  (void)SPAN_VARNAME

#define fc_add_str_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE_STR) \
  (void)SPAN_VARNAME

#endif // ENABLE_CPPKIN
