/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <type_traits>
#include <limits>

#include <eosiolib/system.h>
#include <eosiolib/print.h>

namespace eosio {

   namespace _detail_safe_number {

      template<typename NumberType, bool IsUnsigned>
      class safe_number_base
      {
      public:
         static_assert( std::is_arithmetic<NumberType>::value && std::is_signed<NumberType>::value,
                        "NumberType in safe_number_base must either be a floating point or a signed integer" );

         typedef NumberType number_type;

         /// signed_number_type is the signed version of the integral number_type (aka NumberType).
         ///  (signed_number_type is identical to number_type if it is floating point.)
         typedef typename std::conditional<std::is_floating_point<NumberType>::value,
                                         NumberType, typename std::make_signed<NumberType>::type>::type signed_number_type;

      protected:
         /// unsigned_number_type is the unsigned version of the integral number_type (aka NumberType).
         ///  (unsigned_number_type is identical to number_type if it is floating point.)
         typedef typename std::conditional<std::is_floating_point<NumberType>::value,
                                           NumberType, typename std::make_unsigned<NumberType>::type>::type unsigned_number_type;

         static_assert( std::is_floating_point<NumberType>::value || IsUnsigned ||
                       (-(std::numeric_limits<NumberType>::lowest()/2) == std::numeric_limits<NumberType>::max()/2 + 1),
                       "failure in positive/negative symmetry assumption for signed integral NumberType" );
         static_assert( !std::is_floating_point<NumberType>::value ||
                       (-(std::numeric_limits<NumberType>::lowest()/2) == std::numeric_limits<NumberType>::max()/2),
                       "failure in positive/negative symmetry assumption for floating point NumberType" );

         static constexpr NumberType lower_limit = (std::numeric_limits<NumberType>::lowest() / 2); // should be 0 if IsUnsigned
         static constexpr NumberType upper_limit = IsUnsigned ? (std::numeric_limits<NumberType>::max()/2 + 1) :  -lower_limit;
         /// safe_number must maintain invariant that (-lower_limit < number < upper_limit) or (number == 0)

         NumberType number;

         template<class T>
         inline T explicit_to( typename std::enable_if<!std::is_floating_point<NumberType>::value && !IsUnsigned &&
                                                       std::is_same<T, unsigned_number_type>::value>::type* = nullptr )const {
            eosio_assert( number >= NumberType(), "casting negative integer to unsigned integer" );
            return static_cast<unsigned_number_type>(number);
         }

         template<class T>
         inline T implicit_to( typename std::enable_if<IsUnsigned && std::is_same<T, unsigned_number_type>::value>::type* = nullptr )const {
            return static_cast<unsigned_number_type>(number);
         }

         template<class T>
         inline T implicit_to( typename std::enable_if<std::is_same<T, signed_number_type>::value>::type* = nullptr )const {
            return number;
         }

         template<typename T>
         void _print(typename std::enable_if<std::is_same<T, NumberType>::value &&
                                             !std::is_floating_point<NumberType>::value>::type* = nullptr)const {
            printi(number);
         }

         template<typename T>
         void _print(typename std::enable_if<std::is_same<T, NumberType>::value &&
                                             std::is_floating_point<NumberType>::value>::type* = nullptr)const {
            printd(static_cast<T>(number));
         }


         template<typename T>
         inline
         typename std::enable_if<!std::is_floating_point<NumberType>::value &&
                                 std::is_same<T, unsigned_number_type>::value>::type
         _multiply_by( const safe_number_base& q ) {
            T orig_number{static_cast<T>(number)};
            number = static_cast<signed_number_type>( orig_number * static_cast<T>(q.number) );
            eosio_assert( q.number == 0 ||
                          ( ( static_cast<T>(number / q.number) == orig_number ) && is_valid() ), "overflow/underflow");
         }

         template<typename T>
         inline
         typename std::enable_if<!std::is_floating_point<NumberType>::value &&
                                 std::is_same<T, signed_number_type>::value>::type
         _multiply_by( const safe_number_base& q ) {
            // Convoluted process required to avoid undefined behavior (signed integer overflow)
            auto lhs = static_cast<unsigned_number_type>(std::abs(number));
            auto rhs = static_cast<unsigned_number_type>(std::abs(q.number));
            auto product = lhs * rhs;
            eosio_assert( rhs == 0 || ( product / rhs == lhs ), "overflow/underflow");
            if( (number >= 0) == (q.number >= 0) ) // product should have positive sign (unless it is zero)
               number = static_cast<signed_number_type>(product);
            else // product should have negative sign (unless it is zero)
               number = -static_cast<signed_number_type>(product);
            eosio_assert( is_valid(), "overflow/underflow" );
         }

         template<typename T>
         inline
         typename std::enable_if<std::is_floating_point<NumberType>::value &&
                                 std::is_same<T, NumberType>::value>::type
         _multiply_by( const safe_number_base& q ) {
            NumberType orig_number{number};
            number *= q.number;
            eosio_assert( is_valid(), "multiplication puts floating point number outside allowed range");
         }

         template<typename T>
         inline
         typename std::enable_if<!std::is_floating_point<NumberType>::value &&
                                 std::is_same<T, NumberType>::value>::type
         _divide_by( const safe_number_base& q ) {
            eosio_assert( q.number != NumberType(), "divide by zero" );
            number /= q.number;
         }

         template<typename T>
         inline
         typename std::enable_if<std::is_floating_point<NumberType>::value &&
                                 std::is_same<T, NumberType>::value>::type
         _divide_by( const safe_number_base& q ) {
            number /= q.number;
            eosio_assert( is_valid(), "division puts floating point number outside allowed range");
         }

      public:
         static_assert( ( lower_limit < NumberType() && NumberType() < upper_limit ) ||
                        ( IsUnsigned && lower_limit == NumberType() ),
                        "default constructor for NumberType creates value that is outside the allowed range" );

         safe_number_base() : number() {}

         explicit safe_number_base(const NumberType& v)
         : number(v) {
            eosio_assert( is_valid(), "safe_number constructed with value outside allowed range" );
         }


         template<typename T, typename Enable = typename std::enable_if<IsUnsigned ||
                                                                        std::is_same<T, signed_number_type>::value>::type>
         operator T()const { return implicit_to<T>(); } // Implicitly convert safe unsigned integer to signed integer or
                                                        // implicitly convert safe unsigned integer to unsigned integer or
                                                        // implicitly convert safe signed integer to signed integer or
                                                        // implicitly convert safe floating point number to floating point number.

         /// Explicitly convert safe signed integer to unsigned integer
         template<typename Enable = typename std::enable_if<!std::is_floating_point<NumberType>::value && !IsUnsigned>::type>
         explicit operator unsigned_number_type()const { return explicit_to<unsigned_number_type>(); }

         /// Explicitly convert safe unsigned/signed integer to float
         template<typename Enable = typename std::enable_if<!std::is_floating_point<NumberType>::value>::type>
         explicit operator float()const { return static_cast<float>(implicit_to<NumberType>()); }

         /// Explicitly convert safe unsigned/signed integer to double
         template<typename Enable = typename std::enable_if<!std::is_floating_point<NumberType>::value>::type>
         explicit operator double()const { return static_cast<double>(implicit_to<NumberType>()); }

         /// Explicitly convert safe unsigned/signed integer to long double
         template<typename Enable = typename std::enable_if<!std::is_floating_point<NumberType>::value>::type>
         explicit operator long double()const { return static_cast<long double>(implicit_to<NumberType>()); }

         constexpr bool is_valid()const {
            return ( lower_limit < number && number < upper_limit ) || ( IsUnsigned && number == NumberType() );
         }

         typename std::conditional<IsUnsigned, unsigned_number_type, signed_number_type>::type
         get_number()const {
            return static_cast<typename std::conditional<IsUnsigned, unsigned_number_type, signed_number_type>::type>(number);
         }

         void set_number(const typename std::conditional<IsUnsigned, unsigned_number_type, signed_number_type>::type& v) {
            number = static_cast<signed_number_type>(v);
            eosio_assert( v < static_cast<typename std::conditional<IsUnsigned, unsigned_number_type, signed_number_type>::type>(upper_limit) && is_valid(),
                          "safe_number::set_number called with value outside allowed range" );
         }

         explicit operator bool()const { return number != NumberType(); }

         inline friend bool operator <=( const safe_number_base& lhs, const safe_number_base& rhs ) { return lhs.number <= rhs.number; }
         inline friend bool operator < ( const safe_number_base& lhs, const safe_number_base& rhs ) { return lhs.number <  rhs.number; }
         inline friend bool operator >=( const safe_number_base& lhs, const safe_number_base& rhs ) { return lhs.number >= rhs.number; }
         inline friend bool operator > ( const safe_number_base& lhs, const safe_number_base& rhs ) { return lhs.number >  rhs.number; }
         inline friend bool operator ==( const safe_number_base& lhs, const safe_number_base& rhs ) { return lhs.number == rhs.number; }
         inline friend bool operator !=( const safe_number_base& lhs, const safe_number_base& rhs ) { return lhs.number != rhs.number; }

         template<typename T>
         typename std::enable_if<std::is_same<T, safe_number_base>::value && !IsUnsigned, T>::type
         operator-()const {
            safe_number_base q = *this;
            q.number = -q.number;
            return q;
         }

         safe_number_base& operator +=( const safe_number_base& q ) {
            number += q.number;
            eosio_assert( IsUnsigned || (lower_limit < number), "underflow" );
            eosio_assert( number < upper_limit, "overflow" );
            return *this;
         }

         safe_number_base& operator -=( const safe_number_base& q ) {
            eosio_assert( !IsUnsigned || number >= q.number, "underflow" );
            number -= q.number;
            eosio_assert( IsUnsigned || lower_limit < number, "underflow" );
            eosio_assert( number < upper_limit, "overflow" );
            return *this;
         }

         safe_number_base& operator *=( const safe_number_base& q ) {
            _multiply_by<NumberType>(q);
            return *this;
         }

         safe_number_base& operator /=( const safe_number_base& q ) {
            _divide_by<NumberType>(q);
            return *this;
         }

         safe_number_base& operator %=( const safe_number_base& q ) {
            eosio_assert( q.number != NumberType(), "divide by zero while getting remainder" );
            number %= q.number; // Will fail if NumberType is floating point.
            return *this;
         }

         void print()const {
            _print<NumberType>();
         }

         template<typename DataStream>
         friend DataStream& operator << ( DataStream& ds, const safe_number_base& q ){
            return ds << q.number;
         }

         template<typename DataStream>
         friend DataStream& operator >> ( DataStream& ds, safe_number_base& q ){
            ds >> q.number;
            eosio_assert( q.is_valid(), "deserialized safe_number has value outside allowed range" );
            return ds;
         }

      };

   } /// namespace eosio::_detail_safe_number


   template<typename NumberType = int64_t, uint128_t Unit = 0>
   class safe_number : public _detail_safe_number::safe_number_base<typename std::make_signed<NumberType>::type,
                                                                    std::is_unsigned<NumberType>::value>
   {
   protected:
      static_assert( std::is_arithmetic<NumberType>::value, "NumberType must be arithmetic" );

      typedef _detail_safe_number::safe_number_base<typename std::make_signed<NumberType>::type,
                                                        std::is_unsigned<NumberType>::value>         base_type;

   public:
      typedef safe_number<NumberType, Unit> safe_number_type;

      static constexpr bool is_unsigned = std::is_unsigned<NumberType>::value;

      safe_number() : base_type() {}

      explicit safe_number( const NumberType& v )
      : base_type(static_cast<typename std::make_signed<NumberType>::type>(v))
      {
         eosio_assert( v < static_cast<NumberType>(base_type::upper_limit),
                      "safe_number constructed with value outside allowed range" );
      }

      safe_number operator-()const {
         base_type r = this->base_type::template operator-<base_type>();
         return safe_number{r};
      }

      safe_number& operator +=( const safe_number& q ) {
         static_cast<base_type&>(*this) += static_cast<const base_type&>(q);
         return *this;
      }

      safe_number& operator -=( const safe_number& q ) {
         static_cast<base_type&>(*this) -= static_cast<const base_type&>(q);
         return *this;
      }

      safe_number& operator *=( const safe_number& q ) {
         static_cast<base_type&>(*this) *= static_cast<const base_type&>(q);
         return *this;
      }

      safe_number& operator /=( const safe_number& q ) {
         static_cast<base_type&>(*this) /= static_cast<const base_type&>(q);
         return *this;
      }

      safe_number& operator %=( const safe_number& q ) {
         static_cast<base_type&>(*this) %= static_cast<const base_type&>(q);
         return *this;
      }

      inline friend safe_number_type operator +( const safe_number_type& lhs, const safe_number_type& rhs ) {
         safe_number sum = lhs;
         sum += rhs;
         return sum;
      }

      inline friend safe_number_type operator -( const safe_number_type& lhs, const safe_number_type& rhs ) {
         safe_number difference = lhs;
         difference -= rhs;
         return difference;
      }

      inline friend safe_number_type operator *( const safe_number_type& lhs, const safe_number_type& rhs ) {
         safe_number product = lhs;
         product *= rhs;
         return product;
      }

      inline friend safe_number_type operator /( const safe_number_type& lhs, const safe_number_type& rhs ) {
         safe_number quotient = lhs;
         quotient /= rhs;
         return quotient;
      }

      inline friend safe_number_type operator %( const safe_number_type& lhs, const safe_number_type& rhs ) {
         safe_number remainder = lhs;
         remainder %= rhs;
         return remainder;
      }

   };

   template<typename NumberType = int64_t, uint128_t Unit = 0>
   safe_number<NumberType, Unit> make_safe( const NumberType& v ) {
      return safe_number<NumberType, Unit>{v};
   }

} /// namespace eosio
