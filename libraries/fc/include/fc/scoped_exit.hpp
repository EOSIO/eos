#pragma once

#include <utility>

namespace fc {

   template<typename Callback>
   class scoped_exit {
      public:
         template<typename C>
         scoped_exit( C&& c ):callback( std::forward<C>(c) ){}

         scoped_exit( scoped_exit&& mv )
         :callback( std::move( mv.callback ) ),canceled(mv.canceled)
         {
            mv.canceled = true;
         }

         scoped_exit( const scoped_exit& ) = delete;
         scoped_exit& operator=( const scoped_exit& ) = delete;

         ~scoped_exit() {
            if (!canceled)
               try { callback(); } catch( ... ) {}
         }

         void cancel() { canceled = true; }

      private:
         Callback callback;
         bool canceled = false;
   };

   template<typename Callback>
   scoped_exit<Callback> make_scoped_exit( Callback&& c ) {
      return scoped_exit<Callback>( std::forward<Callback>(c) );
   }

   /// Similar to scoped_exit except it wraps (takes ownership) of an object T.
   /// T is provided to the Callback at scope exit.
   /// Access to wrapped object is provided by get_wrapped().
   template<typename Callback, typename T>
   class wrapped_scoped_exit {
      public:
         template<typename C, typename U>
         wrapped_scoped_exit( C&& c, U&& w )
         : callback( std::forward<C>(c) )
         , wrapped( std::forward<U>(w) ) {}

         wrapped_scoped_exit( wrapped_scoped_exit&& mv )
               : callback( std::move( mv.callback ) )
               , wrapped( std::move( mv.wrapped) )
               , canceled( mv.canceled )
         {
            mv.canceled = true;
         }

         wrapped_scoped_exit( const wrapped_scoped_exit& ) = delete;
         wrapped_scoped_exit& operator=( const wrapped_scoped_exit& ) = delete;

         ~wrapped_scoped_exit() {
            if (!canceled)
               try { callback(wrapped); } catch( ... ) {}
         }

         void cancel() { canceled = true; }

         T& get_wrapped() { return wrapped; }
         const T& get_wrapped() const { return wrapped; }

      private:
         Callback callback;
         T wrapped;
         bool canceled = false;
   };

   template<typename Callback, typename T>
   wrapped_scoped_exit<Callback,T> make_wrapped_scoped_exit( Callback&& c, T&& t ) {
      return wrapped_scoped_exit<Callback,T>( std::forward<Callback>(c), std::forward<T>(t) );
   }

}
