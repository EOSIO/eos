#include <stdlib.h>
#include <string>
#include <iostream>

using namespace std;

int main(int , char*[]) {

   constexpr size_t bufsize = 1024;
   string report;
   while ( !cin.eof() ) {
      char buffer[bufsize];
      cin.get(buffer, bufsize);
      buffer[cin.gcount()] = 0;
      string bstr(buffer);
      if (report.empty() ) {
         bstr.erase(0, 1);
      }
      if (cin.eof()) {
         bstr.resize(bstr.size() - 1);
      }
      report += bstr;
   }

   size_t esc = report.find( "\\");
   while( esc != string::npos ) {
      if (report[esc+1] == '"' || report[esc+1] == '\\') {
         report.erase(esc, 1);
      }
      else {
         ++esc;
      }
      esc = report.find("\\",esc+1);
   }

   size_t start = 1;
   size_t end = report.find("\\n", start );
   int linecount = 0;
   while ( end != string::npos ) {
      size_t count = end - start;
      if (count > 0)
         cout << report.substr(start, count) << endl;
      else
         cout << endl;
      start = end + 2;
      end = report.find("\\n", start );
      ++linecount;
   }
   end = report.size() - 1;
   cout << report.substr(start, end) << endl;
   cerr << "report.size = " << report.size() << endl;
   cerr << "line count = " << linecount << endl;

   return 0;
}
