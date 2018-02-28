/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <array>
#include <algorithm>
#include <byteswap.h>

namespace eosio {

    /**
    *  @defgroup key256 256-bit key sorted lexicographically
    *  @ingroup types
    * @{
    */
   class key256 {
      public:

         static constexpr size_t arr_size = 32;

         /**
         * @brief Default constructor to key256 object
         *
         * @details Default constructor to key256 object which initializes all bytes to zero
         */
         key256() : _data() {}

        /**
        * @brief Constructor to key256 object from array of 32 bytes
        *
        * @details Constructor to key256 object from array of 32 bytes
        * @param arr    data
        */
        key256(const uint8_t (&arr)[arr_size])
        {
           std::copy(arr, arr + arr_size, _data.begin());
        }

        /**
        * @brief Constructor to key256 object from four uint64_t numbers
        *
        * @details Constructor to key256 object from four uint64_t numbers
        * @param w1    first 64-bit word in the 256 bit key
        * @param w2    second 64-bit word in the 256 bit key
        * @param w3    third 64-bit word in the 256 bit key
        * @param w4    fourth 64-bit word in the 256 bit key
        */
        key256(uint64_t w1, uint64_t w2, uint64_t w3, uint64_t w4)
        {
           // Assumes little endian machine
           uint64_t* ptr = reinterpret_cast<uint64_t*>(_data.data());
           *ptr = bswap_64(w1);
           ++ptr;
           *ptr = bswap_64(w2);
           ++ptr;
           *ptr = bswap_64(w3);
           ++ptr;
           *ptr = bswap_64(w4);
        }

        auto data() { return _data.data(); }
        auto data()const { return _data.data(); }

        auto size()const { return _data.size(); }

        // Comparison operators
        friend bool operator==(const key256 &c1, const key256 &c2);

        friend bool operator!=(const key256 &c1, const key256 &c2);

        friend bool operator>(const key256 &c1, const key256 &c2);

        friend bool operator<(const key256 &c1, const key256 &c2);

      private:

         std::array<uint8_t, arr_size> _data;
    };

    /**
     * @brief Compares two key256 variables c1 and c2
     *
     * @details Lexicographically compares two key256 variables c1 and c2
     * @return if c1 == c2, return true, otherwise false
     */
    bool operator==(const key256 &c1, const key256 &c2) {
        return c1._data == c2._data;
    }

    /**
     * @brief Compares two key256 variables c1 and c2
     *
     * @details Lexicographically compares two key256 variables c1 and c2
     * @return if c1 != c2, return true, otherwise false
     */
    bool operator!=(const key256 &c1, const key256 &c2) {
        return c1._data != c2._data;
    }

    /**
     * @brief Compares two key256 variables c1 and c2
     *
     * @details Lexicographically compares two key256 variables c1 and c2
     * @return if c1 > c2, return true, otherwise false
     */
    bool operator>(const key256 &c1, const key256 &c2) {
        return c1._data > c2._data;
    }

    /**
     * @brief Compares two key256 variables c1 and c2
     *
     * @details Lexicographically compares two key256 variables c1 and c2
     * @return if c1 < c2, return true, otherwise false
     */
    bool operator<(const key256 &c1, const key256 &c2) {
        return c1._data < c2._data;
    }
   /// @} key256
}
