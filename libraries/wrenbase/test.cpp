#include <wrenbase/Int.hpp>
#include <fc/time.hpp>
#include <fc/log/logger.hpp>

int main( int argc, char** argv ) {
    try {
       auto start = fc::time_point::now();
       wrenbase::UInt256::wrenBind(); 

       for( uint32_t i = 0; i < 100000; ++i ) {
         wrenpp::VM vm;
        // std::cerr <<"vm: " << vm.vm() <<"\n";

         auto script = wrenbase::UInt256::wrenScript();
         script += R"(
        //   System.print( "Hello World" )
       //    System.print( UInt256 )

           var i = UInt256.new(15000)
       //    i.test()
           /*

           System.print( "binary..." )
           System.print( i.toBinary() )
           System.print( i.toBinary().toString )


           System.print( i.toString() )
           System.print( i.toHex() )
           var x = i + UInt256.fromString("2")
           System.print( x.toString() )
           System.print( x )

           var v = UInt256.fromDouble(0)
           while( v < UInt256.fromDouble(1000000) ) {
             i = i + v
             v = v + UInt256.new(1)
           }
           System.print( i.toString() )
           System.print( i.toDouble() )
           */
         )";


         //std::cerr << script << "\n-----------------------------------------------\n";
         vm.executeString( script );
       }
       auto end = fc::time_point::now();
       auto delta = end - start;
       edump( (10000.0 *1000000 / delta.count()) );


   } catch ( const std::exception& e ) {
      std::cerr << "caught exception and exiting cleanly\n" << e.what() <<"\n";
   } catch ( ... ) {
      std::cerr << "caught exception and exiting cleanly\n";
   }
}
