#pragma once

#include <stdint.h>
#include <setjmp.h>
#include <string.h>

#include <apifiny/chain/webassembly/apifiny-vm-oc/apifiny-vm-oc.h>

#ifdef __clang__
   #define GS_PTR __attribute__((address_space(256)))
#else
   #define GS_PTR __seg_gs
#endif

//This is really rather unfortunate, but on the upside it does allow a static assert to know if
//the values ever slide which would be a PIC breaking event we'd want to know about at compile
//time.
#define EOS_VM_OC_CONTROL_BLOCK_OFFSET (-18944)
#define EOS_VM_OC_MEMORY_STRIDE (UINT64_C(4329598976))

#ifdef __cplusplus
extern "C" {
#endif

int32_t apifiny_vm_oc_grow_memory(int32_t grow, int32_t max);
sigjmp_buf* apifiny_vm_oc_get_jmp_buf();
void* apifiny_vm_oc_get_exception_ptr();
void* apifiny_vm_oc_get_bounce_buffer_list();

#ifdef __cplusplus
}
#endif