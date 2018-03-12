/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <array>
#include <algorithm>
#include <type_traits>

#include <fc/exception/exception.hpp>

namespace eosio {

   template<size_t Size>
   class fixed_key;

   template<size_t Size>
   bool operator==(const fixed_key<Size> &c1, const fixed_key<Size> &c2);

   template<size_t Size>
   bool operator!=(const fixed_key<Size> &c1, const fixed_key<Size> &c2);

   template<size_t Size>
   bool operator>(const fixed_key<Size> &c1, const fixed_key<Size> &c2);

   template<size_t Size>
   bool operator<(const fixed_key<Size> &c1, const fixed_key<Size> &c2);

    /**
    *  @defgroup fixed_key fixed size key sorted lexicographically
    *  @ingroup types
    * @{
    */
   template<size_t Size>
   class fixed_key {
      private:

         template<bool...> struct bool_pack;
         template<bool... bs>
         using all_true = std::is_same< bool_pack<bs..., true>, bool_pack<true, bs...> >;

      public:

         typedef uint128_t word_t;

         static constexpr size_t num_words() { return (Size + sizeof(word_t) - 1) / sizeof(word_t); }
         static constexpr size_t padded_bytes() { return num_words() * sizeof(word_t) - Size; }

         /**
         * @brief Default constructor to fixed_key object
         *
         * @details Default constructor to fixed_key object which initializes all bytes to zero
         */
         fixed_key() : _data() {}

         /**
         * @brief Constructor to fixed_key object from array of num_words() words
         *
         * @details Constructor to fixed_key object from array of num_words() words
         * @param arr    data
         */
         fixed_key(const word_t (&arr)[num_words()])
         {
           std::copy(arr, arr + num_words(), _data.begin());
         }

         /**
         * @brief Constructor to fixed_key object from std::array of num_words() words
         *
         * @details Constructor to fixed_key object from std::array of num_words() words
         * @param arr    data
         */
         fixed_key(const std::array<word_t, num_words()>& arr)
         {
           std::copy(arr.begin(), arr.end(), _data.begin());
         }

#if 0
// Cannot infer constructor template arguments in C++ versions prior to C++17
         /**
         * @brief Constructor to fixed_key object from sequence of words supplied as arguments
         *
         * @details Constructor to fixed_key object from sequence of words (of small sizes allowed) supplied as arguments
         */
         template<typename FirstWord, typename... Rest>
         fixed_key(typename std::enable_if<std::is_integral<FirstWord>::value &&
                                           !std::is_same<FirstWord, bool>::value &&
                                           sizeof(FirstWord) <= sizeof(word_t) &&
                                           all_true<(std::is_same<FirstWord, Rest>::value)...>::value,
                                          FirstWord>::type first_word,
                    Rest... rest)
         {
            static_assert( sizeof(word_t) == (sizeof(word_t)/sizeof(FirstWord)) * sizeof(FirstWord),
                           "size of the backing word size is not divisble by the size of the words supplied as arguments" );
            static_assert( sizeof(FirstWord) * (1 + sizeof...(Rest)) <= Size, "too many words supplied to fixed_key constructor" );

            auto itr = _data.begin();
            word_t temp_word = 0;
            const size_t sub_word_shift = 8 * sizeof(FirstWord);
            const size_t num_sub_words = sizeof(word_t) / sizeof(FirstWord);
            auto sub_words_left = num_sub_words;
            for( auto&& w : { first_word, rest... } ) {
               if( sub_words_left > 1 ) {
                   temp_word |= static_cast<word_t>(w);
                   temp_word <<= sub_word_shift;
                   --sub_words_left;
                   continue;
               }

               FC_ASSERT( sub_words_left == 1, "unexpected error in fixed_key constructor" );
               temp_word |= static_cast<word_t>(w);
               sub_words_left = num_sub_words;

               *itr = temp_word;
               temp_word = 0;
               ++itr;
            }
            if( sub_words_left != num_sub_words ) {
               if( sub_words_left > 1 )
                  temp_word <<= 8 * (sub_words_left-1);
               *itr = temp_word;
            }
         }
#endif

         template<typename FirstWord, typename... Rest>
         static
         fixed_key<Size>
         make_from_word_sequence(typename std::enable_if<std::is_integral<FirstWord>::value &&
                                                          !std::is_same<FirstWord, bool>::value &&
                                                          sizeof(FirstWord) <= sizeof(word_t) &&
                                                          all_true<(std::is_same<FirstWord, Rest>::value)...>::value,
                                                         FirstWord>::type first_word,
                                 Rest... rest)
         {
            static_assert( sizeof(word_t) == (sizeof(word_t)/sizeof(FirstWord)) * sizeof(FirstWord),
                           "size of the backing word size is not divisble by the size of the words supplied as arguments" );
            static_assert( sizeof(FirstWord) * (1 + sizeof...(Rest)) <= Size, "too many words supplied to fixed_key constructor" );

            fixed_key<Size> key;

            auto itr = key._data.begin();
            word_t temp_word = 0;
            const size_t sub_word_shift = 8 * sizeof(FirstWord);
            const size_t num_sub_words = sizeof(word_t) / sizeof(FirstWord);
            auto sub_words_left = num_sub_words;
            for( auto&& w : { first_word, rest... } ) {
               if( sub_words_left > 1 ) {
                   temp_word |= static_cast<word_t>(w);
                   temp_word <<= sub_word_shift;
                   --sub_words_left;
                   continue;
               }

               FC_ASSERT( sub_words_left == 1, "unexpected error in fixed_key constructor" );
               temp_word |= static_cast<word_t>(w);
               sub_words_left = num_sub_words;

               *itr = temp_word;
               temp_word = 0;
               ++itr;
            }
            if( sub_words_left != num_sub_words ) {
               if( sub_words_left > 1 )
                  temp_word <<= 8 * (sub_words_left-1);
               *itr = temp_word;
            }

            return key;
         }

         const auto& get_array()const { return _data; }

         auto data() { return _data.data(); }
         auto data()const { return _data.data(); }

         auto size()const { return _data.size(); }

         std::array<uint8_t, Size> extract_as_byte_array()const {
            std::array<uint8_t, Size> arr;

            const size_t num_sub_words = sizeof(word_t);

            auto arr_itr  = arr.begin();
            auto data_itr = _data.begin();

            for( size_t counter = _data.size(); counter > 0; --counter, ++data_itr ) {
               size_t sub_words_left = num_sub_words;

               if( counter == 1 ) { // If last word in _data array...
                  sub_words_left -= padded_bytes();
               }
               auto temp_word = *data_itr;
               for( ; sub_words_left > 0; --sub_words_left ) {
                  *(arr_itr + sub_words_left - 1) = static_cast<uint8_t>(temp_word & 0xFF);
                  temp_word >>= 8;
               }
               arr_itr += num_sub_words;
            }

            return arr;
         }

         // Comparison operators
         friend bool operator== <>(const fixed_key<Size> &c1, const fixed_key<Size> &c2);

         friend bool operator!= <>(const fixed_key<Size> &c1, const fixed_key<Size> &c2);

         friend bool operator> <>(const fixed_key<Size> &c1, const fixed_key<Size> &c2);

         friend bool operator< <>(const fixed_key<Size> &c1, const fixed_key<Size> &c2);

      private:

         std::array<word_t, num_words()> _data;
    };

   /**
    * @brief Compares two fixed_key variables c1 and c2
    *
    * @details Lexicographically compares two fixed_key variables c1 and c2
    * @return if c1 == c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator==(const fixed_key<Size> &c1, const fixed_key<Size> &c2) {
      return c1._data == c2._data;
   }

   /**
    * @brief Compares two fixed_key variables c1 and c2
    *
    * @details Lexicographically compares two fixed_key variables c1 and c2
    * @return if c1 != c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator!=(const fixed_key<Size> &c1, const fixed_key<Size> &c2) {
      return c1._data != c2._data;
   }

   /**
    * @brief Compares two fixed_key variables c1 and c2
    *
    * @details Lexicographically compares two fixed_key variables c1 and c2
    * @return if c1 > c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator>(const fixed_key<Size> &c1, const fixed_key<Size> &c2) {
      return c1._data > c2._data;
   }

   /**
    * @brief Compares two fixed_key variables c1 and c2
    *
    * @details Lexicographically compares two fixed_key variables c1 and c2
    * @return if c1 < c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator<(const fixed_key<Size> &c1, const fixed_key<Size> &c2) {
      return c1._data < c2._data;
   }
   /// @} fixed_key

   typedef fixed_key<32> key256;
}
