#pragma once
#include <eoslib/math.hpp>
#include <eoslib/print.hpp>

namespace eosio {
    /**
    *  @defgroup real data type representation for eos
    *  @brief real data type with basic operators. Contract developers need not use the math APIs
    *  @ingroup mathapi
    *
    * Example:
    * @code
    * real a(100);
    * real b(10);
    * real c = a+b
    * real d = a / b
    * real e = a*b
    * if(a == b) {}
    * if(a > b) {}
    * if(a < b) {}
    * auto val = d.value();
    */
    class real {
    private:
        uint64_t val;
    public:
        /**
        * Constructor to double object from uint64 value
        * @param val    data
        */
        real(const uint64_t &_val) : val(_val) {}

        uint64_t value() const { return val; }

        // Arithmetic operations
        real operator+(const real &rhs) const;

        real operator*(const real &rhs) const;

        real operator/(const real &rhs) const;

        // Comparison operators
        friend bool operator==(const real &c1, const real &c2);

        friend bool operator>(const real &c1, const real &c2);

        friend bool operator<(const real &c1, const real &c2);

    };

    /**
     * Add two real variables
     * @param rhs    double variable to be added with this
     * @return the sum of this and rhs
     */
    real real::operator+(const real &rhs) const {
        auto _result = double_add(value(), rhs.value());
        return real(_result);
    }

    /**
     * Multiply two real variables
     * @param rhs    double variable to be multiplied with this
     * @return the result after multiplication
     */
    real real::operator*(const real &rhs) const {
        auto _result = double_mult(value(), rhs.value());
        return real(_result);
    }

    /**
     * Division between two real varialbles
     * @param rhs    double variable to be multiplied with this
     * @return the result after division
     */
    real real::operator/(const real &rhs) const {
        auto _result = double_div(i64_to_double(value()), i64_to_double(rhs.value()));
        return real(_result);
    }

    /**
     * Compares two double variables c1 and c2
     * @param c1
     * @param c2
     * @return if c1 == c2, return true, otherwise false
     */
    bool operator==(const real &c1, const real &c2) {
        auto res = double_eq(c1.value(), c2.value());
        return (res == 1);
    }

    /**
     * Compares two double variables c1 and c2
     * @param c1
     * @param c2
     * @return if c1 > c2, return true, otherwise false
     */
    bool operator>(const real &c1, const real &c2) {
        auto res = double_gt(c1.value(), c2.value());
        return (res == 1);
    }

    /**
     * Compares two double variables c1 and c2
     * @param c1
     * @param c2
     * @return if c1 < c2, return true, otherwise false
     */
    bool operator<(const real &c1, const real &c2) {
        auto res = double_lt(c1.value(), c2.value());
        return (res == 1);
    }
}
