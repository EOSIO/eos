#pragma once

#include <eosio/chain/types.hpp>
#include <fc/io/raw.hpp>
#include <softfloat.hpp>

namespace eosio { namespace chain {

   template<typename ...Indices>
   class index_set;

   template<typename Index>
   class index_utils {
      public:
         using index_t = Index;

         template<typename F>
         static void walk( const chainbase::database& db, F function ) {
            auto const& index = db.get_index<Index>().indices();
            const auto& first = index.begin();
            const auto& last = index.end();
            for (auto itr = first; itr != last; ++itr) {
               function(*itr);
            }
         }

         template<typename Secondary, typename F>
         static void walk( const chainbase::database& db, F function ) {
            auto const& index = db.get_index<Index, Secondary>();
            const auto& first = index.begin();
            const auto& last = index.end();
            for (auto itr = first; itr != last; ++itr) {
               function(*itr);
            }
         }

         template<typename Secondary, typename Key, typename F>
         static void walk_range( const chainbase::database& db, const Key& begin_key, const Key& end_key, F function ) {
            const auto& idx = db.get_index<Index, Secondary>();
            auto begin_itr = idx.lower_bound(begin_key);
            auto end_itr = idx.lower_bound(end_key);
            for (auto itr = begin_itr; itr != end_itr; ++itr) {
               function(*itr);
            }
         }

         template<typename Secondary, typename Key>
         static size_t size_range( const chainbase::database& db, const Key& begin_key, const Key& end_key ) {
            const auto& idx = db.get_index<Index, Secondary>();
            auto begin_itr = idx.lower_bound(begin_key);
            auto end_itr = idx.lower_bound(end_key);
            size_t res = 0;
            while (begin_itr != end_itr) {
               res++; ++begin_itr;
            }
            return res;
         }

         template<typename F>
         static void create( chainbase::database& db, F cons ) {
            db.create<typename index_t::value_type>(cons);
         }
   };

   template<typename Index>
   class index_set<Index> {
   public:
      static void add_indices( chainbase::database& db ) {
         db.add_index<Index>();
      }

      template<typename F>
      static void walk_indices( F function ) {
         function( index_utils<Index>() );
      }
   };

   template<typename FirstIndex, typename ...RemainingIndices>
   class index_set<FirstIndex, RemainingIndices...> {
   public:
      static void add_indices( chainbase::database& db ) {
         index_set<FirstIndex>::add_indices(db);
         index_set<RemainingIndices...>::add_indices(db);
      }

      template<typename F>
      static void walk_indices( F function ) {
         index_set<FirstIndex>::walk_indices(function);
         index_set<RemainingIndices...>::walk_indices(function);
      }
   };

   template<typename DataStream>
   DataStream& operator << ( DataStream& ds, const shared_blob& b ) {
      fc::raw::pack(ds, static_cast<const shared_string&>(b));
      return ds;
   }

   template<typename DataStream>
   DataStream& operator >> ( DataStream& ds, shared_blob& b ) {
      fc::raw::unpack(ds, static_cast<shared_string &>(b));
      return ds;
   }
} }

namespace fc {

   // overloads for to/from_variant
   template<typename OidType>
   void to_variant( const chainbase::oid<OidType>& oid, variant& v ) {
      v = variant(oid._id);
   }

   template<typename OidType>
   void from_variant( const variant& v, chainbase::oid<OidType>& oid ) {
      from_variant(v, oid._id);
   }

   inline
   void float64_to_double (const float64_t& f, double& d) {
      memcpy(&d, &f, sizeof(d));
   }

   inline
   void double_to_float64 (const double& d, float64_t& f) {
      memcpy(&f, &d, sizeof(f));
   }

   inline
   void float128_to_uint128 (const float128_t& f, eosio::chain::uint128_t& u) {
      memcpy(&u, &f, sizeof(u));
   }

   inline
   void uint128_to_float128 (const eosio::chain::uint128_t& u,  float128_t& f) {
      memcpy(&f, &u, sizeof(f));
   }

   inline
   void to_variant( const float64_t& f, variant& v ) {
      double double_f;
      float64_to_double(f, double_f);
      v = variant(double_f);
   }

   inline
   void from_variant( const variant& v, float64_t& f ) {
      double double_f;
      from_variant(v, double_f);
      double_to_float64(double_f, f);
   }

   inline
   void to_variant( const float128_t& f, variant& v ) {
      // Assumes platform is little endian and hex representation of 128-bit integer is in little endian order.	
      char as_bytes[sizeof(eosio::chain::uint128_t)];
      memcpy(as_bytes, &f, sizeof(as_bytes));
      std::string s = "0x";	
      s.append( to_hex( as_bytes, sizeof(as_bytes) ) );
      v = s;
   }

   inline
   void from_variant( const variant& v, float128_t& f ) {
      // Temporarily hold the binary in uint128_t before casting it to float128_t
      char temp[sizeof(eosio::chain::uint128_t)];
      memset(temp, 0, sizeof(temp));
      auto s = v.as_string();	
      FC_ASSERT( s.size() == 2 + 2 * sizeof(temp) && s.find("0x") == 0,	"Failure in converting hex data into a float128_t");	
      auto sz = from_hex( s.substr(2), temp, sizeof(temp) );
      // Assumes platform is little endian and hex representation of 128-bit integer is in little endian order.	
      FC_ASSERT( sz == sizeof(temp), "Failure in converting hex data into a float128_t" );	
      memcpy(&f, temp, sizeof(f));
   }

   inline
   void to_variant( const eosio::chain::shared_string& s, variant& v ) {
      v = variant(std::string(s.begin(), s.end()));
   }

   inline
   void from_variant( const variant& v, eosio::chain::shared_string& s ) {
      string _s;
      from_variant(v, _s);
      s = eosio::chain::shared_string(_s.begin(), _s.end(), s.get_allocator());
   }

   inline
   void to_variant( const eosio::chain::shared_blob& b, variant& v ) {
      v = variant(base64_encode(b.data(), b.size()));
   }

   inline
   void from_variant( const variant& v, eosio::chain::shared_blob& b ) {
      string _s = base64_decode(v.as_string());
      b = eosio::chain::shared_blob(_s.begin(), _s.end(), b.get_allocator());
   }

   template<typename T>
   void to_variant( const eosio::chain::shared_vector<T>& sv, variant& v ) {
      to_variant(std::vector<T>(sv.begin(), sv.end()), v);
   }

   template<typename T>
   void from_variant( const variant& v, eosio::chain::shared_vector<T>& sv ) {
      std::vector<T> _v;
      from_variant(v, _v);
      sv = eosio::chain::shared_vector<T>(_v.begin(), _v.end(), sv.get_allocator());
   }
}

namespace chainbase {
   // overloads for OID packing
   template<typename DataStream, typename OidType>
   DataStream& operator << ( DataStream& ds, const oid<OidType>& oid ) {
      fc::raw::pack(ds, oid._id);
      return ds;
   }

   template<typename DataStream, typename OidType>
   DataStream& operator >> ( DataStream& ds, oid<OidType>& oid ) {
      fc::raw::unpack(ds, oid._id);
      return ds;
   }
}

// overloads for softfloat packing
template<typename DataStream>
DataStream& operator << ( DataStream& ds, const float64_t& v ) {
   double double_v;
   fc::float64_to_double(v, double_v);
   fc::raw::pack(ds, double_v);
   return ds;
}

template<typename DataStream>
DataStream& operator >> ( DataStream& ds, float64_t& v ) {
   double double_v;
   fc::raw::unpack(ds, double_v);
   fc::double_to_float64(double_v, v);
   return ds;
}

template<typename DataStream>
DataStream& operator << ( DataStream& ds, const float128_t& v ) {
   eosio::chain::uint128_t uint128_v;
   fc::float128_to_uint128(v, uint128_v);
   fc::raw::pack(ds, uint128_v);
   return ds;
}

template<typename DataStream>
DataStream& operator >> ( DataStream& ds, float128_t& v ) {
   eosio::chain::uint128_t uint128_v;
   fc::raw::unpack(ds, uint128_v);
   fc::uint128_to_float128(uint128_v, v);
   return ds;
}
