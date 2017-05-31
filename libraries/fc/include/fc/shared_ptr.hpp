#pragma once
#include <fc/utility.hpp>

namespace fc {

  /**
   *  @brief used to create reference counted types.
   *
   *  Must be a virtual base class that is initialized with the
   *
   */
  class retainable {
    public:
      retainable();
      void    retain();
      void    release();
      int32_t retain_count()const;

    protected:
      virtual ~retainable();
    private:
      volatile int32_t _ref_count;
  };

  template<typename T>
  class shared_ptr {
    public:
      template<typename Other>
      shared_ptr( const shared_ptr<Other>& o )
      :_ptr(o.get()) {
        if(_ptr != nullptr ) _ptr->retain();
      }
      shared_ptr( const shared_ptr& o ) 
      :_ptr(o.get()) {
        if(_ptr != nullptr ) _ptr->retain();
      }
      
      shared_ptr( T* t, bool inc = false )
      :_ptr(t) { if( inc && t != nullptr) t->retain();  }

      shared_ptr():_ptr(nullptr){}

     
      shared_ptr( shared_ptr&& p )
      :_ptr(p._ptr){ p._ptr = nullptr; }

      ~shared_ptr() { if( nullptr != _ptr ) { _ptr->release(); } }

      shared_ptr& reset( T* v = nullptr, bool inc = false )  {
        if( v == _ptr ) return *this;
        if( inc &&  nullptr != v ) v->retain();
        if( nullptr != _ptr ) _ptr->release();
        _ptr = v;
        return *this;
      }

      shared_ptr& operator=(const shared_ptr& p ) {
        if( _ptr == p._ptr ) return *this;
        if( p._ptr != nullptr ) p._ptr->retain();
        if( _ptr != nullptr ) _ptr->release();
        _ptr = p._ptr;
        return *this;
      }
      shared_ptr& operator=(shared_ptr&& p ) {
        fc_swap(_ptr,p._ptr); 
        return *this;
      }
      T& operator*  ()const  { return *_ptr; }
      T* operator-> ()const  { return _ptr; }

      bool operator==( const shared_ptr& p )const { return get() == p.get(); }
      bool operator<( const shared_ptr& p )const  { return get() < p.get();  }
      T * get() const { return _ptr; }

      bool operator!()const { return _ptr == 0; }
      operator bool()const  { return _ptr != 0; }
    private:
      T* _ptr;
  };

  template<typename T, typename O>
  fc::shared_ptr<T> dynamic_pointer_cast( const fc::shared_ptr<O>& t ) {
    return fc::shared_ptr<T>( dynamic_cast<T*>(t.get()), true );
  }
  template<typename T, typename O>
  fc::shared_ptr<T> static_pointer_cast( const fc::shared_ptr<O>& t ) {
    return fc::shared_ptr<T>( static_cast<T*>(t.get()), true );
  }
}

