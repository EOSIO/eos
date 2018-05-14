#pragma once

#include <utility>

namespace eosio {

   template<typename T>
   class optional {
      public:
         typedef T value_type;
         typedef typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_type;

         optional():_valid(false){}
         ~optional(){ reset(); }

         optional( optional& o )
            :_valid(false)
         {
            if( o._valid ) new (ptr()) T( *o );
            _valid = o._valid;
         }

         optional( const optional& o )
            :_valid(false)
         {
            if( o._valid ) new (ptr()) T( *o );
            _valid = o._valid;
         }

         optional( optional&& o )
            :_valid(false)
         {
            if( o._valid ) new (ptr()) T( std::move(*o) );
            _valid = o._valid;
            o.reset();
         }

         template<typename U>
         optional( const optional<U>& o )
            :_valid(false)
         {
            if( o._valid ) new (ptr()) T( *o );
            _valid = o._valid;
         }

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

         template<typename U>
         optional( optional<U>&& o )
            :_valid(false)
         {
            if( o._valid ) new (ptr()) T( std::move(*o) );
            _valid = o._valid;
            o.reset();
         }

         template<typename U>
         optional( U&& u )
            :_valid(true)
         {
            new ((char*)ptr()) T( std::forward<U>(u) );
         }

         template<typename U>
         optional& operator=( U&& u )
         {
            reset();
            new (ptr()) T( std::forward<U>(u) );
            _valid = true;
            return *this;
         }

         template<typename ...Args>
         void emplace(Args&& ... args) {
            if (_valid) {
               reset();
            }

            new ((char*)ptr()) T( std::forward<Args>(args)... );
            _valid = true;
         }

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

         bool valid()const     { return _valid;  }
         bool operator!()const { return !_valid; }

         // this operation is not safe and can result in unintential
         // casts and comparisons, use valid() or !!
         explicit operator bool()const  { return _valid;  }

         T&       operator*()      { eosio_assert(_valid, "dereference of empty optional"); return ref(); }
         const T& operator*()const { eosio_assert(_valid, "dereference of empty optional"); return ref(); }

         T*       operator->()
         {
            eosio_assert(_valid, "dereference of empty optional");
            return ptr();
         }
         const T* operator->()const
         {
            eosio_assert(_valid, "dereference of empty optional");
            return ptr();
         }

         optional& operator=(std::nullptr_t)
         {
            reset();
            return *this;
         }

         void reset()
         {
            if( _valid ) {
                  ref().~T(); // cal destructor
            }
            _valid = false;
         }

         friend bool operator < ( const optional a, optional b )
         {
            if( a.valid() && b.valid() ) return *a < *b;
            return a.valid() < b.valid();
         }
         friend bool operator == ( const optional a, optional b )
         {
            if( a.valid() && b.valid() ) return *a == *b;
            return a.valid() == b.valid();
         }

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

   template<typename T>
   bool operator == ( const optional<T>& left, const optional<T>& right ) {
      return (!left == !right) || (!!left && *left == *right);
   }
   template<typename T, typename U>
   bool operator == ( const optional<T>& left, const U& u ) {
      return !!left && *left == u;
   }
   template<typename T>
   bool operator != ( const optional<T>& left, const optional<T>& right ) {
      return (!left != !right) || (!!left && *left != *right);
   }
   template<typename T, typename U>
   bool operator != ( const optional<T>& left, const U& u ) {
      return !left || *left != u;
   }

} // namespace eosio
