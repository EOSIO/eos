#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>

int main() {
   struct itimerval enable = {{0, 0}, {0, 1000u}};
   if(setitimer(ITIMER_REAL, &enable, NULL))
      return 1;
   return 0;
}