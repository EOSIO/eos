#pragma once
#include <eoslib/utility.hpp>
#include <eoslib/memory.hpp>
#include <eoslib/stdlib.hpp>

namespace eosio {

   /**
    *  This class provides an interface similar to std::vector 
    */
   template<typename T>
   class vector {
      public:
         typedef uint32_t size_type;

         vector(){}

         vector( size_t s, const T& value ) {
            reserve( s );
            for( uint32_t i = 0; i < s; ++i )
               emplace_back( value );
         }

         vector( const vector& copy ) {
            reserve( copy.size() );
            for( uint32_t i = 0; i < copy.size(); ++i ) {
               emplace_back( copy[i] );
            }
         }

         vector( vector&& mv ) {
            _data  = mv._data;
            _size  = mv._size;
            _capacity = mv._capacity;
            mv._data = nullptr;
            mv._size = 0;
            mv._capacity = 0;
         }

         vector( std::initializer_list<T> init ) {
            resize(0);
            reserve( init.size() );

            int i = 0;
            for( auto itr = init.begin(); itr != init.end(); itr++ ) {
               emplace_back( *itr );
            }
         }

         ~vector() {
            for( uint32_t i = 0; i < _size; ++i )
               _data[i].~T();
            free( _data );
            _data     = nullptr;
            _size     = 0;
            _capacity = 0;
         }

         vector& operator = ( const vector& copy ) {
            if( this != &copy ) {
               resize(0);
               reserve( copy.size() );

               for( uint32_t i = 0; i < copy.size(); ++i ) {
                  new (&_data[i]) T( copy[i] );
               }
               _size = copy.size();
            }
            return *this;
         }

         vector& operator = ( vector&& mv ) {
            resize( 0 );
            _data  = mv._data;
            _size  = mv._size;
            _capacity = mv._capacity;
            mv._data = nullptr;
            mv._size = 0;
            mv._capacity = 0;
            return *this;
         }


         T&       operator[]( uint32_t index )      { return _data[index]; }
         const T& operator[]( uint32_t index )const { return _data[index]; }

         T* begin()              { return _data; }
         T* end()                { return _data + _size; }
         const T* begin()const   { return _data; }
         const T* end()const     { return _data + _size; }

         const T* data()const     { return _data;     }
         T* data()                { return _data;     }
         uint32_t size()const     { return _size;     }
         uint32_t capacity()const { return _capacity; }

         void push_back( const T& value ) {
            if( _size == _capacity )
               reserve( _size + 1 );
            new (&_data[_size]) T( value );
            ++_size;
         }

         template<typename... Args>
         void emplace_back( Args&& ...value ) {
            if( _size == _capacity )
               reserve( _size + 1 );
            new (&_data[_size]) T( forward<Args>(value)... );
            ++_size;
         }

         void resize( uint32_t new_size ) {
            reserve( new_size );
            if( new_size > _size ) 
            {
               for( uint32_t i = _size; i < new_size; ++i ) {
                  new (&_data[i]) T();
               }
            } 
            else if ( new_size < _size ) 
            {
               for( uint32_t i = new_size; i < _size; ++i ) {
                  _data[i].~T();
               }
            }
            _size = new_size;
         } /// resize

         void reserve( uint32_t new_capacity ) {
            if( new_capacity > _capacity ) {
               T* new_data = (T*)malloc( sizeof(T) * new_capacity );
               for( uint32_t i = 0; i < _size; ++i ) {
                  new (new_data+i) T( move( _data[i] ) );
                  _data[i].~T();
               }
               free( _data );
               _data = new_data;
               _capacity = new_capacity;
            }
         }

         void clear() {
            resize(0);
         }

      private:
         T*       _data     = nullptr;
         uint32_t _size     = 0;
         uint32_t _capacity = 0;
   }; /// class vector

}  /// namespace eosio
