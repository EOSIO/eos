#pragma once
#include <fc/exception/exception.hpp>
#include <fc/reflect/reflect.hpp>

#include <limits>

namespace fc {

   /**
    *  This type is designed to provide automatic checks for
    *  integer overflow and default initialization. It will
    *  throw an exception on overflow conditions.
    *
    *  It can only be used on built-in types.  In particular,
    *  safe<uint128_t> is buggy and should not be used.
    *
    *  Implemented using spec from:
    *  https://www.securecoding.cert.org/confluence/display/c/INT32-C.+Ensure+that+operations+on+signed+integers+do+not+result+in+overflow
    */
   template<typename T>
   struct safe
   {
      T value = 0;

      template<typename O>
      safe( O o ):value(o){}
      safe(){}
      safe( const safe& o ):value(o.value){}

      static safe min()
      {
          return std::numeric_limits<T>::min();
      }
      static safe max()
      {
          return std::numeric_limits<T>::max();
      }

      friend safe operator + ( const safe& a, const safe& b )
      {
          if( b.value > 0 && a.value > (std::numeric_limits<T>::max() - b.value) ) FC_CAPTURE_AND_THROW( overflow_exception, (a)(b) );
          if( b.value < 0 && a.value < (std::numeric_limits<T>::min() - b.value) ) FC_CAPTURE_AND_THROW( underflow_exception, (a)(b) );
          return safe( a.value + b.value );
      }
      friend safe operator - ( const safe& a, const safe& b )
      {
          if( b.value > 0 && a.value < (std::numeric_limits<T>::min() + b.value) ) FC_CAPTURE_AND_THROW( underflow_exception, (a)(b) );
          if( b.value < 0 && a.value > (std::numeric_limits<T>::max() + b.value) ) FC_CAPTURE_AND_THROW( overflow_exception, (a)(b) );
          return safe( a.value - b.value );
      }

      friend safe operator * ( const safe& a, const safe& b )
      {
          if( a.value > 0 )
          {
              if( b.value > 0 )
              {
                  if( a.value > (std::numeric_limits<T>::max() / b.value) ) FC_CAPTURE_AND_THROW( overflow_exception, (a)(b) );
              }
              else
              {
                  if( b.value < (std::numeric_limits<T>::min() / a.value) ) FC_CAPTURE_AND_THROW( underflow_exception, (a)(b) );
              }
          }
          else
          {
              if( b.value > 0 )
              {
                  if( a.value < (std::numeric_limits<T>::min() / b.value) ) FC_CAPTURE_AND_THROW( underflow_exception, (a)(b) );
              }
              else
              {
                  if( a.value != 0 && b.value < (std::numeric_limits<T>::max() / a.value) ) FC_CAPTURE_AND_THROW( overflow_exception, (a)(b) );
              }
          }

          return safe( a.value * b.value );
      }

      friend safe operator / ( const safe& a, const safe& b )
      {
          if( b.value == 0 ) FC_CAPTURE_AND_THROW( divide_by_zero_exception, (a)(b) );
          if( a.value == std::numeric_limits<T>::min() && b.value == -1 ) FC_CAPTURE_AND_THROW( overflow_exception, (a)(b) );
          return safe( a.value / b.value );
      }
      friend safe operator % ( const safe& a, const safe& b )
      {
          if( b.value == 0 ) FC_CAPTURE_AND_THROW( divide_by_zero_exception, (a)(b) );
          if( a.value == std::numeric_limits<T>::min() && b.value == -1 ) FC_CAPTURE_AND_THROW( overflow_exception, (a)(b) );
          return safe( a.value % b.value );
      }

      safe operator - ()const
      {
          if( value == std::numeric_limits<T>::min() ) FC_CAPTURE_AND_THROW( overflow_exception, (*this) );
          return safe( -value );
      }

      safe& operator += ( const safe& b )
      {
          value = (*this + b).value;
          return *this;
      }
      safe& operator -= ( const safe& b )
      {
          value = (*this - b).value;
          return *this;
      }
      safe& operator *= ( const safe& b )
      {
          value = (*this * b).value;
          return *this;
      }
      safe& operator /= ( const safe& b )
      {
          value = (*this / b).value;
          return *this;
      }
      safe& operator %= ( const safe& b )
      {
          value = (*this % b).value;
          return *this;
      }

      safe& operator++()
      {
          *this += 1;
          return *this;
      }
      safe operator++( int )
      {
          safe bak = *this;
          *this += 1;
          return bak;
      }

      safe& operator--()
      {
          *this -= 1;
          return *this;
      }
      safe operator--( int )
      {
          safe bak = *this;
          *this -= 1;
          return bak;
      }

      friend bool operator == ( const safe& a, const safe& b )
      {
          return a.value == b.value;
      }
      friend bool operator == ( const safe& a, const T& b )
      {
          return a.value == b;
      }
      friend bool operator == ( const T& a, const safe& b )
      {
          return a == b.value;
      }

      friend bool operator < ( const safe& a, const safe& b )
      {
          return a.value < b.value;
      }
      friend bool operator < ( const safe& a, const T& b )
      {
          return a.value < b;
      }
      friend bool operator < ( const T& a, const safe& b )
      {
          return a < b.value;
      }

      friend bool operator > ( const safe& a, const safe& b )
      {
          return a.value > b.value;
      }
      friend bool operator > ( const safe& a, const T& b )
      {
          return a.value > b;
      }
      friend bool operator > ( const T& a, const safe& b )
      {
          return a > b.value;
      }

      friend bool operator != ( const safe& a, const safe& b )
      {
          return !(a == b);
      }
      friend bool operator != ( const safe& a, const T& b )
      {
          return !(a == b);
      }
      friend bool operator != ( const T& a, const safe& b )
      {
          return !(a == b);
      }

      friend bool operator <= ( const safe& a, const safe& b )
      {
          return !(a > b);
      }
      friend bool operator <= ( const safe& a, const T& b )
      {
          return !(a > b);
      }
      friend bool operator <= ( const T& a, const safe& b )
      {
          return !(a > b);
      }

      friend bool operator >= ( const safe& a, const safe& b )
      {
          return !(a < b);
      }
      friend bool operator >= ( const safe& a, const T& b )
      {
          return !(a < b);
      }
      friend bool operator >= ( const T& a, const safe& b )
      {
          return !(a < b);
      }
   };

}

FC_REFLECT_TEMPLATE( (typename T), safe<T>, (value) )
