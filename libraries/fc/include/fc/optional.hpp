#pragma once
#include <fc/utility.hpp>
#include <assert.h>


namespace fc {
#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable:4521)  /* multiple copy ctors */
# pragma warning(disable:4522) /* multiple assignment operators */
#endif
  bool assert_optional(bool is_valid ); // defined in exception.cpp

  /**
   *  @brief provides stack-based nullable value similar to boost::optional
   *
   *  Simply including boost::optional adds 35,000 lines to each object file, using
   *  fc::optional adds less than 400.
   */
  template<typename T>
  class optional
  {
    public:
      typedef T value_type;

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
        if( o._valid ) new (ptr()) T( fc::move(*o) );
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
        if( o._valid ) new (ptr()) T( fc::move(*o) );
        _valid = o._valid;
        o.reset();
      }

      template<typename U>
      optional( U&& u )
      :_valid(true) 
      {
        new ((char*)ptr()) T( fc::forward<U>(u) );
      }

      template<typename U>
      optional& operator=( U&& u ) 
      {
        reset();
        new (ptr()) T( fc::forward<U>(u) );
        _valid = true;
        return *this;
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
            ref() = fc::move(*o);
            o.reset();
          } else if ( !_valid && o._valid ) {
            *this = fc::move(*o);
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
            ref() = fc::move(*o);
            o.reset();
          } else if ( !_valid && o._valid ) {
            *this = fc::move(*o);
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

      T&       operator*()      { assert(_valid); return ref(); }
      const T& operator*()const { assert(_valid); return ref(); }

      T*       operator->()      
      { 
         assert(_valid);
         return ptr(); 
      }
      const T* operator->()const 
      { 
         assert(_valid);
         return ptr(); 
      }

      optional& operator=(std::nullptr_t)
      {
        reset();
        return *this;
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

      void     reset()    
      { 
          if( _valid ) 
          {
              ref().~T(); // cal destructor
          }
          _valid = false;
      }
    private:
      template<typename U> friend class optional;
      T&       ref()      { return *ptr(); }
      const T& ref()const { return *ptr(); }
      T*       ptr()      { void* v = &_value[0]; return static_cast<T*>(v); }
      const T* ptr()const { const void* v = &_value[0]; return static_cast<const T*>(v); }

      // force alignment... to 8 byte boundaries 
      double _value[((sizeof(T)+7)/8)];
      bool   _valid;
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

#ifdef _MSC_VER
# pragma warning(pop)
#endif

} // namespace fc

