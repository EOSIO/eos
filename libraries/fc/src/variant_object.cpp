#include <fc/variant_object.hpp>
#include <fc/exception/exception.hpp>
#include <assert.h>


namespace fc
{
   // ---------------------------------------------------------------
   // entry

   variant_object::entry::entry() {}
   variant_object::entry::entry( string k, variant v ) : _key(fc::move(k)),_value(fc::move(v)) {}
   variant_object::entry::entry( entry&& e ) : _key(fc::move(e._key)),_value(fc::move(e._value)) {}
   variant_object::entry::entry( const entry& e ) : _key(e._key),_value(e._value) {}
   variant_object::entry& variant_object::entry::operator=( const variant_object::entry& e )
   {
      if( this != &e ) 
      {
         _key = e._key;
         _value = e._value;
      }
      return *this;
   }
   variant_object::entry& variant_object::entry::operator=( variant_object::entry&& e )
   {
      fc_swap( _key, e._key );
      fc_swap( _value, e._value );
      return *this;
   }
   
   const string&        variant_object::entry::key()const
   {
      return _key;
   }

   const variant& variant_object::entry::value()const
   {
      return _value;
   }
   variant& variant_object::entry::value()
   {
      return _value;
   }

   void  variant_object::entry::set( variant v )
   {
      fc_swap( _value, v );
   }

   // ---------------------------------------------------------------
   // variant_object

   variant_object::iterator variant_object::begin() const
   {
      assert( _key_value != nullptr );
      return _key_value->begin();
   }

   variant_object::iterator variant_object::end() const
   {
      return _key_value->end();
   }

   variant_object::iterator variant_object::find( const string& key )const
   {
      return find( key.c_str() );
   }

   variant_object::iterator variant_object::find( const char* key )const
   {
      for( auto itr = begin(); itr != end(); ++itr )
      {
         if( itr->key() == key )
         {
            return itr;
         }
      }
      return end();
   }

   const variant& variant_object::operator[]( const string& key )const
   {
      return (*this)[key.c_str()];
   }

   const variant& variant_object::operator[]( const char* key )const
   {
      auto itr = find( key );
      if( itr != end() ) return itr->value();
      FC_THROW_EXCEPTION( key_not_found_exception, "Key ${key}", ("key",key) );
   }

   size_t variant_object::size() const
   {
      return _key_value->size();
   }

   variant_object::variant_object() 
      :_key_value(std::make_shared<std::vector<entry>>() )
   {
   }

   variant_object::variant_object( string key, variant val )
      : _key_value(std::make_shared<std::vector<entry>>())
   {
       //_key_value->push_back(entry(fc::move(key), fc::move(val)));
       _key_value->emplace_back(entry(fc::move(key), fc::move(val)));
   }

   variant_object::variant_object( const variant_object& obj )
   :_key_value( obj._key_value )
   {
      assert( _key_value != nullptr );
   }

   variant_object::variant_object( variant_object&& obj)
   : _key_value( fc::move(obj._key_value) )
   {
      obj._key_value = std::make_shared<std::vector<entry>>();
      assert( _key_value != nullptr );
   }

   variant_object::variant_object( const mutable_variant_object& obj )
      : _key_value(std::make_shared<std::vector<entry>>(*obj._key_value))
   {
   }

   variant_object::variant_object( mutable_variant_object&& obj )
   : _key_value(fc::move(obj._key_value))
   {
      assert( _key_value != nullptr );
   }

   variant_object& variant_object::operator=( variant_object&& obj )
   {
      if (this != &obj)
      {
         fc_swap(_key_value, obj._key_value );
         assert( _key_value != nullptr );
      }
      return *this;
   }

   variant_object& variant_object::operator=( const variant_object& obj )
   {
      if (this != &obj)
      {
         _key_value = obj._key_value;
      }
      return *this;
   }

   variant_object& variant_object::operator=( mutable_variant_object&& obj )
   {
      _key_value = fc::move(obj._key_value);
      obj._key_value.reset( new std::vector<entry>() );
      return *this;
   }

   variant_object& variant_object::operator=( const mutable_variant_object& obj )
   {
      *_key_value = *obj._key_value;
      return *this;
   }


   void to_variant( const variant_object& var,  variant& vo, uint32_t )
   {
      vo = variant(var);
   }

   void from_variant( const variant& var,  variant_object& vo, uint32_t )
   {
      vo = var.get_object();
   }

   // ---------------------------------------------------------------
   // mutable_variant_object

   mutable_variant_object::iterator mutable_variant_object::begin()
   {
      return _key_value->begin();
   }

   mutable_variant_object::iterator mutable_variant_object::end() 
   {
      return _key_value->end();
   }

   mutable_variant_object::iterator mutable_variant_object::begin() const
   {
      return _key_value->begin();
   }

   mutable_variant_object::iterator mutable_variant_object::end() const
   {
      return _key_value->end();
   }

   mutable_variant_object::iterator mutable_variant_object::find( const string& key )const
   {
      return find( key.c_str() );
   }

   mutable_variant_object::iterator mutable_variant_object::find( const char* key )const
   {
      for( auto itr = begin(); itr != end(); ++itr )
      {
         if( itr->key() == key )
         {
            return itr;
         }
      }
      return end();
   }

   mutable_variant_object::iterator mutable_variant_object::find( const string& key )
   {
      return find( key.c_str() );
   }

   mutable_variant_object::iterator mutable_variant_object::find( const char* key )
   {
      for( auto itr = begin(); itr != end(); ++itr )
      {
         if( itr->key() == key )
         {
            return itr;
         }
      }
      return end();
   }

   const variant& mutable_variant_object::operator[]( const string& key )const
   {
      return (*this)[key.c_str()];
   }

   const variant& mutable_variant_object::operator[]( const char* key )const
   {
      auto itr = find( key );
      if( itr != end() ) return itr->value();
      FC_THROW_EXCEPTION( key_not_found_exception, "Key ${key}", ("key",key) );
   }
   variant& mutable_variant_object::operator[]( const string& key )
   {
      return (*this)[key.c_str()];
   }

   variant& mutable_variant_object::operator[]( const char* key )
   {
      auto itr = find( key );
      if( itr != end() ) return itr->value();
      _key_value->emplace_back(entry(key, variant()));
      return _key_value->back().value();
   }

   size_t mutable_variant_object::size() const
   {
      return _key_value->size();
   }

   mutable_variant_object::mutable_variant_object() 
      :_key_value(new std::vector<entry>)
   {
   }

   mutable_variant_object::mutable_variant_object( string key, variant val )
      : _key_value(new std::vector<entry>())
   {
       _key_value->push_back(entry(fc::move(key), fc::move(val)));
   }

   mutable_variant_object::mutable_variant_object( const variant_object& obj )
      : _key_value( new std::vector<entry>(*obj._key_value) )
   {
   }

   mutable_variant_object::mutable_variant_object( const mutable_variant_object& obj )
      : _key_value( new std::vector<entry>(*obj._key_value) )
   {
   }

   mutable_variant_object::mutable_variant_object( mutable_variant_object&& obj )
      : _key_value(fc::move(obj._key_value))
   {
   }

   mutable_variant_object& mutable_variant_object::operator=( const variant_object& obj )
   {
      *_key_value = *obj._key_value;
      return *this;
   }

   mutable_variant_object& mutable_variant_object::operator=( mutable_variant_object&& obj )
   {
      if (this != &obj)
      {
         _key_value = fc::move(obj._key_value);
      }
      return *this;
   }

   mutable_variant_object& mutable_variant_object::operator=( const mutable_variant_object& obj )
   {
      if (this != &obj)
      {
         *_key_value = *obj._key_value;
      }
      return *this;
   }

   void mutable_variant_object::reserve( size_t s )
   {
      _key_value->reserve(s);
   }

   void  mutable_variant_object::erase( const string& key )
   {
      for( auto itr = begin(); itr != end(); ++itr )
      {
         if( itr->key() == key )
         {
            _key_value->erase(itr);
            return;
         }
      }
   }

   /** replaces the value at \a key with \a var or insert's \a key if not found */
   mutable_variant_object& mutable_variant_object::set( string key, variant var )
   {
      auto itr = find( key.c_str() );
      if( itr != end() )
      {
         itr->set( fc::move(var) );
      }
      else
      {
         _key_value->push_back( entry( fc::move(key), fc::move(var) ) );
      }
      return *this;
   }

   /** Appends \a key and \a var without checking for duplicates, designed to
    *  simplify construction of dictionaries using (key,val)(key2,val2) syntax 
    */
   mutable_variant_object& mutable_variant_object::operator()( string key, variant var, uint32_t )
   {
      _key_value->push_back( entry( fc::move(key), fc::move(var) ) );
      return *this;
   }

   mutable_variant_object& mutable_variant_object::operator()( const variant_object& vo )
   {
      for( const variant_object::entry& e : vo )
         set( e.key(), e.value() );
      return *this;
   }

   mutable_variant_object& mutable_variant_object::operator()( const mutable_variant_object& mvo )
   {
      if( &mvo == this )     // mvo(mvo) is no-op
         return *this;
      for( const mutable_variant_object::entry& e : mvo )
         set( e.key(), e.value() );
      return *this;
   }

   limited_mutable_variant_object::limited_mutable_variant_object( uint32_t m, bool skip_on_exception )
         : mutable_variant_object(),
           _max_depth(m - 1),
           _reached_depth_limit(m == 0),
           _skip_on_exception(skip_on_exception)
   {
      if( !skip_on_exception )
         FC_ASSERT( m > 0, "Recursion depth exceeded!" );
      else if( m == 0 )
         set( "__err_msg", "[ERROR: Recusion depth exceeded!]" );
   }

   limited_mutable_variant_object& limited_mutable_variant_object::operator()( const variant_object& vo )
   {
      if( _reached_depth_limit )
         // _skip_on_exception will always be true here
         return *this;

      try
      {
         mutable_variant_object::operator()( vo );
      }
      catch( ... )
      {
         if( !_skip_on_exception )
            throw;
         else
            set( "__err_msg", "[ERROR: Caught exception in operator()( const variant_object& ).]" );
      }
      return *this;
   }

   void to_variant( const mutable_variant_object& var,  variant& vo, uint32_t )
   {
      vo = variant(var);
   }

   void from_variant( const variant& var,  mutable_variant_object& vo, uint32_t )
   {
      vo = var.get_object();
   }

} // namesapce fc
