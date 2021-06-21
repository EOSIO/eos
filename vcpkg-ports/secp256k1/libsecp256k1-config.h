#ifndef SECP256K1_BASIC_CONFIG_H
#define SECP256K1_BASIC_CONFIG_H

#define ECMULT_GEN_PREC_BITS 4
#define ECMULT_WINDOW_SIZE 15

#ifdef HAVE_LIBGMP
  #define USE_NUM_GMP 1
  #define USE_FIELD_INV_NUM 1
  #define USE_SCALAR_INV_NUM 1
#else
  #define USE_NUM_NONE 1
  #define USE_FIELD_INV_BUILTIN 1
  #define USE_SCALAR_INV_BUILTIN 1
#endif

//enable asm
#ifdef __x86_64__
  #define USE_ASM_X86_64 1
#endif

#endif /* SECP256K1_BASIC_CONFIG_H */
