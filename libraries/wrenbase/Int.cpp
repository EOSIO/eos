#include <wrenbase/Int.hpp>
#include <Wren++.h>


namespace wrenbase {

   void UInt256::wrenBind() {
      wrenpp::beginModule( "main" )
         .bindClass<UInt256,double>( "UInt256" )
            .bindFunction< decltype( &UInt256::operator+ ), &UInt256::operator+ >( false, "+(_)")
            .bindFunction< decltype( &UInt256::operator- ), &UInt256::operator- >( false, "-(_)")
            .bindFunction< decltype( &UInt256::operator* ), &UInt256::operator* >( false, "*(_)")
            .bindFunction< decltype( &UInt256::operator/ ), &UInt256::operator/ >( false, "/(_)")
            .bindFunction< decltype( &UInt256::operator% ), &UInt256::operator% >( false, "%(_)")
            .bindFunction< decltype( &UInt256::operator< ), &UInt256::operator< >( false, "<(_)")
            .bindFunction< decltype( &UInt256::operator> ), &UInt256::operator> >( false, ">(_)")
            .bindFunction< decltype( &UInt256::operator! ), &UInt256::operator! >( false, "!")
            .bindFunction< decltype( &UInt256::operator~ ), &UInt256::operator~ >( false, "~")
            .bindFunction< decltype( &UInt256::operator<= ), &UInt256::operator<= >( false, "<=(_)")
            .bindFunction< decltype( &UInt256::operator>= ), &UInt256::operator>= >( false, ">=(_)")
            .bindFunction< decltype( &UInt256::operator== ), &UInt256::operator== >( false, "==(_)")
            .bindFunction< decltype( &UInt256::operator!= ), &UInt256::operator!= >( false, "!=(_)")
            .bindFunction< decltype( &UInt256::operator| ), &UInt256::operator| >( false, "|(_)")
            .bindFunction< decltype( &UInt256::operator& ), &UInt256::operator| >( false, "&(_)")
            .bindFunction< decltype( &UInt256::operator^ ), &UInt256::operator| >( false, "^(_)")
            .bindFunction< decltype( &UInt256::fromString ), &UInt256::fromString >( true, "fromString(_)")
            .bindFunction< decltype( &UInt256::fromDouble ), &UInt256::fromDouble >( true, "fromDouble(_)")
            .bindFunction< decltype( &UInt256::toHex ), &UInt256::toHex >( false, "toHex()")
            .bindFunction< decltype( &UInt256::toBinary ), &UInt256::toBinary >( false, "toBinary()")
            .bindFunction< decltype( &UInt256::toString ), &UInt256::toString >( false, "toString()")
            .bindFunction< decltype( &UInt256::toDouble ), &UInt256::toDouble >( false, "toDouble()")
            .bindFunction< decltype( &UInt256::test ), &UInt256::test >( false, "test()")
         .endClass()
      .endModule();
   }

   std::string UInt256::wrenScript() {
      return  R"(
      foreign class UInt256 {
         construct new(v) {}
         foreign test()
         foreign ~
         foreign !
         foreign &(v)
         foreign |(v)
         foreign ^(v)
         foreign +(v)
         foreign -(v)
         foreign *(v)
         foreign /(v)
         foreign %(v)
         foreign <(v)
         foreign >(v)
         foreign <=(v)
         foreign >=(v)
         foreign ==(v)
         foreign !=(v)
         foreign static fromString(s)
         foreign static fromDouble(d)
         foreign toBinary()
         foreign toHex()
         foreign toString()
         foreign toDouble()
      }

      )";  // Wren UInt256
   }

} // namespace wrenbase
