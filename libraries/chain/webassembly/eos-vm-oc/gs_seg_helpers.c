#include <eosio/chain/webassembly/eos-vm-oc/gs_seg_helpers.h>

//#include <asm/prctl.h>
//#include <sys/prctl.h>

#if 0
int arch_prctl(int code, unsigned long* addr);

#define EOSVMOC_MEMORY_PTR_cb_ptr GS_PTR struct eos_vm_oc_control_block* const cb_ptr = ((GS_PTR struct eos_vm_oc_control_block* const)(EOS_VM_OC_CONTROL_BLOCK_OFFSET));

int32_t eos_vm_oc_grow_memory(int32_t grow, int32_t max) {
   EOSVMOC_MEMORY_PTR_cb_ptr;
   uint64_t previous_page_count = cb_ptr->current_linear_memory_pages;
   int32_t grow_amount = grow;
   uint64_t max_pages = max;
   if(grow == 0)
      return (int32_t)cb_ptr->current_linear_memory_pages;
   if(previous_page_count + grow_amount > max_pages)
      return (int32_t)-1;

   uint64_t current_gs;
   arch_prctl(ARCH_GET_GS, &current_gs);
   current_gs += grow_amount * EOS_VM_OC_MEMORY_STRIDE;
   arch_prctl(ARCH_SET_GS, (unsigned long*)current_gs);
   cb_ptr->current_linear_memory_pages += grow_amount;
   cb_ptr->first_invalid_memory_address += grow_amount*64*1024;

   if(grow_amount > 0)
      memset(cb_ptr->full_linear_memory_start + previous_page_count*64u*1024u, 0, grow_amount*64u*1024u);

   return (int32_t)previous_page_count;
}

sigjmp_buf* eos_vm_oc_get_jmp_buf() {
   EOSVMOC_MEMORY_PTR_cb_ptr;
   return cb_ptr->jmp;
}

void* eos_vm_oc_get_exception_ptr() {
   EOSVMOC_MEMORY_PTR_cb_ptr;
   return cb_ptr->eptr;
}

void* eos_vm_oc_get_bounce_buffer_list() {
   EOSVMOC_MEMORY_PTR_cb_ptr;
   return cb_ptr->bounce_buffers;
}
#endif