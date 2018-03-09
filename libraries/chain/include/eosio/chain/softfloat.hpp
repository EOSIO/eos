#pragma once
#include <softfloat_types.h>

#ifndef THREAD_LOCAL
#define THREAD_LOCAL __thread
#endif
extern "C" {
/*----------------------------------------------------------------------------
| Integer-to-floating-point conversion routines.
*----------------------------------------------------------------------------*/
float16_t ui32_to_f16( uint32_t );
float32_t ui32_to_f32( uint32_t );
float64_t ui32_to_f64( uint32_t );
float128_t ui32_to_f128( uint32_t );
void ui32_to_f128M( uint32_t, float128_t * );
float16_t ui64_to_f16( uint64_t );
float32_t ui64_to_f32( uint64_t );
float64_t ui64_to_f64( uint64_t );
float128_t ui64_to_f128( uint64_t );
void ui64_to_f128M( uint64_t, float128_t * );
float16_t i32_to_f16( int32_t );
float32_t i32_to_f32( int32_t );
float64_t i32_to_f64( int32_t );
float128_t i32_to_f128( int32_t );
void i32_to_f128M( int32_t, float128_t * );
float16_t i64_to_f16( int64_t );
float32_t i64_to_f32( int64_t );
float64_t i64_to_f64( int64_t );
float128_t i64_to_f128( int64_t );
void i64_to_f128M( int64_t, float128_t * );

/*----------------------------------------------------------------------------
| 16-bit (half-precision) floating-point operations.
*----------------------------------------------------------------------------*/
uint_fast32_t f16_to_ui32( float16_t, uint_fast8_t, bool );
uint_fast64_t f16_to_ui64( float16_t, uint_fast8_t, bool );
int_fast32_t f16_to_i32( float16_t, uint_fast8_t, bool );
int_fast64_t f16_to_i64( float16_t, uint_fast8_t, bool );
uint_fast32_t f16_to_ui32_r_minMag( float16_t, bool );
uint_fast64_t f16_to_ui64_r_minMag( float16_t, bool );
int_fast32_t f16_to_i32_r_minMag( float16_t, bool );
int_fast64_t f16_to_i64_r_minMag( float16_t, bool );
float32_t f16_to_f32( float16_t );
float64_t f16_to_f64( float16_t );
float128_t f16_to_f128( float16_t );
void f16_to_f128M( float16_t, float128_t * );
float16_t f16_roundToInt( float16_t, uint_fast8_t, bool );
float16_t f16_add( float16_t, float16_t );
float16_t f16_sub( float16_t, float16_t );
float16_t f16_mul( float16_t, float16_t );
float16_t f16_mulAdd( float16_t, float16_t, float16_t );
float16_t f16_div( float16_t, float16_t );
float16_t f16_rem( float16_t, float16_t );
float16_t f16_sqrt( float16_t );
bool f16_eq( float16_t, float16_t );
bool f16_le( float16_t, float16_t );
bool f16_lt( float16_t, float16_t );
bool f16_eq_signaling( float16_t, float16_t );
bool f16_le_quiet( float16_t, float16_t );
bool f16_lt_quiet( float16_t, float16_t );
bool f16_isSignalingNaN( float16_t );

/*----------------------------------------------------------------------------
| 32-bit (single-precision) floating-point operations.
*----------------------------------------------------------------------------*/
uint_fast32_t f32_to_ui32( float32_t, uint_fast8_t, bool );
uint_fast64_t f32_to_ui64( float32_t, uint_fast8_t, bool );
int_fast32_t f32_to_i32( float32_t, uint_fast8_t, bool );
int_fast64_t f32_to_i64( float32_t, uint_fast8_t, bool );
uint_fast32_t f32_to_ui32_r_minMag( float32_t, bool );
uint_fast64_t f32_to_ui64_r_minMag( float32_t, bool );
int_fast32_t f32_to_i32_r_minMag( float32_t, bool );
int_fast64_t f32_to_i64_r_minMag( float32_t, bool );
float16_t f32_to_f16( float32_t );
float64_t f32_to_f64( float32_t );
float128_t f32_to_f128( float32_t );

void f32_to_f128M( float32_t, float128_t * );
float32_t f32_roundToInt( float32_t, uint_fast8_t, bool );
float32_t f32_add( float32_t, float32_t );
float32_t f32_sub( float32_t, float32_t );
float32_t f32_mul( float32_t, float32_t );
float32_t f32_mulAdd( float32_t, float32_t, float32_t );
float32_t f32_div( float32_t, float32_t );
float32_t f32_rem( float32_t, float32_t );
float32_t f32_sqrt( float32_t );
bool f32_eq( float32_t, float32_t );
bool f32_le( float32_t, float32_t );
bool f32_lt( float32_t, float32_t );
bool f32_eq_signaling( float32_t, float32_t );
bool f32_le_quiet( float32_t, float32_t );
bool f32_lt_quiet( float32_t, float32_t );
bool f32_isSignalingNaN( float32_t );

/*----------------------------------------------------------------------------
| 64-bit (double-precision) floating-point operations.
*----------------------------------------------------------------------------*/
uint_fast32_t f64_to_ui32( float64_t, uint_fast8_t, bool );
uint_fast64_t f64_to_ui64( float64_t, uint_fast8_t, bool );
int_fast32_t f64_to_i32( float64_t, uint_fast8_t, bool );
int_fast64_t f64_to_i64( float64_t, uint_fast8_t, bool );
uint_fast32_t f64_to_ui32_r_minMag( float64_t, bool );
uint_fast64_t f64_to_ui64_r_minMag( float64_t, bool );
int_fast32_t f64_to_i32_r_minMag( float64_t, bool );
int_fast64_t f64_to_i64_r_minMag( float64_t, bool );
float16_t f64_to_f16( float64_t );
float32_t f64_to_f32( float64_t );
float128_t f64_to_f128( float64_t );
void f64_to_f128M( float64_t, float128_t * );
float64_t f64_roundToInt( float64_t, uint_fast8_t, bool );
float64_t f64_add( float64_t, float64_t );
float64_t f64_sub( float64_t, float64_t );
float64_t f64_mul( float64_t, float64_t );
float64_t f64_mulAdd( float64_t, float64_t, float64_t );
float64_t f64_div( float64_t, float64_t );
float64_t f64_rem( float64_t, float64_t );
float64_t f64_sqrt( float64_t );
bool f64_eq( float64_t, float64_t );
bool f64_le( float64_t, float64_t );
bool f64_lt( float64_t, float64_t );
bool f64_eq_signaling( float64_t, float64_t );
bool f64_le_quiet( float64_t, float64_t );
bool f64_lt_quiet( float64_t, float64_t );
bool f64_isSignalingNaN( float64_t );

/*----------------------------------------------------------------------------
| 128-bit (quadruple-precision) floating-point operations.
*----------------------------------------------------------------------------*/
uint_fast32_t f128_to_ui32( float128_t, uint_fast8_t, bool );
uint_fast64_t f128_to_ui64( float128_t, uint_fast8_t, bool );
int_fast32_t f128_to_i32( float128_t, uint_fast8_t, bool );
int_fast64_t f128_to_i64( float128_t, uint_fast8_t, bool );
uint_fast32_t f128_to_ui32_r_minMag( float128_t, bool );
uint_fast64_t f128_to_ui64_r_minMag( float128_t, bool );
int_fast32_t f128_to_i32_r_minMag( float128_t, bool );
int_fast64_t f128_to_i64_r_minMag( float128_t, bool );
float16_t f128_to_f16( float128_t );
float32_t f128_to_f32( float128_t );
float64_t f128_to_f64( float128_t );
float128_t f128_roundToInt( float128_t, uint_fast8_t, bool );
float128_t f128_add( float128_t, float128_t );
float128_t f128_sub( float128_t, float128_t );
float128_t f128_mul( float128_t, float128_t );
float128_t f128_mulAdd( float128_t, float128_t, float128_t );
float128_t f128_div( float128_t, float128_t );
float128_t f128_rem( float128_t, float128_t );
float128_t f128_sqrt( float128_t );
bool f128_eq( float128_t, float128_t );
bool f128_le( float128_t, float128_t );
bool f128_lt( float128_t, float128_t );
bool f128_eq_signaling( float128_t, float128_t );
bool f128_le_quiet( float128_t, float128_t );
bool f128_lt_quiet( float128_t, float128_t );
bool f128_isSignalingNaN( float128_t );

uint_fast32_t f128M_to_ui32( const float128_t *, uint_fast8_t, bool );
uint_fast64_t f128M_to_ui64( const float128_t *, uint_fast8_t, bool );
int_fast32_t f128M_to_i32( const float128_t *, uint_fast8_t, bool );
int_fast64_t f128M_to_i64( const float128_t *, uint_fast8_t, bool );
uint_fast32_t f128M_to_ui32_r_minMag( const float128_t *, bool );
uint_fast64_t f128M_to_ui64_r_minMag( const float128_t *, bool );
int_fast32_t f128M_to_i32_r_minMag( const float128_t *, bool );
int_fast64_t f128M_to_i64_r_minMag( const float128_t *, bool );
float16_t f128M_to_f16( const float128_t * );
float32_t f128M_to_f32( const float128_t * );
float64_t f128M_to_f64( const float128_t * );
void f128M_roundToInt( const float128_t *, uint_fast8_t, bool, float128_t * );
void f128M_add( const float128_t *, const float128_t *, float128_t * );
void f128M_sub( const float128_t *, const float128_t *, float128_t * );
void f128M_mul( const float128_t *, const float128_t *, float128_t * );
void
 f128M_mulAdd(
     const float128_t *, const float128_t *, const float128_t *, float128_t *
 );
void f128M_div( const float128_t *, const float128_t *, float128_t * );
void f128M_rem( const float128_t *, const float128_t *, float128_t * );
void f128M_sqrt( const float128_t *, float128_t * );
bool f128M_eq( const float128_t *, const float128_t * );
bool f128M_le( const float128_t *, const float128_t * );
bool f128M_lt( const float128_t *, const float128_t * );
bool f128M_eq_signaling( const float128_t *, const float128_t * );
bool f128M_le_quiet( const float128_t *, const float128_t * );
bool f128M_lt_quiet( const float128_t *, const float128_t * );
bool f128M_isSignalingNaN( const float128_t * );
};
