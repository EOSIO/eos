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

   namespace _detail_safe_quantity {

      template<typename NumberType, bool IsUnsigned>
      class safe_quantity_base
      {
      public:
         static_assert( std::is_arithmetic<NumberType>::value && std::is_signed<NumberType>::value,
                        "NumberType in safe_quantity_base must either be a floating point or a signed integer" );

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
         /// safe_quantity must maintain invariant that (-lower_limit < quantity < upper_limit) or (quantity == 0)

         NumberType quantity;

         template<class T>
         inline T explicit_to( typename std::enable_if<!std::is_floating_point<NumberType>::value && !IsUnsigned &&
                                                       std::is_same<T, unsigned_number_type>::value>::type* = nullptr )const {
            eosio_assert( quantity >= NumberType(), "casting negative integer to unsigned integer" );
            return static_cast<unsigned_number_type>(quantity);
         }

         template<class T>
         inline T implicit_to( typename std::enable_if<IsUnsigned && std::is_same<T, unsigned_number_type>::value>::type* = nullptr )const {
            return static_cast<unsigned_number_type>(quantity);
         }

         template<class T>
         inline T implicit_to( typename std::enable_if<std::is_same<T, signed_number_type>::value>::type* = nullptr )const {
            return quantity;
         }

         template<typename T>
         void _print(typename std::enable_if<std::is_same<T, NumberType>::value &&
                                             !std::is_floating_point<NumberType>::value>::type* = nullptr)const {
            printi(quantity);
         }

         template<typename T>
         void _print(typename std::enable_if<std::is_same<T, NumberType>::value &&
                                             std::is_floating_point<NumberType>::value>::type* = nullptr)const {
            printd(static_cast<T>(quantity));
         }

      public:
         static_assert( ( lower_limit < NumberType() && NumberType() < upper_limit ) ||
                        ( IsUnsigned && lower_limit == NumberType() ),
                        "default constructor for NumberType creates value that is outside the allowed range" );

         safe_quantity_base() : quantity() {}

         explicit safe_quantity_base(const NumberType& v)
         : quantity(v) {
            eosio_assert( is_valid(), "safe_quantity constructed with value outside allowed range" );
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
            return ( lower_limit < quantity && quantity < upper_limit ) || ( IsUnsigned && quantity == NumberType() );
         }

         typename std::conditional<IsUnsigned, unsigned_number_type, signed_number_type>::type
         get_quantity()const {
            return static_cast<typename std::conditional<IsUnsigned, unsigned_number_type, signed_number_type>::type>(quantity);
         }

         template<typename T>
         void set_quantity(const typename std::enable_if<std::is_same<T, signed_number_type>::value ||
                                                         std::is_same<T, unsigned_number_type>::value, T>::type& v) {
            quantity = static_cast<signed_number_type>(v);
            eosio_assert( v < static_cast<T>(upper_limit) && is_valid(),
                          "safe_quantity::set_quantity called with value outside allowed range" );
         }

         explicit operator bool()const { return quantity != NumberType(); }

         inline friend bool operator <=( const safe_quantity_base& lhs, const safe_quantity_base& rhs ) { return lhs.quantity <= rhs.quantity; }
         inline friend bool operator < ( const safe_quantity_base& lhs, const safe_quantity_base& rhs ) { return lhs.quantity <  rhs.quantity; }
         inline friend bool operator >=( const safe_quantity_base& lhs, const safe_quantity_base& rhs ) { return lhs.quantity >= rhs.quantity; }
         inline friend bool operator > ( const safe_quantity_base& lhs, const safe_quantity_base& rhs ) { return lhs.quantity >  rhs.quantity; }
         inline friend bool operator ==( const safe_quantity_base& lhs, const safe_quantity_base& rhs ) { return lhs.quantity == rhs.quantity; }
         inline friend bool operator !=( const safe_quantity_base& lhs, const safe_quantity_base& rhs ) { return lhs.quantity != rhs.quantity; }

         template<typename T>
         typename std::enable_if<std::is_same<T, safe_quantity_base>::value && !IsUnsigned, T>::type
         operator-()const {
            safe_quantity_base q = *this;
            q.quantity = -q.quantity;
            return q;
         }

         safe_quantity_base& operator +=( const safe_quantity_base& q ) {
            quantity += q.quantity;
            eosio_assert( IsUnsigned || (lower_limit < quantity), "underflow" );
            eosio_assert( quantity < upper_limit, "overflow" );
            return *this;
         }

         safe_quantity_base& operator -=( const safe_quantity_base& q ) {
            eosio_assert( !IsUnsigned || quantity >= q.quantity, "underflow" );
            quantity -= q.quantity;
            eosio_assert( IsUnsigned || lower_limit < quantity, "underflow" );
            eosio_assert( quantity < upper_limit, "overflow" );
            return *this;
         }

         void print()const {
            _print<NumberType>();
         }

         template<typename DataStream>
         friend DataStream& operator << ( DataStream& ds, const safe_quantity_base& q ){
            return ds << q.quantity;
         }

         template<typename DataStream>
         friend DataStream& operator >> ( DataStream& ds, safe_quantity_base& q ){
            ds >> q.quantity;
            eosio_assert( q.is_valid(), "deserialized safe_quantity has value outside allowed range" );
            return ds;
         }

      };

   } /// namespace eosio::_detail_safe_quantity


   template<typename NumberType = int64_t, uint128_t Unit = 0>
   class safe_quantity : public _detail_safe_quantity::safe_quantity_base<typename std::make_signed<NumberType>::type,
                                                                           std::is_unsigned<NumberType>::value>
   {
   protected:
      static_assert( std::is_arithmetic<NumberType>::value, "NumberType must be arithmetic" );

      typedef _detail_safe_quantity::safe_quantity_base<typename std::make_signed<NumberType>::type,
                                                        std::is_unsigned<NumberType>::value>         base_type;

   public:
      typedef safe_quantity<NumberType, Unit> safe_quantity_type;

      static constexpr bool is_unsigned = std::is_unsigned<NumberType>::value;

      safe_quantity() : base_type() {}

      explicit safe_quantity( const NumberType& v )
      : base_type(static_cast<typename std::make_signed<NumberType>::type>(v))
      {
         eosio_assert( v < static_cast<NumberType>(base_type::upper_limit),
                      "safe_quantity constructed with value outside allowed range" );
      }

      safe_quantity operator-()const {
         base_type r = this->base_type::template operator-<base_type>();
         return safe_quantity{r};
      }

      safe_quantity& operator +=( const safe_quantity& q ) {
         static_cast<base_type&>(*this) += static_cast<const base_type&>(q);
         return *this;
      }

      safe_quantity& operator -=( const safe_quantity& q ) {
         static_cast<base_type&>(*this) -= static_cast<const base_type&>(q);
         return *this;
      }

      inline friend safe_quantity_type operator +( const safe_quantity_type& lhs, const safe_quantity_type& rhs ) {
         safe_quantity sum = lhs;
         sum += rhs;
         return sum;
      }

      inline friend safe_quantity_type operator -( const safe_quantity_type& lhs, const safe_quantity_type& rhs ) {
         safe_quantity difference = lhs;
         difference -= rhs;
         return difference;
      }

   };

#if 0

   template<typename NumberType = int64_t, uint128_t Unit = 0>
   struct safe_quantity
   {
      static_assert( std::is_arithmetic<NumberType>::value, "NumberType must be arithmetic" );

      static constexpr bool is_unsigned = std::is_unsigned<NumberType>::value;

      typedef NumberType number_type;
      /// unsigned_number_type is the unsigned version of the integral number_type (aka NumberType).
      ///  (unsigned_number_type is identical to number_type if it is floating point.)
      typedef typename std::conditional<std::is_floating_point<NumberType>::value,
                                        NumberType, typename std::make_unsigned<NumberType>::type>::type unsigned_number_type;
      /// signed_number_type is the signed version of the integral number_type (aka NumberType).
      ///  (signed_number_type is identical to number_type if it is floating point.)
      typedef typename std::conditional<std::is_floating_point<NumberType>::value,
                                      NumberType, typename std::make_signed<NumberType>::type>::type signed_number_type;

      static_assert( std::is_floating_point<NumberType>::value || is_unsigned ||
                     (-(std::numeric_limits<NumberType>::lowest()/2) == std::numeric_limits<NumberType>::max()/2 + 1),
                     "failure in positive/negative symmetry assumption for signed integral NumberType" );
      static_assert( !std::is_floating_point<NumberType>::value ||
                     (-(std::numeric_limits<NumberType>::lowest()/2) == std::numeric_limits<NumberType>::max()/2),
                     "failure in positive/negative symmetry assumption for floating point NumberType" );

      static constexpr NumberType lower_limit = (std::numeric_limits<NumberType>::lowest() / 2); // should be 0 if is_unsigned
      static constexpr NumberType upper_limit = is_unsigned ? (std::numeric_limits<NumberType>::max()/2 + 1) :  -lower_limit;
      /// safe_quantity must maintain invariant that (-lower_limit < quantity < upper_limit) or (quantity == 0)

   protected:
      NumberType quantity;

      template<class T>
      inline T to( typename std::enable_if<std::is_same<T, number_type>::value>::type* = nullptr )const {
          return quantity;
      }

      template<class T>
      inline T to( typename std::enable_if<!std::is_floating_point<number_type>::value && !is_unsigned &&
                                           is_unsigned && std::is_same<T, signed_number_type>::value>::type* = nullptr )const {
         return static_cast<signed_number_type>(quantity);
      }

      template<class T>
      inline T to( typename std::enable_if<!std::is_floating_point<number_type>::value &&
                                           !is_unsigned && std::is_same<T, unsigned_number_type>::value>::type* = nullptr )const {
         eosio_assert( quantity >= NumberType(), "casting negative integer to unsigned integer" );
         return static_cast<unsigned_number_type>(std::abs(quantity));
      }

   public:
      static_assert( ( lower_limit < NumberType() && NumberType() < upper_limit ) ||
                     ( is_unsigned && lower_limit == NumberType() ),
                     "default constructor for NumberType creates value that is outside the allowed range" );

      safe_quantity() : quantity() {}

      explicit safe_quantity(const NumberType& v) : quantity(v) {
         eosio_assert( is_valid(), "safe_quantity constructed with value outside allowed range" );
      }

      template<class T>
      explicit operator T()const { return to<T>(); }

      constexpr bool is_valid()const {
         return ( lower_limit < quantity && quantity < upper_limit ) || ( is_unsigned && quantity == NumberType() );
      }

      NumberType get_quantity()const { return quantity; }
      void       set_quantity(const NumberType& v) {
         quantity = v;
         eosio_assert( is_valid(), "safe_quantity::set_quantity called with value outside allowed range" );
      }

      /*
      safe_quantity& operator=(const NumberType& v) {
         set_quantity(v);
         return *this;
      }
      */

      explicit operator bool()const { return quantity != NumberType(); }

      inline friend bool operator <=( const safe_quantity& lhs, const safe_quantity& rhs ) { return lhs.quantity <= rhs.quantity; }
      inline friend bool operator < ( const safe_quantity& lhs, const safe_quantity& rhs ) { return lhs.quantity <  rhs.quantity; }
      inline friend bool operator >=( const safe_quantity& lhs, const safe_quantity& rhs ) { return lhs.quantity >= rhs.quantity; }
      inline friend bool operator > ( const safe_quantity& lhs, const safe_quantity& rhs ) { return lhs.quantity >  rhs.quantity; }
      inline friend bool operator ==( const safe_quantity& lhs, const safe_quantity& rhs ) { return lhs.quantity == rhs.quantity; }
      inline friend bool operator !=( const safe_quantity& lhs, const safe_quantity& rhs ) { return lhs.quantity != rhs.quantity; }

      inline friend bool operator <=( const NumberType& lhs, const safe_quantity& rhs ) { return lhs <= rhs.quantity; }
      inline friend bool operator >=( const NumberType& lhs, const safe_quantity& rhs ) { return lhs >= rhs.quantity; }
      inline friend bool operator > ( const NumberType& lhs, const safe_quantity& rhs ) { return lhs >  rhs.quantity; }
      inline friend bool operator ==( const NumberType& lhs, const safe_quantity& rhs ) { return lhs == rhs.quantity; }
      inline friend bool operator !=( const NumberType& lhs, const safe_quantity& rhs ) { return lhs != rhs.quantity; }
      inline friend bool operator < ( const NumberType& lhs, const safe_quantity& rhs ) { return lhs <  rhs.quantity; }

      inline friend bool operator <=( const safe_quantity& lhs, const NumberType& rhs ) { return lhs.quantity <= rhs; }
      inline friend bool operator < ( const safe_quantity& lhs, const NumberType& rhs ) { return lhs.quantity <  rhs; }
      inline friend bool operator >=( const safe_quantity& lhs, const NumberType& rhs ) { return lhs.quantity >= rhs; }
      inline friend bool operator > ( const safe_quantity& lhs, const NumberType& rhs ) { return lhs.quantity >  rhs; }
      inline friend bool operator ==( const safe_quantity& lhs, const NumberType& rhs ) { return lhs.quantity == rhs; }
      inline friend bool operator !=( const safe_quantity& lhs, const NumberType& rhs ) { return lhs.quantity != rhs; }

      safe_quantity operator-()const {
         eosio_assert( !is_unsigned, "cannot negate unsigned integer" ); // TODO: make it a compile time error to call negative on unsigned types
         safe_quantity q = *this;
         q.quantity = -q.quantity;
         return q;
      }

      safe_quantity& operator +=( const safe_quantity& q ) {
         quantity += q.quantity;
         eosio_assert( is_unsigned || (lower_limit < quantity), "underflow" );
         eosio_assert( quantity < upper_limit, "overflow" );
         return *this;
      }

      safe_quantity& operator -=( const safe_quantity& q ) {
         eosio_assert( !is_unsigned || quantity >= q.quantity, "underflow" );
         quantity -= q.quantity;
         eosio_assert( is_unsigned || lower_limit < quantity, "underflow" );
         eosio_assert( quantity < upper_limit, "overflow" );
         return *this;
      }

      inline friend safe_quantity operator +( const safe_quantity& lhs, const safe_quantity& rhs ) {
         safe_quantity sum = lhs;
         sum += rhs;
         return sum;
      }

      inline friend safe_quantity operator -( const safe_quantity& lhs, const safe_quantity& rhs ) {
         safe_quantity difference = lhs;
         difference -= rhs;
         return difference;
      }

      template<typename DataStream>
      friend DataStream& operator << ( DataStream& ds, const safe_quantity& q ){
         return ds << q.quantity;
      }

      template<typename DataStream>
      friend DataStream& operator >> ( DataStream& ds, safe_quantity& q ){
         ds >> q.quantity;
         eosio_assert( q.is_valid(), "deserialized safe_quantity has value outside allowed range" );
         return ds;
      }

   };
#endif

   template<typename NumberType = int64_t, uint128_t Unit = 0>
   safe_quantity<NumberType, Unit> make_safe( const NumberType& v ) {
      return safe_quantity<NumberType, Unit>{v};
   }

} /// namespace eosio
