#pragma once
#include <fc/utility.hpp>
#include <memory>

namespace fc
{
    template<typename T>
    class unique_ptr
    {
       public:
         typedef T* pointer;

         explicit unique_ptr( pointer t = nullptr ):_p(t){}

         unique_ptr( unique_ptr&& m ) 
         :_p(m._p){ m._p = nullptr; }

         ~unique_ptr() { delete _p; }

         operator bool()const { return _p != nullptr; }
         friend bool operator==(const unique_ptr& p, nullptr_t)
         {
              return p._p == nullptr;
         }

         friend bool operator!=(const unique_ptr& p, nullptr_t)
         {
              return p._p != nullptr;
         }

         unique_ptr& operator=( nullptr_t )
         {
            delete _p; _p = nullptr;
         }

         unique_ptr& operator=( unique_ptr&& o )
         {
            fc_swap( _p, o._p );
            return *this;
         }

         pointer operator->()const { return _p; }
         T& operator*()const  { return *_p; }

         void reset( pointer v )
         {
              delete _p; _p = v;
         }
         pointer release()
         {
             auto tmp = _p;
             _p = nullptr;
             return tmp;
         }
       
       private:
         pointer _p;
    };

}
