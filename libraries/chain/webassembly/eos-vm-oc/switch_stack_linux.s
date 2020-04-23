.file	"switch_stack_linux.s"
.text
.globl	eosvmoc_switch_stack
.type	eosvmoc_switch_stack, @function
eosvmoc_switch_stack:
   movq %rsp, -16(%rdi)
   leaq -16(%rdi), %rsp
   movq %rdx, %rdi
   callq *%rsi
   mov (%rsp), %rsp
   retq
.size	eosvmoc_switch_stack, .-eosvmoc_switch_stack
.section	.note.GNU-stack,"",@progbits
