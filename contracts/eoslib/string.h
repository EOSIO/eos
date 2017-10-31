/**
 + *  @file
 + *  @copyright defined in eos/LICENSE.txt
 + */
 #pragma once
 #include <eoslib/types.h>

   /**
   *  @defgroup stringapi String API
   *  @brief Defines string
   *  @ingroup contractdev
   */

 extern "C" {


  /**
   *  @defgroup stringcapi String C API
   *  @brief Defines string 
   *  @ingroup stringapi
   *
   *  @{
   */

    /**
    * @brief Get number of digit for unsigned int
    * 
    * @param val - the number which digit to be counted
    * @return uint32_t - length
    */
    uint32_t uint_digit_len( uint64_t val);

    /**
    * @brief Get number of digit for signed int
    * 
    * @param val - the number which digit to be counted
    * @return uint32_t - length
    */
    uint32_t int_digit_len( int64_t val);

    /**
     * @brief Convert unsigned int to char array representation
     * 
     * @param input - number to be converted
     * @param digit_len - len of number to be converted
     * @param output - resulting char array representation
     */
    void uint_to_char_arr( uint64_t input, uint32_t digit_len, char* output);

    /**
     * @brief Convert signed int to char array representation
     * 
     * @param input - number to be converted
     * @param digit_len - len of number to be converted
     * @param output - resulting char array representation
     */
    void int_to_char_arr( int64_t input, uint32_t digit_len, char* output);

    /// @}
}