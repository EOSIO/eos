#pragma once

#define a_ll a_ll
static inline int a_ll(volatile int *p)
{
   return *p;
}

#define a_sc a_sc
static inline int a_sc(volatile int *p, int v)
{
   *p = v;
   return 1;
}

#define a_barrier a_barrier
static inline void a_barrier()
{
}
