/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <array>
#include <algorithm>
#include <type_traits>

#include <eosiolib/system.h>

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
    *  @defgroup fixed_key Fixed Size Key
    *  @brief Fixed size key sorted lexicographically for Multi Index Table
    *  @ingroup types
    *  @{
    */

   /**
    *  Fixed size key sorted lexicographically for Multi Index Table
    * 
    *  @brief Fixed size key sorted lexicographically for Multi Index Table
    *  @tparam Size - Size of the fixed_key object
    *  @ingroup types
    */
   template<size_t Size>
   class fixed_key {
      private:

         template<bool...> struct bool_pack;
         template<bool... bs>
         using all_true = std::is_same< bool_pack<bs..., true>, bool_pack<true, bs...> >;

         template<typename Word, size_t NumWords>
         static void set_from_word_sequence(const std::array<Word, NumWords>& arr, fixed_key<Size>& key)
         {
            auto itr = key._data.begin();
            word_t temp_word = 0;
            const size_t sub_word_shift = 8 * sizeof(Word);
            const size_t num_sub_words = sizeof(word_t) / sizeof(Word);
            auto sub_words_left = num_sub_words;
            for( auto&& w : arr ) {
               if( sub_words_left > 1 ) {
                   temp_word |= static_cast<word_t>(w);
                   temp_word <<= sub_word_shift;
                   --sub_words_left;
                   continue;
               }

               eosio_assert( sub_words_left == 1, "unexpected error in fixed_key constructor" );
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

      public:

         typedef uint128_t word_t;
         
         /**
          * Get number of words contained in this fixed_key object. A word is defined to be 16 bytes in size
          * 
          * @brief Get number of words contained in this fixed_key object
          */

         static constexpr size_t num_words() { return (Size + sizeof(word_t) - 1) / sizeof(word_t); }

         /**
          * Get number of padded bytes contained in this fixed_key object. Padded bytes are the remaining bytes
          * inside the fixed_key object after all the words are allocated
          * 
          * @brief Get number of padded bytes contained in this fixed_key object
          */
         static constexpr size_t padded_bytes() { return num_words() * sizeof(word_t) - Size; }

         /**
         * @brief Default constructor to fixed_key object
         *
         * @details Default constructor to fixed_key object which initializes all bytes to zero
         */
         constexpr fixed_key() : _data() {}

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

         /**
         * @brief Constructor to fixed_key object from std::array of num_words() words
         *
         * @details Constructor to fixed_key object from std::array of num_words() words
         * @param arr - Source data
         */
         template<typename Word, size_t NumWords,
                  typename Enable = typename std::enable_if<std::is_integral<Word>::value &&
                                                             !std::is_same<Word, bool>::value &&
                                                             sizeof(Word) < sizeof(word_t)>::type >
         fixed_key(const std::array<Word, NumWords>& arr)
         {
            static_assert( sizeof(word_t) == (sizeof(word_t)/sizeof(Word)) * sizeof(Word),
                           "size of the backing word size is not divisible by the size of the array element" );
            static_assert( sizeof(Word) * NumWords <= Size, "too many words supplied to fixed_key constructor" );

            set_from_word_sequence(arr, *this);
         }

         /**
         * @brief Create a new fixed_key object from a sequence of words
         *
         * @details Create a new fixed_key object from a sequence of words
         * @tparam FirstWord - The type of the first word in the sequence
         * @tparam Rest - THe type of the remaining words in the sequence
         * @param first_word - The first word in the sequence
         * @param rest - The remaining words in the sequence
         */
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
                           "size of the backing word size is not divisible by the size of the words supplied as arguments" );
            static_assert( sizeof(FirstWord) * (1 + sizeof...(Rest)) <= Size, "too many words supplied to make_from_word_sequence" );

            fixed_key<Size> key;
            set_from_word_sequence(std::array<FirstWord, 1+sizeof...(Rest)>{{ first_word, rest... }}, key);
            return key;
         }

         /**
          * Get the contained std::array
          * @brief Get the contained std::array
          */
         const auto& get_array()const { return _data; }

         /**
          * Get the underlying data of the contained std::array
          * @brief Get the underlying data of the contained std::array
          */
         auto data() { return _data.data(); }

         /**
          * Get the underlying data of the contained std::array
          * @brief Get the underlying data of the contained std::array
          */
         auto data()const { return _data.data(); }

         /**
          * Get the size of the contained std::array
          * @brief Get the size of the contained std::array
          */
         auto size()const { return _data.size(); }


         /**
          * Extract the contained data as an array of bytes
          * @brief Extract the contained data as an array of bytes
          * @return - the extracted data as array of bytes
          */
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
    * @param c1 - First fixed_key object to compare
    * @param c2 - Second fixed_key object to compare
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
    * @param c1 - First fixed_key object to compare
    * @param c2 - Second fixed_key object to compare
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
    * @param c1 - First fixed_key object to compare
    * @param c2 - Second fixed_key object to compare
    * @return if c1 > c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator>(const fixed_key<Size>& c1, const fixed_key<Size>& c2) {
      return c1._data > c2._data;
   }

   /**
    * @brief Compares two fixed_key variables c1 and c2
    *
    * @details Lexicographically compares two fixed_key variables c1 and c2
    * @param c1 - First fixed_key object to compare
    * @param c2 - Second fixed_key object to compare
    * @return if c1 < c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator<(const fixed_key<Size> &c1, const fixed_key<Size> &c2) {
      return c1._data < c2._data;
   }
   /// @} fixed_key

   typedef fixed_key<32> key256;
}
