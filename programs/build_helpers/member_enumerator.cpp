/*
 * Copyright (c) 2017, Respective Authors.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Any modified source or binaries are used only with the BitShares network.
 *
 * 2. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 3. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <eos/chain/protocol/protocol.hpp>
#include <eos/chain/producer_object.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/io/json.hpp>
#include <iostream>

using namespace eos::chain;

namespace eos { namespace member_enumerator {

struct class_processor
{
   class_processor( std::map< std::string, std::vector< std::string > >& r ) : result(r) {}

   template< typename T >
   void process_class( const T* dummy );

   template< typename... T >
   void process_class( const static_variant< T... >* dummy );

   template< typename T >
   void process_class( const std::vector< T >* dummy );

   template< typename K, typename V >
   void process_class( const std::map< K, V >* dummy );

   template< typename T >
   void process_class( const fc::flat_set< T >* dummy );

   template< typename K, typename V >
   void process_class( const fc::flat_map< K, V >* dummy );

   template< typename T >
   void process_class( const fc::optional< T >* dummy );

   template< typename T >
   static void process_class( std::map< std::string, std::vector< std::string > >& result );

   std::map< std::string, std::vector< std::string > >& result;
};

template< typename T >
struct member_visitor
{
   member_visitor( class_processor* p ) : proc(p) {}

   template<typename Member, class Class, Member (Class::*member)>
   void operator()( const char* name )const
   {
      members.emplace_back( name );
      proc->process_class( (const Member*) nullptr );
   }

   class_processor* proc;
   mutable std::vector< std::string > members;
};

struct static_variant_visitor
{
   static_variant_visitor( class_processor* p ) : proc(p) {}

   typedef void result_type;

   template<typename T>
   void operator()( const T& element )const
   {
      proc->process_class( (const T*) nullptr );
   }

   class_processor* proc;
};

template< typename... T >
void class_processor::process_class( const static_variant< T... >* dummy )
{
   static_variant<T...> dummy2;
   static_variant_visitor vtor( this );

   for( int w=0; w<dummy2.count(); w++ )
   {
      dummy2.set_which(w);
      dummy2.visit( vtor );
   }
}

template<typename IsReflected=fc::false_type>
struct if_reflected
{
   template< typename T >
   static void process_class( class_processor* proc, const T* dummy )
   {
      std::string tname = fc::get_typename<T>::name();
      std::cerr << "skipping non-reflected class " << tname << std::endl;
   }
};

template<>
struct if_reflected<fc::true_type>
{
   template< typename T >
   static void process_class( class_processor* proc, const T* dummy )
   {
      std::string tname = fc::get_typename<T>::name();
      if( proc->result.find( tname ) != proc->result.end() )
         return;
      ilog( "processing class ${c}", ("c", tname) );
      // need this to keep from recursing on same class
      proc->result.emplace( tname, std::vector< std::string >() );

      member_visitor<T> vtor( proc );
      fc::reflector<T>::visit( vtor );
      ilog( "members of class ${c} are ${m}", ("c", tname)("m", vtor.members) );
      proc->result[tname] = vtor.members;
   }
};

template< typename T >
void class_processor::process_class( const T* dummy )
{
   if_reflected< typename fc::reflector<T>::is_defined >::process_class( this, dummy );
}

template< typename T >
void class_processor::process_class( const std::vector< T >* dummy )
{
   process_class( (T*) nullptr );
}

template< typename K, typename V >
void class_processor::process_class( const std::map< K, V >* dummy )
{
   process_class( (K*) nullptr );
   process_class( (V*) nullptr );
}

template< typename T >
void class_processor::process_class( const fc::flat_set< T >* dummy )
{
   process_class( (T*) nullptr );
}

template< typename K, typename V >
void class_processor::process_class( const fc::flat_map< K, V >* dummy )
{
   process_class( (K*) nullptr );
   process_class( (V*) nullptr );
}

template< typename T >
void class_processor::process_class( const fc::optional< T >* dummy )
{
   process_class( (T*) nullptr );
}

template< typename T >
void class_processor::process_class( std::map< std::string, std::vector< std::string > >& result )
{
   class_processor proc(result);
   proc.process_class( (T*) nullptr );
}

} }

int main( int argc, char** argv )
{
   try
   {
      std::map< std::string, std::vector< std::string > > result;
      eos::member_enumerator::class_processor::process_class<signed_block>(result);
      //eos::member_enumerator::process_class<transfer_operation>(result);

      fc::mutable_variant_object mvo;
      for( const std::pair< std::string, std::vector< std::string > >& e : result )
      {
         variant v;
         to_variant( e.second, v );
         mvo.set( e.first, v );
      }

      std::cout << fc::json::to_string( mvo ) << std::endl;
   }
   catch ( const fc::exception& e )
   {
      edump((e.to_detail_string()));
   }
   return 0;
}
