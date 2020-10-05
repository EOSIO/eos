#include <eosio/chain/webassembly/eos-vm-oc/gs_seg_helpers.h>

#include <asm/prctl.h>
#include <sys/prctl.h>
#include <sys/mman.h>

int arch_prctl(int code, unsigned long* addr);

#define EOSVMOC_MEMORY_PTR_cb_ptr GS_PTR struct eos_vm_oc_control_block* const cb_ptr = ((GS_PTR struct eos_vm_oc_control_block* const)(EOS_VM_OC_CONTROL_BLOCK_OFFSET));

int32_t eos_vm_oc_grow_memory(int32_t grow, int32_t max) {
   EOSVMOC_MEMORY_PTR_cb_ptr;
   uint64_t previous_page_count = cb_ptr->current_linear_memory_pages;
   int32_t grow_amount = grow;
   uint64_t max_pages = max;
   if(max_pages > cb_ptr->max_linear_memory_pages)
      max_pages = cb_ptr->max_linear_memory_pages;
   if(grow == 0)
      return (int32_t)cb_ptr->current_linear_memory_pages;
   if(previous_page_count + grow_amount > max_pages)
      return (int32_t)-1;

   int64_t max_segments = cb_ptr->execution_thread_memory_length / EOS_VM_OC_MEMORY_STRIDE - 1;
   int was_extended = previous_page_count > max_segments;
   int will_be_extended = previous_page_count + grow_amount > max_segments;
   char* extended_memory_start = cb_ptr->full_linear_memory_start + max_segments * 64*1024;
   int64_t gs_diff;
   if(will_be_extended && grow_amount > 0) {
      uint64_t skip = was_extended ? previous_page_count - max_segments : 0;
      gs_diff = was_extended ? 0 : max_segments - previous_page_count;
      mprotect(extended_memory_start + skip * 64*1024, (grow_amount - gs_diff) * 64*1024, PROT_READ | PROT_WRITE);
   } else if (was_extended && grow_amount < 0) {
      uint64_t skip = will_be_extended ? previous_page_count + grow_amount - max_segments : 0;
      gs_diff = will_be_extended ? 0 : previous_page_count + grow_amount - max_segments;
      mprotect(extended_memory_start + skip * 64*1024, (-grow_amount + gs_diff) * 64*1024, PROT_NONE);
   } else {
      gs_diff = grow_amount;
   }

   uint64_t current_gs;
   arch_prctl(ARCH_GET_GS, &current_gs);
   current_gs += gs_diff * EOS_VM_OC_MEMORY_STRIDE;
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
