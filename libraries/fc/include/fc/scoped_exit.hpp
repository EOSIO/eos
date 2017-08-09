#pragma once

namespace fc {

   template<typename Callback>
   class scoped_exit {
      public:
         template<typename C>
         scoped_exit( C&& c ):callback( std::forward<C>(c) ){}
         scoped_exit( scoped_exit&& mv ):callback( std::move( mv.callback ) ){}

         void cancel() { canceled = true; }

         ~scoped_exit() {
            if (!canceled)
               try { callback(); } catch( ... ) {}
         }

         scoped_exit& operator = ( scoped_exit&& mv ) {
            callback = std::move(mv.callback);
            return *this;
         }
      private:
         scoped_exit( const scoped_exit& );
         scoped_exit& operator=( const scoped_exit& );

         Callback callback;
         bool canceled = false;
   };

   template<typename Callback>
   scoped_exit<Callback> make_scoped_exit( Callback&& c ) {
      return scoped_exit<Callback>( std::forward<Callback>(c) );
   }

}
