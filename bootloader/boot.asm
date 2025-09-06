[BITS 16]
[ORG 0x7C00]

cli                 ; disable interrupts
xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7C00      ; stack

; -------------------------
; Print loading message
; -------------------------
mov si, msg_loading
call print_string

; -------------------------
; Disk Address Packet for LBA
; -------------------------
align 4
dap:
    db 0x10        ; size = 16 bytes
    db 0x00        ; reserved
    dw 60          ; number of sectors to read
    dw 0x0000      ; buffer offset 
    dw 0x1000      ; buffer segment 
    dq 1           ; starting LBA

; -------------------------
; Load kernel from disk into memory (16-bit real mode)
; -------------------------
mov ah, 0x42       
mov dl, 0x80          ; first hard disk
mov si, dap
int 0x13
jc disk_error         ; jump if error

mov si, msg_loaded
call print_string

; -------------------------
; Load GDT
; -------------------------
lgdt [gdt_descriptor]

; Enable protected mode
mov eax, cr0
or eax, 1           ; set PE bit
mov cr0, eax

; Far jump to flush prefetch queue
jmp 0x08:protected_mode_entry

; -------------------------
[BITS 32]
protected_mode_entry:
    ; Setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000          ; stack for protected mode
    xor ebp, ebp              ; clear base pointer

    ; Jump to C kernel at 0x10000
    jmp 0x08:0x10000          ; far jump to kernel

; -------------------------
; GDT (kernel, flat memory model)
gdt_start:
    dq 0x0000000000000000      ; null descriptor
    dq 0x00CF9A000000FFFF      ; code segment
    dq 0x00CF92000000FFFF      ; data segment
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; -------------------------
disk_error:
    mov si, msg_disk_error
    call print_string          ; print "Disk read error!"
    jmp $                      ; hang

; -------------------------
; Print string routine (BIOS teletype)
print_string:
    pusha
.next_char:
    lodsb                       ; load next character from DS:SI into AL
    cmp al, 0
    je .done
    mov ah, 0x0E                ; BIOS teletype function
    int 0x10                    ; print character in AL
    jmp .next_char
.done:
    popa
    ret

; -------------------------
msg_loading db "Loading kernel...",13,10,0
msg_disk_error db "Disk read error!",13,10,0
msg_loaded db "Kernel loaded!",13,10,0

; -------------------------
; Bootloader padding & signature
times 510-($-$$) db 0
dw 0xAA55
; -------------------------