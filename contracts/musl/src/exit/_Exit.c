#include <stdlib.h>

_Noreturn void _Exit(int ec)
{
   abort();
}
