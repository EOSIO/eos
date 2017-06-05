#pragma once
#include <boost/multiprecision/cpp_int.hpp>
#include <Wren++.h>
#include <sstream>


struct WrenVM;

namespace wrenbase {
   using namespace boost::multiprecision;
   using std::string;
   using std::vector;

   typedef number<cpp_int_backend<256, 256, unsigned_magnitude, unchecked, void> >     uint256;

   class UInt256 {
      public:
        explicit UInt256(double v):value(v){}
        UInt256(uint256 v=0):value(v){}

        void test( WrenVM& vm ) {
           std::cerr << "WrenVM* => " << &vm <<"\n";
        }

        UInt256 operator & ( const UInt256& other )const  { return value & other.value;  }
        UInt256 operator | ( const UInt256& other )const  { return value | other.value;  }
        UInt256 operator ^ ( const UInt256& other )const  { return value ^ other.value;  }
        UInt256 operator + ( const UInt256& other )const  { return value + other.value;  }
        UInt256 operator - ( const UInt256& other )const  { return value - other.value;  }
        UInt256 operator * ( const UInt256& other )const  { return value * other.value;  }
        UInt256 operator / ( const UInt256& other )const  { return value / other.value;  }
        UInt256 operator % ( const UInt256& other )const  { return value % other.value;  }
        bool    operator < ( const UInt256& other )const  { return value < other.value;  }
        bool    operator > ( const UInt256& other )const  { return value > other.value;  }
        bool    operator <= ( const UInt256& other )const { return value <= other.value; }
        bool    operator >= ( const UInt256& other )const { return value >= other.value; }
        bool    operator == ( const UInt256& other )const { return value == other.value; }
        bool    operator != ( const UInt256& other )const { return value != other.value; }
        UInt256 operator << ( double other )const { return value << int64_t(other); }
        UInt256 operator >> ( double other )const { return value >> int64_t(other); }
        bool    operator ! ()const { return !value; }
        UInt256 operator ~ ()const { return ~value; }

        static UInt256 fromString( string s ) { return uint256(s); }
        static UInt256 fromDouble( double s ) { return uint256(s); }
        string toString()const { return string(value); }
        double toDouble()const { return static_cast<double>(value); }
        vector<char> toBinary()const    {  
           vector<char> tmp(32);
           memcpy( tmp.data(), &value, 32 );
           return tmp;
        }

        string toHex()const    {  
           std::stringstream ss; 
           ss << std::hex << value;
           return ss.str();
        }

        static void        wrenBind();
        static std::string wrenScript();

        uint256 value;
   };



} // namespace wrenbase

