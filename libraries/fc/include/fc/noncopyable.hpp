#pragma once

namespace fc
{
   class noncopyable
   {
      public:
         noncopyable(){}
      private:
         noncopyable( const noncopyable& ) = delete;
         noncopyable& operator=(  const noncopyable& ) = delete;
   };
}

