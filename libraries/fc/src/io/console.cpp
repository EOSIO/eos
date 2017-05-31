#include <iostream>
#include <string>

namespace fc {

#ifdef WIN32
#include <windows.h>

void set_console_echo( bool enable_echo )
{
   auto stdin_handle = GetStdHandle( STD_INPUT_HANDLE );
   DWORD mode = 0;
   GetConsoleMode( stdin_handle, &mode );
   if( enable_echo )
   {
      SetConsoleMode( stdin_handle, mode | ENABLE_ECHO_INPUT );
   }
   else
   {
      SetConsoleMode( stdin_handle, mode & (~ENABLE_ECHO_INPUT) );
   }
}

#else // NOT WIN32
#include <termios.h>
#include <unistd.h>

void set_console_echo( bool enable_echo )
{
   termios oldt;
   tcgetattr(STDIN_FILENO, &oldt);
   termios newt = oldt;
   if( enable_echo )
   {
      newt.c_lflag |= ECHO;
   }
   else
   {
      newt.c_lflag &= ~ECHO;
   }
   tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

#endif // WIN32

} // namespace fc
