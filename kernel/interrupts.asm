[bits 64]

section .text
global idt_load
global interrupts_enable
global interrupts_disable
global default_interrupt_stub
global timer_interrupt_stub
global keyboard_interrupt_stub

extern interrupt_default_handler
extern pit_irq_handler
extern keyboard_irq_handler

idt_load:
    lidt [rdi]
    ret

interrupts_enable:
    sti
    ret

interrupts_disable:
    cli
    ret

%macro PUSH_ALL 0
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro POP_ALL 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
%endmacro

default_interrupt_stub:
    cld
    PUSH_ALL
    call interrupt_default_handler
    POP_ALL
    iretq

timer_interrupt_stub:
    cld
    PUSH_ALL
    call pit_irq_handler
    POP_ALL
    iretq

keyboard_interrupt_stub:
    cld
    PUSH_ALL
    call keyboard_irq_handler
    POP_ALL
    iretq
