/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/chain/types.hpp>

#include <array>
#include <algorithm>
#include <functional>
#include <type_traits>

namespace eosio {

   /// @cond IMPLEMENTATIONS

   using chain::uint128_t;

   template<size_t Size>
   class fixed_bytes;

   template<size_t Size>
   bool operator ==(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   template<size_t Size>
   bool operator !=(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   template<size_t Size>
   bool operator >(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   template<size_t Size>
   bool operator <(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   template<size_t Size>
   bool operator >=(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   template<size_t Size>
   bool operator <=(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

   /// @endcond

    /**
     *  @defgroup fixed_bytes Fixed Size Byte Array
     *  @ingroup core
     *  @ingroup types
     *  @brief Fixed size array of bytes sorted lexicographically
     */

   /**
    *  Fixed size byte array sorted lexicographically
    *
    *  @ingroup fixed_bytes
    *  @tparam Size - Size of the fixed_bytes object
    */
   template<size_t Size>
   class fixed_bytes {
      private:

         template<bool...> struct bool_pack;
         template<bool... bs>
         using all_true = std::is_same< bool_pack<bs..., true>, bool_pack<true, bs...> >;

         template<typename Word, size_t NumWords>
         static void set_from_word_sequence(const Word* arr_begin, const Word* arr_end, fixed_bytes<Size>& key)
         {
            auto itr = key._data.begin();
            word_t temp_word = 0;
            const size_t sub_word_shift = 8 * sizeof(Word);
            const size_t num_sub_words = sizeof(word_t) / sizeof(Word);
            auto sub_words_left = num_sub_words;
            for( auto w_itr = arr_begin; w_itr != arr_end; ++w_itr ) {
               if( sub_words_left > 1 ) {
                   temp_word |= static_cast<word_t>(*w_itr);
                   temp_word <<= sub_word_shift;
                   --sub_words_left;
                   continue;
               }

               FC_ASSERT( sub_words_left == 1, "unexpected error in fixed_bytes constructor" );
               temp_word |= static_cast<word_t>(*w_itr);
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
          * Get number of words contained in this fixed_bytes object. A word is defined to be 16 bytes in size
          */

         static constexpr size_t num_words() { return (Size + sizeof(word_t) - 1) / sizeof(word_t); }

         /**
          * Get number of padded bytes contained in this fixed_bytes object. Padded bytes are the remaining bytes
          * inside the fixed_bytes object after all the words are allocated
          */
         static constexpr size_t padded_bytes() { return num_words() * sizeof(word_t) - Size; }

         /**
         * Default constructor to fixed_bytes object which initializes all bytes to zero
         */
         constexpr fixed_bytes() : _data() {}

         /**
         * Constructor to fixed_bytes object from std::array of num_words() word_t types
         *
         * @param arr    data
         */
         fixed_bytes(const std::array<word_t, num_words()>& arr)
         {
           std::copy(arr.begin(), arr.end(), _data.begin());
         }

         /**
         * Constructor to fixed_bytes object from std::array of Word types smaller in size than word_t
         *
         * @param arr - Source data
         */
         template<typename Word, size_t NumWords,
                  typename Enable = typename std::enable_if<std::is_integral<Word>::value &&
                                                             std::is_unsigned<Word>::value &&
                                                             !std::is_same<Word, bool>::value &&
                                                             std::less<size_t>{}(sizeof(Word), sizeof(word_t))>::type >
         fixed_bytes(const std::array<Word, NumWords>& arr)
         {
            static_assert( sizeof(word_t) == (sizeof(word_t)/sizeof(Word)) * sizeof(Word),
                           "size of the backing word size is not divisible by the size of the array element" );
            static_assert( sizeof(Word) * NumWords <= Size, "too many words supplied to fixed_bytes constructor" );

            set_from_word_sequence<Word, NumWords>(arr.data(), arr.data() + arr.size(), *this);
         }

         /**
         * Constructor to fixed_bytes object from fixed-sized C array of Word types smaller in size than word_t
         *
         * @param arr - Source data
         */
         template<typename Word, size_t NumWords,
                  typename Enable = typename std::enable_if<std::is_integral<Word>::value &&
                                                             std::is_unsigned<Word>::value &&
                                                             !std::is_same<Word, bool>::value &&
                                                             std::less<size_t>{}(sizeof(Word), sizeof(word_t))>::type >
         fixed_bytes(const Word(&arr)[NumWords])
         {
            static_assert( sizeof(word_t) == (sizeof(word_t)/sizeof(Word)) * sizeof(Word),
                           "size of the backing word size is not divisible by the size of the array element" );
            static_assert( sizeof(Word) * NumWords <= Size, "too many words supplied to fixed_bytes constructor" );

            set_from_word_sequence<Word, NumWords>(arr, arr + NumWords, *this);
         }

         /**
         *  Create a new fixed_bytes object from a sequence of words
         *
         *  @tparam FirstWord - The type of the first word in the sequence
         *  @tparam Rest - The type of the remaining words in the sequence
         *  @param first_word - The first word in the sequence
         *  @param rest - The remaining words in the sequence
         */
         template<typename FirstWord, typename... Rest>
         static
         fixed_bytes<Size>
         make_from_word_sequence(typename std::enable_if<std::is_integral<FirstWord>::value &&
                                                          std::is_unsigned<FirstWord>::value &&
                                                          !std::is_same<FirstWord, bool>::value &&
                                                          sizeof(FirstWord) <= sizeof(word_t) &&
                                                          all_true<(std::is_same<FirstWord, Rest>::value)...>::value,
                                                         FirstWord>::type first_word,
                                 Rest... rest)
         {
            static_assert( sizeof(word_t) == (sizeof(word_t)/sizeof(FirstWord)) * sizeof(FirstWord),
                           "size of the backing word size is not divisible by the size of the words supplied as arguments" );
            static_assert( sizeof(FirstWord) * (1 + sizeof...(Rest)) <= Size, "too many words supplied to make_from_word_sequence" );

            fixed_bytes<Size> key;
            std::array<FirstWord, 1+sizeof...(Rest)> arr{{ first_word, rest... }};
            set_from_word_sequence<FirstWord, 1+sizeof...(Rest)>(arr.data(), arr.data() + arr.size(), key);
            return key;
         }

         /**
          * Get the contained std::array
          */
         const auto& get_array()const { return _data; }

         /**
          * Get the underlying data of the contained std::array
          */
         auto data() { return _data.data(); }

         /// @cond INTERNAL

         /**
          * Get the underlying data of the contained std::array
          */
         auto data()const { return _data.data(); }

         /// @endcond

         /**
          * Get the size of the contained std::array
          */
         auto size()const { return _data.size(); }


         /**
          * Extract the contained data as an array of bytes
          *
          * @return - the extracted data as array of bytes
          */
         std::array<uint8_t, Size> extract_as_byte_array()const {
            std::array<uint8_t, Size> arr;

            const size_t num_sub_words = sizeof(word_t);

            auto arr_itr  = arr.begin();
            auto data_itr = _data.begin();

            for( size_t counter = _data.size(); counter > 0; --counter, ++data_itr ) {
               size_t sub_words_left = num_sub_words;

               auto temp_word = *data_itr;
               if( counter == 1 ) { // If last word in _data array...
                  sub_words_left -= padded_bytes();
                  temp_word >>= 8*padded_bytes();
               }
               for( ; sub_words_left > 0; --sub_words_left ) {
                  *(arr_itr + sub_words_left - 1) = static_cast<uint8_t>(temp_word & 0xFF);
                  temp_word >>= 8;
               }
               arr_itr += num_sub_words;
            }

            return arr;
         }

         /**
          * Prints fixed_bytes as a hexidecimal string
          *
          * @param val to be printed
          */
         inline void print()const {
            auto arr = extract_as_byte_array();
            printhex(static_cast<const void*>(arr.data()), arr.size());
         }

         /// @cond OPERATORS

         friend bool operator == <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         friend bool operator != <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         friend bool operator > <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         friend bool operator < <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         friend bool operator >= <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         friend bool operator <= <>(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2);

         /// @endcond

      private:

         std::array<word_t, num_words()> _data;
    };

  /// @cond IMPLEMENTATIONS

   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 == c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator ==(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2) {
      return c1._data == c2._data;
   }

   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 != c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator !=(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2) {
      return c1._data != c2._data;
   }

   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 > c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator >(const fixed_bytes<Size>& c1, const fixed_bytes<Size>& c2) {
      return c1._data > c2._data;
   }

   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 < c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator <(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2) {
      return c1._data < c2._data;
   }

   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 >= c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator >=(const fixed_bytes<Size>& c1, const fixed_bytes<Size>& c2) {
      return c1._data >= c2._data;
   }

   /**
    * Lexicographically compares two fixed_bytes variables c1 and c2
    *
    * @param c1 - First fixed_bytes object to compare
    * @param c2 - Second fixed_bytes object to compare
    * @return if c1 <= c2, return true, otherwise false
    */
   template<size_t Size>
   bool operator <=(const fixed_bytes<Size> &c1, const fixed_bytes<Size> &c2) {
      return c1._data <= c2._data;
   }


   using checksum160 = fixed_bytes<20>;
   using checksum256 = fixed_bytes<32>;
   using checksum512 = fixed_bytes<64>;

   /**
    *  Serialize a fixed_bytes into a stream
    *
    *  @brief Serialize a fixed_bytes
    *  @param ds - The stream to write
    *  @param d - The value to serialize
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
   template<typename DataStream, size_t Size>
   inline DataStream& operator<<(DataStream& ds, const fixed_bytes<Size>& d) {
      auto arr = d.extract_as_byte_array();
      ds.write( (const char*)arr.data(), arr.size() );
      return ds;
   }

   /**
    *  Deserialize a fixed_bytes from a stream
    *
    *  @brief Deserialize a fixed_bytes
    *  @param ds - The stream to read
    *  @param d - The destination for deserialized value
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
   template<typename DataStream, size_t Size>
   inline DataStream& operator>>(DataStream& ds, fixed_bytes<Size>& d) {
      std::array<uint8_t, Size> arr;
      ds.read( (char*)arr.data(), arr.size() );
      d = fixed_bytes<Size>( arr );
      return ds;
   }

   /// @endcond
}
