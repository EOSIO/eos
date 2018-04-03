#pragma once

namespace fc {

   template<typename Callback>
   class scoped_exit {
      public:
         template<typename C>
         scoped_exit( C&& c ):callback( std::forward<C>(c) ){}
         scoped_exit( scoped_exit&& mv ):callback( std::move( mv.callback ) ){}

         scoped_exit( const scoped_exit& ) = delete;
         scoped_exit& operator=( const scoped_exit& ) = delete;

         void cancel() { canceled = true; }

         ~scoped_exit() {
            if (!canceled)
               try { callback(); } catch( ... ) {}
         }

         scoped_exit& operator = ( scoped_exit&& mv ) {
            if( this != &mv ) {
               ~scoped_exit();
               callback = std::move(mv.callback);
               canceled = mv.canceled;
               mv.canceled = true;
            }

            return *this;
         }
      private:
         Callback callback;
         bool canceled = false;
   };

   template<typename Callback>
   scoped_exit<Callback> make_scoped_exit( Callback&& c ) {
      return scoped_exit<Callback>( std::forward<Callback>(c) );
   }

}
