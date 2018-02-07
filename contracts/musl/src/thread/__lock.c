#include "pthread_impl.h"

void __lock(volatile int *l)
{
   *l = 1;
}

void __unlock(volatile int *l)
{
   *l = 0;
}
