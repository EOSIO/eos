#pragma once
#include <eoslib/types.h>

namespace eos 
{
    /**
    * @defgroup fixed_point Template classes for Fixed Point representaton
    * @ingroup contractdev
    * @brief 32,64,128,256 bits version of Fixed Point variables
    *
    * Floating point operations are indeterministic, hence is prevented in smart contract.
    * The smart contract developers should use the appropriate Fixed_Point template class 
    * by passing the number to be represented in integer format and the number of decimals 
    * required.
    * These template classes also support the arithmetic operations and basic comparison operators
    * 
    */

    // Some forward declarations
    template <uint32_t q> struct fixed_point32;
    template <uint32_t q> struct fixed_point64;
    template <uint32_t q> struct fixed_point128;

    // Will support fixed_point256 in next release    
#if 0
    template <uint32_t q> struct fixed_point256;
    template <uint32_t q> 
    struct fixed_point256
    {
        int128_t val;
        fixed_point256(int256_t v=0) : val(v) {}    
        template <uint32_t qr> fixed_point256(const fixed_point256<qr> &r);
        template <uint32_t qr> fixed_point256(const fixed_point128<qr> &r);
        template <uint32_t qr> fixed_point256(const fixed_point64<qr> &r);
        template <uint32_t qr> fixed_point256(const fixed_point32<qr> &r);
        /**
        * Get the integer part of the 64 bit fixed number
        * @brief To get the integer part of the fixed number
        * @return Returns integer part of the fixed number
        *
        * Example:
        * @code
        * fixed64<18> a(1234.455667)
        * std::cout << a.int_part(); // Output: 1234
        * @endcode
        */
        int128_t int_part() const {
            return val >> q;
        }

        /**
        * Get the decimal part of the 64 bit fixed number
        * @brief To get the decimal part of the fixed number
        * @return Returns decimal part of the fixed number
        *
        * Example:
        * @code
        * fixed64<18> a(1234.455667)
        * std::cout << a.decimal_part(); // Output: 45567
        * @endcode
        */
        uint128_t frac_part() const {
            if(!q) return 0;
            return val << (32-q);
        }

        

        template <uint32_t qr> fixed_point256 &operator=(const fixed_point32<qr> &r);
        template <uint32_t qr> fixed_point256 &operator=(const fixed_point64<qr> &r);
        template <uint32_t qr> fixed_point256 &operator=(const fixed_point128<qr> &r);
        template <uint32_t qr> fixed_point256 &operator=(const fixed_point256<qr> &r);
        // Comparison functions
        template <uint32_t qr> bool operator==(const fixed_point256<qr> &r) { return (val == r.val);}
        template <uint32_t qr> bool operator>(const fixed_point256<qr> &r) { return (val > r.val);}
        template <uint32_t qr> bool operator<(const fixed_point256<qr> &r) { return (val < r.val);}
    };
#endif    

    /**
    * 128 bits representation of Fixed Point class
    *
    * Example:
    * @code
    * fixed_point128<6> a(123232.455667233)
    * fixed_point128<0> a(123424)
    * fixed_point128<18> c = a*b;
    * fixed_point128<24> d = a+b+c;
    * fixed_point128<24> e = b/a;
    * @endcode
    */
    template <uint32_t q> 
    struct fixed_point128
    {
        int128_t val;
        /**
        * Various constructors for fixed_point128
        * @brief Can create fixed_point128 instance from an int128_t, fixed_point128,64,32 instance
        * 
        *
        * Example:
        * @code
        * fixed_point64<18> a(1234.455667);
        * fixed_point128<3> b(a);
        * fixed_point32<6> b(13324.32323);
        * fixed_point128<5> c(a);
        * @endcode
        */
        fixed_point128(int128_t v=0) : val(v) {}    
        template <uint32_t qr> fixed_point128(const fixed_point128<qr> &r);
        template <uint32_t qr> fixed_point128(const fixed_point64<qr> &r);
        template <uint32_t qr> fixed_point128(const fixed_point32<qr> &r);
        /**
        * Get the integer part of the 64 bit fixed number
        * @brief To get the integer part of the fixed number
        * @return Returns integer part of the fixed number
        *
        * Example:
        * @code
        * fixed_point64<5> a(1234.455667)
        * std::cout << a.int_part(); // Output: 1234
        * @endcode
        */
        int128_t int_part() const {
            return val >> q;
        }

        /**
        * Get the decimal part of the 64 bit fixed number
        * @brief To get the decimal part of the fixed number
        * @return Returns decimal part of the fixed number
        *
        * Example:
        * @code
        * fixed_point128<3> a(1234.455667)
        * std::cout << a.decimal_part(); // Output: 455
        * @endcode
        */
        uint128_t frac_part() const {
            if(!q) return 0;
            return val << (32-q);
        }

        
        // Various assignment operators
        template <uint32_t qr> fixed_point128 &operator=(const fixed_point32<qr> &r);
        template <uint32_t qr> fixed_point128 &operator=(const fixed_point64<qr> &r);
        template <uint32_t qr> fixed_point128 &operator=(const fixed_point128<qr> &r);
        // Comparison functions
        template <uint32_t qr> bool operator==(const fixed_point128<qr> &r) { return (val == r.val);}
        template <uint32_t qr> bool operator>(const fixed_point128<qr> &r) { return (val > r.val);}
        template <uint32_t qr> bool operator<(const fixed_point128<qr> &r) { return (val < r.val);}
    };

    /**
    * 64 bits representation of Fixed Point class
    *
    * Example:
    * @code
    * fixed_point64<6> a(123232.455667233)
    * fixed_point64<0> a(123424)
    * fixed_point64<18> c = a*b;
    * fixed_point64<24> d = a+b+c;
    * fixed_point64<24> e = b/a;
    * @endcode
    */
    template <uint32_t q> 
    struct fixed_point64 
    {
        int64_t val;
        fixed_point64(int64_t v=0) : val(v) {}    
        /**
        * Various constructors for fixed_point64
        * @brief Can create fixed_point64 instance from int64_t, fixed_point64,32 instances
        * 
        *
        * Example:
        * @code
        * fixed_point32<18> a(1234.455667);
        * fixed_point64<3> b(a);
        * fixed_point64<6> b(13324.32323);
        * fixed_point64<5> c(a);
        * @endcode
        */
        template <uint32_t qr> fixed_point64(const fixed_point64<qr> &r);
        template <uint32_t qr> fixed_point64(const fixed_point32<qr> &r);
        /**
        * Get the integer part of the 64 bit fixed number
        * @brief To get the integer part of the fixed number
        * @return Returns integer part of the fixed number
        *
        * Example:
        * @code
        * fixed_point64<18> a(1234.455667)
        * std::cout << a.int_part(); // Output: 1234
        * @endcode
        */
        int64_t int_part() const {
            return val >> q;
        }

        /**
        * Get the decimal part of the 64 bit fixed number
        * @brief To get the decimal part of the fixed number
        * @return Returns decimal part of the fixed number
        *
        * Example:
        * @code
        * fixed64<3> a(1234.455667)
        * std::cout << a.decimal_part(); // Output: 455
        * @endcode
        */
        uint64_t frac_part() const {
            if(!q) return 0;
            return val << (32-q);
        }

        // Various assignment operators
        template <uint32_t qr> fixed_point64 &operator=(const fixed_point32<qr> &r);
        template <uint32_t qr> fixed_point64 &operator=(const fixed_point64<qr> &r);
        
        // Arithmetic operations 
        template <uint32_t qr> fixed_point64< (q>qr)?q:qr > operator+(const fixed_point64<qr> &r) const;
        template <uint32_t qr> fixed_point64< (q>qr)?q:qr > operator-(const fixed_point64<qr> &r) const;
        // product and division of two fixed_point64 instances will be fixed_point128
        // The total number of decimals will be the max 
        template <uint32_t qr> fixed_point128<q+qr> operator*(const fixed_point64<qr> &r) const;
        template <uint32_t qr> fixed_point128<q+64-qr> operator/(const fixed_point64<qr> &r) const;
        // Comparison functions
        template <uint32_t qr> bool operator==(const fixed_point64<qr> &r) { return (val == r.val);}
        template <uint32_t qr> bool operator>(const fixed_point64<qr> &r) { return (val > r.val);}
        template <uint32_t qr> bool operator<(const fixed_point64<qr> &r) { return (val < r.val);}
    };

    /**
     *  @defgroup fixed point Fixed Point generic class
     *  @brief Implementation of fixed point template class
     *  @ingroup contractdev
     *
     *  @{
     */

    /**
     * This class is implemented to to replace the floating point variables
     * It can resolve floating point undetermenistic related issues
     *
     * Example:
     * @code
     * 
     * fixed_point32<17> b(9.654);
     * fixed_point32<18> c = a*b;
     * fixed_point32<24> d = a+b+c;
     * fixed_point32<24> e = b/a;
     * @endcode
     */
    // fixed_point 32 bit version. The template param 'q' is the scale factor 
    template <uint32_t q> 
    struct fixed_point32
    {
        // trsnslates given double variable to the int32 based on the scale factor
        int32_t val;
        template <uint32_t qr> fixed_point32(const fixed_point32<qr> &r);
        template <uint32_t qr> fixed_point32(const fixed_point64<qr> &r);

        fixed_point32(int32_t param=0) : val(param) {}
        // fixed_point32(double d=0) : val(d * (1<<q) ) { }
        /*
        double to_double() const {
            return double(val) / (1<<q);
        }
        */

        /**
        * Get the integer part of the 64 bit fixed number
        * @brief To get the integer part of the fixed number
        * @return Returns integer part of the fixed number
        *
        * Example:
        * @code
        * fixed_point32<18> a(1234.455667)
        * std::cout << a.int_part(); // Output: 1234
        * @endcode
        */
        int32_t int_part() const {
            return val >> q;
        }
        uint32_t frac_part() const {
            if(!q) return 0;
            return val << (32-q);
        }

        // Various assignment operators
        template <uint32_t qr> fixed_point32 &operator=(const fixed_point32<qr> &r);
        template <uint32_t qr> fixed_point32 &operator=(const fixed_point64<qr> &r);
        template <uint32_t qr> fixed_point32< (q>qr)?q:qr > operator+(const fixed_point32<qr> &r) const;
        template <uint32_t qr> fixed_point32< (q>qr)?q:qr > operator-(const fixed_point32<qr> &r) const;
        // productd of to fixed_point32 instances will be fixed_point64
        template <uint32_t qr> fixed_point64<q+qr> operator*(const fixed_point32<qr> &r) const;
        template <uint32_t qr> fixed_point64<q+32-qr> operator/(const fixed_point32<qr> &r) const;
        // Comparison functions
        template <uint32_t qr> bool operator==(const fixed_point32<qr> &r) { return (val == r.val);}
        template <uint32_t qr> bool operator>(const fixed_point32<qr> &r) { return (val > r.val);}
        template <uint32_t qr> bool operator<(const fixed_point32<qr> &r) { return (val < r.val);}
    };

    // Helper functions
    template<typename T>
    T assignHelper(T rhs_val, uint32_t q, uint32_t qr)
    {
        T result = (q > qr) ? rhs_val << (q-qr) : rhs_val >> (qr-q);
        return result;
    }

#if 0    
    // fixed_point256 methods
    template<uint32_t q> template<uint32_t qr>
    fixed_point256<q>::fixed_point256(const fixed_point256<qr> &r) {
        val = assignHelper<int256_t>(r.val, q, qr);
    }

    template<uint32_t q> template<uint32_t qr>
    fixed_point256<q>::fixed_point256(const fixed_point128<qr> &r) {
        val = assignHelper<int256_t>(r.val, q, qr);
    }

    template<uint32_t q> template<uint32_t qr>
    fixed_point256<q>::fixed_point256(const fixed_point64<qr> &r) {
        val = assignHelper<int256_t>(r.val, q, qr);
    }

    template<uint32_t q> template <uint32_t qr>
    fixed_point256<q>::fixed_point256(const fixed_point32<qr> &r) {
        val = assignHelper<int256_t>(r.val, q, qr);
    }
#endif    

    // fixed_point128 methods
    template<uint32_t q> template<uint32_t qr>
    fixed_point128<q>::fixed_point128(const fixed_point128<qr> &r) {
        val = assignHelper<int128_t>(r.val, q, qr);
    }

    template<uint32_t q> template<uint32_t qr>
    fixed_point128<q>::fixed_point128(const fixed_point64<qr> &r) {
        val = assignHelper<int128_t>(r.val, q, qr);
    }

    template<uint32_t q> template <uint32_t qr>
    fixed_point128<q>::fixed_point128(const fixed_point32<qr> &r) {
        val = assignHelper<int128_t>(r.val, q, qr);
    }


    // fixed_point64 methods
    template<uint32_t q> template<uint32_t qr>
    fixed_point64<q>::fixed_point64(const fixed_point64<qr> &r) {
        val = assignHelper<int64_t>(r.val, q, qr);
    }

    template<uint32_t q> template <uint32_t qr>
    fixed_point64<q>::fixed_point64(const fixed_point32<qr> &r) {
        val = assignHelper<int32_t>(r.val, q, qr);
    }

    /**
    * Addition between two fixed_point64 variables
    * @brief Addition between two fixed_point64 variables and the result goes to fixed_point64
    * Number of decimal on result will be max of decimals of lhs and rhs
    *
    */
    template<uint32_t q> template<uint32_t qr>
    fixed_point64< (q>qr)?q:qr > fixed_point64<q>::operator+(const fixed_point64<qr> &rhs) const 
    {
        // if the scaling factor for both are same, no need to make any intermediate objects except the result
        if(q == qr)
        {
            return fixed_point64<q>(val + rhs.val);
        }
        return fixed_point64<(q>qr)?q:qr>(
            fixed_point64<(q>qr)?q:qr>( *this ).val +
            fixed_point64<(q>qr)?q:qr>( rhs ).val
        );
    }

    /**
    * Subtraction between two fixed_point64 variables
    * @brief Subtraction between two fixed_point64 variables and the result goes to fixed_point64
    * Number of decimal on result will be max of decimals of lhs and rhs
    *
    */
    template<uint32_t q> template<uint32_t qr>
    fixed_point64< (q>qr)?q:qr > fixed_point64<q>::operator-(const fixed_point64<qr> &rhs) const 
    {
        // if the scaling factor for both are same, no need to make any intermediate objects except the result
        if(q == qr)
        {
            return fixed_point64<q>(val - rhs.val);
        }
        return fixed_point64<(q>qr)?q:qr>(
            fixed_point64<(q>qr)?q:qr>( *this ).val -
            fixed_point64<(q>qr)?q:qr>( rhs ).val
        );
    }

    /**
    * Multiplication operator for fixed_point64. The result goes to fixed_point128
    * @brief Multiplication operator for fixed_point64. The result goes to fixed_point64
    * Number of decimal on result will be sum of number of decimals of lhs and rhs
    *
    * Example:
    * @code
    * fixed_point128<33> result = fixed_point64<0>(131313) / fixed_point64<0>(2323)
    * @endcode
    */
    template<uint32_t q> template <uint32_t qr>
    fixed_point128<q+qr> fixed_point64<q>::operator*(const fixed_point64<qr> &r) const {
        return fixed_point128<q+qr>(int128_t(val)*r.val);
    }

    /**
    * Division operator for fixed_point64
    * @brief Division of two fixed_point64 result will be stored in fixed_point128
    * 
    *
    * Example:
    * @code
    * fixed_point128<33> result = fixed_point64<0>(131313) / fixed_point64<0>(2323)
    * @endcode
    */
    template <uint32_t q> template <uint32_t qr>
    fixed_point128<q+64-qr> fixed_point64<q>::operator/(const fixed_point64<qr> &r) const {
        // std::cout << "Performing division on " << val << ", with " << q << " precision / " << r.val << ", with " << qr << " precision. Result precision " << ((q>qr) ? q:qr) << std::endl;
        // Convert val to 128 bit by additionally shifting 64 bit and take the result to 128bit
        // Q(X+64-Y) = Q(X+64) / Q(Y)
        return fixed_point128<q+64-qr>((int128_t(val)<<64)/r.val);
    }

    // fixed_point32 methods
    template<uint32_t q> template <uint32_t qr>
    fixed_point32<q>::fixed_point32(const fixed_point32<qr> &r) {
        val = assignHelper<int32_t, uint32_t>(r.val, q, qr);
    }

    template<uint32_t q> template <uint32_t qr>
    fixed_point32<q>::fixed_point32(const fixed_point64<qr> &r) {
        val = assignHelper<int32_t, uint32_t>(r.val, q, qr);
    }

    template<uint32_t q> template <uint32_t qr>
    fixed_point32<q> &fixed_point32<q>::operator=(const fixed_point32<qr> &r) {
        val = assignHelper<int32_t, uint32_t>(r.val, q, qr);
    }

    template<uint32_t q> template <uint32_t qr>
    fixed_point32<q> &fixed_point32<q>::operator=(const fixed_point64<qr> &r) {
        val = assignHelper<int32_t, uint32_t>(r.val, q, qr);
    }

    /**
    * Addition between two fixed_point32 variables
    * @brief Addition between two fixed_point32 variables and the result goes to fixed_point32
    * Number of decimal on result will be max of decimals of lhs and rhs
    *
    */
    template<uint32_t q> template<uint32_t qr>
    fixed_point32< (q>qr)?q:qr > fixed_point32<q>::operator+(const fixed_point32<qr> &rhs) const 
    {
        // if the scaling factor for both are same, no need to make any intermediate objects except the result
        if(q == qr)
        {
            return fixed_point32<q>(val + rhs.val);
        }
        return fixed_point32<(q>qr)?q:qr>(
            fixed_point32<(q>qr)?q:qr>( *this ).val +
            fixed_point32<(q>qr)?q:qr>( rhs ).val
        );
    }

    /**
    * Subtraction between two fixed_point32 variables
    * @brief Subtraction between two fixed_point32 variables and the result goes to fixed_point32
    * Number of decimal on result will be max of decimals of lhs and rhs
    *
    */
    template<uint32_t q> template<uint32_t qr>
    fixed_point32< (q>qr)?q:qr > fixed_point32<q>::operator-(const fixed_point32<qr> &rhs) const 
    {
        // if the scaling factor for both are same, no need to make any intermediate objects except the result
        if(q == qr)
        {
            return fixed_point32<q>(val - rhs.val);
        }
        return fixed_point32<(q>qr)?q:qr>(
            fixed_point32<(q>qr)?q:qr>( *this ).val -
            fixed_point32<(q>qr)?q:qr>( rhs ).val
        );
    }

    /**
    * Multiplication operator for fixed_point32. The result goes to fixed_point64
    * @brief Multiplication operator for fixed_point32. The result goes to fixed_point64
    * Number of decimal on result will be sum of number of decimals of lhs and rhs
    *
    * Example:
    * @code
    * fixed_point64<33> result = fixed_point32<0>(131313) / fixed_point32<0>(2323)
    * @endcode
    */
    template<uint32_t q> template <uint32_t qr>
    fixed_point64<q+qr> fixed_point32<q>::operator*(const fixed_point32<qr> &r) const {
        return fixed_point64<q+qr>(int64_t(val)*r.val);
    }

    /**
    * Division operator for fixed_point32
    * @brief Division of two fixed_point32 result will be stored in fixed_point64
    * 
    *
    * Example:
    * @code
    * fixed_point64<33> result = fixed_point32<0>(131313) / fixed_point32<0>(2323)
    * @endcode
    */
    template <uint32_t q> template <uint32_t qr>
    fixed_point64<q+32-qr> fixed_point32<q>::operator/(const fixed_point32<qr> &r) const {
        // Convert val into 64 bit and perform the division
        // Q(X+32-Y) = Q(X+32) / Q(Y)
        return fixed_point64<q+32-qr>((int64_t(val)<<32)/r.val);
    }

    /**
    * Wrapper function for dividing two unit32 variable and stores result in fixed_point64
    * @brief Wrapper function for dividing two unit64 variable and stores result in fixed_point64
    * 
    *
    * Example:
    * @code
    * fixed_point64<33> result = fixed_divide(131313, 2323)
    * @endcode
    */
    template <uint32_t q> 
    fixed_point64<q> fixed_divide(uint32_t lhs, uint32_t rhs)
    {
        fixed_point64<q> result = fixed_point32<0>(lhs) / fixed_point32<0>(rhs);
        return result;
    }

    /**
    * Wrapper function for dividing two unit64 variable and stores result in fixed_point128
    * @brief Wrapper function for dividing two unit64 variable and stores result in fixed_point128
    * 
    *
    * Example:
    * @code
    * fixed_point128<33> result = fixed_divide(131313, 2323)
    * @endcode
    */
        
    template <uint32_t q> 
    fixed_point128<q> fixed_divide(uint64_t lhs, uint64_t rhs)
    {
        fixed_point128<q> result = fixed_point64<0>(lhs) / fixed_point64<0>(rhs);
        return fixed_point128<q>(result);
    }
};
