#pragma once
#include <fc/exception/exception.hpp>
#include <fc/io/raw_fwd.hpp>
#include <fc/variant_object.hpp>
#include <fc/variant.hpp>

namespace fc { namespace raw {

    template<typename Stream>
    class variant_packer : public variant::visitor
    {
       public:
         variant_packer( Stream& _s ):s(_s){}
         virtual void handle()const { }
         virtual void handle( const int64_t& v )const
         {
            fc::raw::pack( s, v );
         }
         virtual void handle( const uint64_t& v )const
         {
            fc::raw::pack( s, v );
         }
         virtual void handle( const double& v )const 
         {
            fc::raw::pack( s, v );
         }
         virtual void handle( const bool& v )const
         {
            fc::raw::pack( s, v );
         }
         virtual void handle( const string& v )const
         {
            fc::raw::pack( s, v );
         }
         virtual void handle( const variant_object& v)const
         {
            fc::raw::pack( s, v );
         }
         virtual void handle( const variants& v)const
         {
            fc::raw::pack( s, v );
         }
        
         Stream& s;
        
    };


    template<typename Stream> 
    inline void pack( Stream& s, const variant& v )
    {
       pack( s, uint8_t(v.get_type()) );
       v.visit( variant_packer<Stream>(s) );
    }
    template<typename Stream> 
    inline void unpack( Stream& s, variant& v )
    {
      uint8_t t;
      unpack( s, t );
      switch( t )
      {
         case variant::null_type:
            return;
         case variant::int64_type:
         {
            int64_t val;
            raw::unpack(s,val);
            v = val;
            return;
         }
         case variant::uint64_type:
         {
            uint64_t val;
            raw::unpack(s,val);
            v = val;
            return;
         }
         case variant::double_type:
         {
            double val;
            raw::unpack(s,val);
            v = val;
            return;
         }
         case variant::bool_type:
         {
            bool val;
            raw::unpack(s,val);
            v = val;
            return;
         }
         case variant::string_type:
         {
            fc::string val;
            raw::unpack(s,val);
            v = fc::move(val);
            return;
         }
         case variant::array_type:
         {
            variants val;
            raw::unpack(s,val);
            v = fc::move(val);
            return;
         }
         case variant::object_type:
         {
            variant_object val; 
            raw::unpack(s,val);
            v = fc::move(val);
            return;
         }
         default:
            FC_THROW_EXCEPTION( parse_error_exception, "Unknown Variant Type ${t}", ("t", t) );
      }
    }

    template<typename Stream> 
    inline void pack( Stream& s, const variant_object& v ) 
    {
       unsigned_int vs = (uint32_t)v.size();
       pack( s, vs );
       for( auto itr = v.begin(); itr != v.end(); ++itr )
       {
          pack( s, itr->key() );
          pack( s, itr->value() );
       }
    }
    template<typename Stream> 
    inline void unpack( Stream& s, variant_object& v ) 
    {
       unsigned_int vs;
       unpack( s, vs );

       mutable_variant_object mvo;
       mvo.reserve(vs.value);
       for( uint32_t i = 0; i < vs.value; ++i )
       {
          fc::string key;
          fc::variant value;
          fc::raw::unpack(s,key);
          fc::raw::unpack(s,value);
          mvo.set( fc::move(key), fc::move(value) );
       }
       v = fc::move(mvo);
    }

} } // fc::raw
