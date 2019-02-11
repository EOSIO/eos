#pragma once

#include <utility>

namespace eosio {
  /**
   * @defgroup optionaltype Optional Type
   * @brief Defines otional type which is similar to boost::optional 
   * @ingroup types
   * @{
   */

  /**
   *  Provides stack-based nullable value similar to boost::optional
   * 
   *  @brief Provides stack-based nullable value similar to boost::optional
   */
   template<typename T>
   class optional {
      public:
         typedef T value_type;
         typedef typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_type;
         
         /**
          * Default constructor
          * 
          * @brief Construct a new optional object
          */
         optional():_valid(false){}

         /**
          * Destructor
          * 
          * @brief Destroy the optional object
          */
         ~optional(){ reset(); }

         /**
          * Construct a new optional object from another optional object
          * 
          * @brief Construct a new optional object
          */
         optional( optional& o )
            :_valid(false)
         {
            if( o._valid ) new (ptr()) T( *o );
            _valid = o._valid;
         }

         /**
          * Copy constructor
          * 
          * @brief Construct a new optional object
          */
         optional( const optional& o )
            :_valid(false)
         {
            if( o._valid ) new (ptr()) T( *o );
            _valid = o._valid;
         }

         /**
          * Move constructor
          * 
          * @brief Construct a new optional object
          */
         optional( optional&& o )
            :_valid(false)
         {
            if( o._valid ) new (ptr()) T( std::move(*o) );
            _valid = o._valid;
            o.reset();
         }

         /**
          * Construct a new optional object from another type of optional object
          * 
          * @brief Construct a new optional object from another type of optional object
          */
         template<typename U>
         optional( const optional<U>& o )
            :_valid(false)
         {
            if( o._valid ) new (ptr()) T( *o );
            _valid = o._valid;
         }

         /**
          * Construct a new optional object from another type of optional object
          * 
          * @brief Construct a new optional object from another type of optional object
          */
         template<typename U>
         optional( optional<U>& o )
            :_valid(false)
         {
            if( o._valid )
               {
                  new (ptr()) T( *o );
               }
            _valid = o._valid;
         }

         /**
          * Construct a new optional object from another type of optional object
          * 
          * @brief Construct a new optional object from another type of optional object
          */
         template<typename U>
         optional( optional<U>&& o )
            :_valid(false)
         {
            if( o._valid ) new (ptr()) T( std::move(*o) );
            _valid = o._valid;
            o.reset();
         }

         /**
          * Construct a new optional object from another object
          * 
          * @brief Construct a new optional object from another object
          */
         template<typename U>
         optional( U&& u )
            :_valid(true)
         {
            new ((char*)ptr()) T( std::forward<U>(u) );
         }

         /**
          * Construct a new optional object from another object
          * 
          * @brief Construct a new optional object from another object
          */
         template<typename U>
         optional& operator=( U&& u )
         {
            reset();
            new (ptr()) T( std::forward<U>(u) );
            _valid = true;
            return *this;
         }

        /**
         * Construct the contained value in place
         * 
         * @brief Construct the contained value in place
         * @tparam Args - Type of the contained value
         * @param args - The value to be assigned as contained value
         */
         template<typename ...Args>
         void emplace(Args&& ... args) {
            if (_valid) {
               reset();
            }

            new ((char*)ptr()) T( std::forward<Args>(args)... );
            _valid = true;
         }

        /**
         * Assignment Operator
         * 
         * @brief Assignment Operator
         * @tparam U - Type of the contained value of the optional object to be assigned from
         * @param o - The other optional object to be assigned from
         * @return optional& - The reference to this object
         */
         template<typename U>
         optional& operator=( optional<U>& o ) {
            if (this != &o) {
               if( _valid && o._valid ) {
                  ref() = *o;
               } else if( !_valid && o._valid ) {
                  new (ptr()) T( *o );
                  _valid = true;
               } else if (_valid) {
                  reset();
               }
            }
            return *this;
         }

        /**
         * Assignment Operator
         * 
         * @brief Assignment Operator
         * @tparam U - Type of the contained value of the optional object to be assigned from
         * @param o - The other optional object to be assigned from
         * @return optional& - The reference to this object
         */
         template<typename U>
         optional& operator=( const optional<U>& o ) {
            if (this != &o) {
               if( _valid && o._valid ) {
                  ref() = *o;
               } else if( !_valid && o._valid ) {
                  new (ptr()) T( *o );
                  _valid = true;
               } else if (_valid) {
                  reset();
               }
            }
            return *this;
         }

        /**
         * Assignment Operator
         * 
         * @brief Assignment Operator
         * @param o - The other optional object to be assigned from
         * @return optional& - The reference to this object
         */
         optional& operator=( optional& o ) {
            if (this != &o) {
               if( _valid && o._valid ) {
                  ref() = *o;
               } else if( !_valid && o._valid ) {
                  new (ptr()) T( *o );
                  _valid = true;
               } else if (_valid) {
                  reset();
               }
            }
            return *this;
         }

        /**
         * Assignment Operator
         * 
         * @brief Assignment Operator
         * @param o - The other optional object to be assigned from
         * @return optional& - The reference to this object
         */
         optional& operator=( const optional& o ) {
            if (this != &o) {
               if( _valid && o._valid ) {
                  ref() = *o;
               } else if( !_valid && o._valid ) {
                  new (ptr()) T( *o );
                  _valid = true;
               } else if (_valid) {
                  reset();
               }
            }
            return *this;
         }

        /**
         * Assignment Operator
         * 
         * @brief Assignment Operator
         * @tparam U - Type of the contained value of the optional object to be assigned from
         * @param o - The other optional object to be assigned from
         * @return optional& - The reference to this object
         */
         template<typename U>
         optional& operator=( optional<U>&& o )
         {
            if (this != &o)
               {
                  if( _valid && o._valid )
                     {
                        ref() = std::move(*o);
                        o.reset();
                     } else if ( !_valid && o._valid ) {
                     *this = std::move(*o);
                  } else if (_valid) {
                     reset();
                  }
               }
            return *this;
         }

        /**
         * Assignment Operator
         * 
         * @brief Assignment Operator
         * @param o - The other optional object to be assigned from
         * @return optional& - The reference to this object
         */
         optional& operator=( optional&& o )
         {
            if (this != &o)
               {
                  if( _valid && o._valid )
                     {
                        ref() = std::move(*o);
                        o.reset();
                     } else if ( !_valid && o._valid ) {
                     *this = std::move(*o);
                  } else if (_valid) {
                     reset();
                  }
               }
            return *this;
         }

         /**
          * Check if this optional has valid contained value
          * 
          * @brief Check if this optional has valid contained value
          * @return true - if this optional has valid contained value
          * @return false - otherwise
          */
         bool valid()const     { return _valid;  }

         /**
          * Logical Negation operator
          * 
          * @brief Logical Negation Operator
          * @return true - if this optional has invalid contained value
          * @return false - otherwise
          */
         bool operator!()const { return !_valid; }

         /**
          * Similar to valid(). However, this operation is not safe and can result in unintential
          * casts and comparisons, use valid() or !!
          * 
          * @brief Check if this optional has valid contained value
          * @return true - if this optional has valid contained value
          * @return false - otherwise
          */
         explicit operator bool()const  { return _valid;  }

         /**
          * Get contained value of this optional
          * 
          * @brief Pointer Dereference operator
          * @return T& - Contained value
          */
         T&       operator*()      { eosio_assert(_valid, "dereference of empty optional"); return ref(); }

         /**
          * Get contained value of this optional
          * 
          * @brief Pointer Dereference operator
          * @return T& - Contained value
          */
         const T& operator*()const { eosio_assert(_valid, "dereference of empty optional"); return ref(); }

         /**
          * Get pointer to the contained value
          * 
          * @brief Member Access Through Pointer Operator
          * @return T& - The pointer to the contained value
          */
         T*       operator->()
         {
            eosio_assert(_valid, "dereference of empty optional");
            return ptr();
         }

         /**
          * Get pointer to the contained value
          * 
          * @brief Member Access Through Pointer Operator
          * @return T& - The pointer to the contained value
          */         
         const T* operator->()const
         {
            eosio_assert(_valid, "dereference of empty optional");
            return ptr();
         }

        /**
         * Assignment Operator with nullptr
         * 
         * @brief Assignment Operator with nullptr
         * @return optional& - The reference to this object
         */
         optional& operator=(std::nullptr_t)
         {
            reset();
            return *this;
         }

         /**
          * Call the destructor fot he contained value and mark this optional as valid
          * 
          * @brief Reset the optional object
          */
         void reset()
         {
            if( _valid ) {
                  ref().~T(); // cal destructor
            }
            _valid = false;
         }

         /**
          * Check if a contained value is less than b contained value
          * 
          * @brief Less than operator
          * @param a - First object to compare
          * @param b - Second object to compare
          * @return true - if a contained value is less than b contained value
          * @return false - otherwise
          */
         friend bool operator < ( const optional a, optional b )
         {
            if( a.valid() && b.valid() ) return *a < *b;
            return a.valid() < b.valid();
         }

         /**
          * Check if a contained value is equal to b contained value
          * 
          * @brief Equality operator
          * @param a - First object to compare
          * @param b - Second object to compare
          * @return true - if contained value is equal to b contained value
          * @return false - otherwise
          */
         friend bool operator == ( const optional a, optional b )
         {
            if( a.valid() && b.valid() ) return *a == *b;
            return a.valid() == b.valid();
         }

        /**
         *  Serialize an optional object
         * 
         *  @brief Serialize an optional object
         *  @param ds - The stream to write
         *  @param op - The value to serialize
         *  @tparam Stream - Type of datastream 
         *  @return eosio::datastream<Stream>& - Reference to the datastream
         */
         template<typename Stream>
         friend inline eosio::datastream<Stream>& operator>> (eosio::datastream<Stream>& ds, optional& op)
         {
            char valid = 0;
            ds >> valid;
            if (valid) {
               op._valid = true;
               ds >> *op;
            }
            return ds;
         }

        /**
         *  Deserialize an optional object
         * 
         *  @brief Deserialize an optional object
         *  @param ds - The stream to read
         *  @param op - The destination for deserialized value
         *  @tparam Stream - Type of datastream
         *  @return eosio::datastream<Stream>& - Reference to the datastream
         */
         template<typename Stream>
         friend inline eosio::datastream<Stream>& operator<< (eosio::datastream<Stream>& ds, const optional& op)
         {
            char valid = op._valid;
            ds << valid;
            if (valid) ds << *op;
            return ds;
         }

      private:
         template<typename U> friend class optional;
         T&       ref()      { return *ptr(); }
         const T& ref()const { return *ptr(); }
         T*       ptr()      { return reinterpret_cast<T*>(&_value); }
         const T* ptr()const { return reinterpret_cast<const T*>(&_value); }

         bool         _valid;
         storage_type _value;

   };

   /**
    * Check equality between two optional object that shares same type of contained value
    * 
    * @brief Equality Operator
    * @tparam T - Type of contained value of the optional objects
    * @param left - First object to be compared
    * @param right - Second object to be compared
    * @return true - if both optional objects are equal
    * @return false 
    */
   template<typename T>
   bool operator == ( const optional<T>& left, const optional<T>& right ) {
      return (!left == !right) || (!!left && *left == *right);
   }

    /**
    * Check equality between an optional object with another object
    * 
    * @brief Equality Operator
    * @tparam T - Type of contained value of the optional object
    * @tparam U - Type of the other object to be compared
    * @param left - First object to be compared
    * @param u - Second object to be compared
    * @return true - if the optional objects contained value is equal with the compared object
    * @return false 
    */
   template<typename T, typename U>
   bool operator == ( const optional<T>& left, const U& u ) {
      return !!left && *left == u;
   }

   /**
    * Check inequality between two optional object that shares same type of contained value
    * 
    * @brief Inquality Operator
    * @tparam T - Type of contained value of the optional objects
    * @param left - First object to be compared
    * @param right - Second object to be compared
    * @return true - if both optional objects are unequal
    * @return false 
    */
   template<typename T>
   bool operator != ( const optional<T>& left, const optional<T>& right ) {
      return (!left != !right) || (!!left && *left != *right);
   }

   /**
    * Check inequality between an optional object with another object
    * 
    * @brief Inqquality Operator
    * @tparam T - Type of contained value of the optional object
    * @tparam U - Type of the other object to be compared
    * @param left - First object to be compared
    * @param u - Second object to be compared
    * @return true - if the optional objects contained value is unequal with the compared object
    * @return false 
    */
   template<typename T, typename U>
   bool operator != ( const optional<T>& left, const U& u ) {
      return !left || *left != u;
   }
///@} optional
} // namespace eosio
