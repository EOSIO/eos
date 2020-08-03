#pragma once

#ifdef ENABLE_CPPKIN

#include <fc/scoped_exit.hpp>
#include <cppkin.h>

/// Simple wrappers for cppkin tracing

inline bool fc_cppkin_trace_enabled = false;

/// @param VARNAME unique within scope variable name for created trace
/// @param TRACE_STR const char* identifier for trace
#define fc_create_trace( VARNAME, TRACE_STR ) \
  std::optional<cppkin::Trace> VARNAME; \
  if( fc_cppkin_trace_enabled ) VARNAME = cppkin::Trace( TRACE_STR ); \
  auto VARNAME##_submit = fc::make_scoped_exit([&VARNAME] { \
    if( VARNAME ) VARNAME->Submit(); \
  }); \

/// @param TRACE_VARNAME variable name provided (VARNAME) provided to fc_create_trace
/// @param VARNAME unique within scope variable name for created span
/// @param SPAN_STR const char* indentifier
#define fc_create_span( TRACE_VARNAME, VARNAME, SPAN_STR ) \
  std::optional<cppkin::Span> VARNAME; \
  if( fc_cppkin_trace_enabled ) VARNAME = TRACE_VARNAME->CreateSpan(SPAN_STR); \
  auto VARNAME##_submit = fc::make_scoped_exit([&VARNAME] {\
    if( VARNAME ) VARNAME->Submit(); \
  }); \

/// @param SPAN_VARNAME variable name (VARNAME) provided to fc_create_span
/// @param TAG_KEY_STR const char* key
/// @param TAG_VALUE int, float, bool, const char*
#define fc_add_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE) \
  if( SPAN_VARNAME ) SPAN_VARNAME->AddSimpleTag(TAG_KEY_STR, TAG_VALUE);

/// @param SPAN_VARNAME variable name (VARNAME) provided to fc_create_span
/// @param TAG_KEY_STR const char* key
/// @param TAG_VALUE_STR std::string value
#define fc_add_str_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE_STR) \
  if( SPAN_VARNAME ) SPAN_VARNAME->AddSimpleTag(TAG_KEY_STR, TAG_VALUE_STR.c_str());

#else // ENABLE_CPPKIN

#define fc_create_trace( VARNAME, TRACE_STR )
#define fc_create_span( TRACE_VARNAME, VARNAME, SPAN_STR )
#define fc_add_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE)
#define fc_add_str_tag( SPAN_VARNAME, TAG_KEY_STR, TAG_VALUE_STR)

#endif // ENABLE_CPPKIN
