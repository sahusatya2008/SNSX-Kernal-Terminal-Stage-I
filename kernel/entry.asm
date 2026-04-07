[bits 64]

section .text.entry
global kernel_entry

extern kmain
extern __bss_start
extern __bss_end

kernel_entry:
    mov rsp, 0x000000000009F000
    mov rbp, rsp
    mov rbx, rdi

    mov rdi, __bss_start
    mov rcx, __bss_end
    sub rcx, rdi
    xor rax, rax
    rep stosb

    mov rdi, rbx
    call kmain

.halt:
    hlt
    jmp .halt
