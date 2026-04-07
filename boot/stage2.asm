[bits 16]
[org 0x8000]

%ifndef KERNEL_LBA
%define KERNEL_LBA 2
%endif

%ifndef KERNEL_SECTORS
%define KERNEL_SECTORS 1
%endif

%ifndef STAGE2_SECTORS
%define STAGE2_SECTORS 1
%endif

%ifndef APPFS_LBA
%define APPFS_LBA 3
%endif

%ifndef APPFS_SECTORS
%define APPFS_SECTORS 1
%endif

%ifndef MAPPED_MEMORY_MB
%define MAPPED_MEMORY_MB 64
%endif

%define BOOTINFO_MAGIC 0x534E5358
%define BOOT_FLAG_BIOS 0x00000001
%define KERNEL_LOAD_SEG 0x2000
%define KERNEL_LOAD_OFF 0x0000
%define KERNEL_ENTRY    0x00020000

%define CODE32_SEL 0x08
%define DATA32_SEL 0x10
%define CODE64_SEL 0x18
%define DATA64_SEL 0x20

jmp stage2_start

boot_drive db 0

stage2_msg db "SNSX ExOS stage2", 13, 10, 0
load_fail_msg db "Kernel load failed", 13, 10, 0

align 8
boot_info:
    dd BOOTINFO_MAGIC
    dd STAGE2_SECTORS
    dd KERNEL_SECTORS
    dd BOOT_FLAG_BIOS
    db 0x00
    db 0x00
    db 0x00
    db 0x00
    dd APPFS_LBA
    dd APPFS_SECTORS
    dd MAPPED_MEMORY_MB

kernel_packet:
    db 0x10
    db 0x00
    dw KERNEL_SECTORS
    dw KERNEL_LOAD_OFF
    dw KERNEL_LOAD_SEG
    dq KERNEL_LBA

gdt64:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
    dq 0x00AF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt64_end:

gdt_descriptor:
    dw gdt64_end - gdt64 - 1
    dd gdt64

align 4096
pml4_table:
    dq pdpt_table + 0x003
    times 511 dq 0

align 4096
pdpt_table:
    dq pd_table + 0x003
    times 511 dq 0

align 4096
pd_table:
%assign map_index 0
%rep 32
    dq (map_index * 0x200000) + 0x83
%assign map_index map_index + 1
%endrep
    times 480 dq 0

stage2_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7A00

    mov [boot_drive], dl
    mov [boot_info + 16], dl

    mov si, stage2_msg
    call print_string

    call enable_a20
    call load_kernel
    call enter_long_mode

.halt:
    hlt
    jmp .halt

print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    int 0x10
    jmp print_string
.done:
    ret

load_kernel:
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, kernel_packet
    int 0x13
    jc .failed
    ret
.failed:
    mov si, load_fail_msg
    call print_string
    cli
    hlt
    jmp $

enable_a20:
    in al, 0x92
    or al, 0x02
    out 0x92, al
    ret

enter_long_mode:
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax
    jmp CODE32_SEL:protected_mode_entry

[bits 32]
protected_mode_entry:
    mov ax, DATA32_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, pml4_table
    mov cr3, eax

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    jmp CODE64_SEL:long_mode_entry

[bits 64]
long_mode_entry:
    mov ax, DATA64_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rsp, 0x9E000
    mov rbp, rsp

    mov edi, boot_info
    mov rax, KERNEL_ENTRY
    jmp rax
