[bits 16]
[org 0x7C00]

%ifndef STAGE2_SECTORS
%define STAGE2_SECTORS 16
%endif

%define STAGE2_SEGMENT 0x0800
%define STAGE2_OFFSET  0x0000

jmp short boot_start
nop

boot_drive db 0
loading_msg db "SNSX ExOS boot", 13, 10, 0
error_msg db "Disk read failed", 13, 10, 0

disk_packet:
    db 0x10
    db 0x00
    dw STAGE2_SECTORS
    dw STAGE2_OFFSET
    dw STAGE2_SEGMENT
    dq 0x0000000000000001

boot_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    mov si, loading_msg
    call print_string

    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, disk_packet
    int 0x13
    jc disk_error

    jmp STAGE2_SEGMENT:STAGE2_OFFSET

disk_error:
    mov si, error_msg
    call print_string
    cli
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

times 510 - ($ - $$) db 0
dw 0xAA55
