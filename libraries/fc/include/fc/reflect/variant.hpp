#pragma once
#include <fc/reflect/reflect.hpp>
#include <fc/variant_object.hpp>

namespace fc
{
   template<typename T>
   void to_variant( const T& o, variant& v, uint32_t max_depth );
   template<typename T>
   void from_variant( const variant& v, T& o, uint32_t max_depth );


   template<typename T>
   class to_variant_visitor
   {
      public:
         to_variant_visitor( mutable_variant_object& mvo, const T& v, uint32_t max_depth )
         :vo(mvo),val(v),_max_depth(max_depth - 1) {
            _FC_ASSERT( max_depth > 0, "Recursion depth exceeded!" );
         }

         template<typename Member, class Class, Member (Class::*member)>
         void operator()( const char* name )const
         {
            this->add(vo,name,(val.*member));
         }

      private:
         template<typename M>
         void add( mutable_variant_object& vo, const char* name, const optional<M>& v )const
         { 
            if( v.valid() )
               vo(name, variant( *v, _max_depth ));
         }
         template<typename M>
         void add( mutable_variant_object& vo, const char* name, const M& v )const
         { vo(name, variant( v, _max_depth )); }

         mutable_variant_object& vo;
         const T& val;
         const uint32_t _max_depth;
   };

   template<typename T>
   class from_variant_visitor : reflector_verifier_visitor<T>
   {
      public:
      from_variant_visitor( const variant_object& _vo, T& v, uint32_t max_depth )
         :reflector_verifier_visitor<T>(v)
         ,vo(_vo)
         ,_max_depth(max_depth - 1) {
            _FC_ASSERT( max_depth > 0, "Recursion depth exceeded!" );
         }

         template<typename Member, class Class, Member (Class::*member)>
         void operator()( const char* name )const
         {
            auto itr = vo.find(name);
            if( itr != vo.end() )
               from_variant( itr->value(), this->obj.*member, _max_depth );
         }

         const variant_object& vo;
         const uint32_t _max_depth;
   };

   template<typename IsReflected=fc::false_type>
   struct if_enum 
   {
     template<typename T>
     static inline void to_variant( const T& v, fc::variant& vo, uint32_t max_depth )
     { 
         mutable_variant_object mvo;
         fc::reflector<T>::visit( to_variant_visitor<T>( mvo, v, max_depth ) );
         vo = fc::move(mvo);
     }
     template<typename T>
     static inline void from_variant( const fc::variant& v, T& o, uint32_t max_depth )
     { 
         const variant_object& vo = v.get_object();
         fc::reflector<T>::visit( from_variant_visitor<T>( vo, o, max_depth ) );
     }
   };

    template<> 
    struct if_enum<fc::true_type> 
    {
       template<typename T>
       static inline void to_variant( const T& o, fc::variant& v, uint32_t max_depth = 1 )
       { 
           v = fc::reflector<T>::to_fc_string(o);
       }
       template<typename T>
       static inline void from_variant( const fc::variant& v, T& o, uint32_t max_depth = 1 )
       { 
           if( v.is_string() )
              o = fc::reflector<T>::from_string( v.get_string().c_str() );
           else
              o = fc::reflector<T>::from_int( v.as_int64() );
       }
    };


   template<typename T>
   void to_variant( const T& o, variant& v, uint32_t max_depth )
   {
      if_enum<typename fc::reflector<T>::is_enum>::to_variant( o, v, max_depth );
   }

   template<typename T>
   void from_variant( const variant& v, T& o, uint32_t max_depth )
   {
      if_enum<typename fc::reflector<T>::is_enum>::from_variant( v, o, max_depth );
   }

}
