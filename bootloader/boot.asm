; boot.asm
; Boot sector (512 bytes) koji postavlja GDT i TSS.
; Nasam: nasm -f bin boot.asm -o boot.bin

[ORG 0x7C00]
[BITS 16]

cli
xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7C00

; -------------------------
; Print loading message
; -------------------------
mov si, msg_loading
call print_string

; -------------------------
; Load kernel from disk into memory (16-bit real mode)
; -------------------------
mov bx, 0x1000        ; memory address to load kernel (0x1000)
mov dh, 0             ; head = 0
mov dl, 0x80          ; first hard disk
mov ch, 0             ; cylinder = 0
mov cl, 2             ; start at sector 2 (after boot sector)
mov al, 54            ; number of sectors to read
mov ah, 0x02          ; BIOS read sectors function
int 0x13
jc disk_error         ; jump if error

mov si, msg_loaded
call print_string

; -------------------------
; Load GDT (we create GDT with a zeroed TSS descriptor placeholder)
; We'll fill TSS descriptor later in protected mode.
; -------------------------
lgdt [gdt_descriptor]

; Enable protected mode
mov eax, cr0
or eax, 1           ; set PE bit
mov cr0, eax

; Far jump to flush prefetch queue and switch CS
jmp 0x08:protected_mode_entry

; -------------------------
; 32-bit protected-mode entry
; -------------------------
[BITS 32]
protected_mode_entry:
    ; Setup data segments to kernel data selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000          ; kernel stack (ESP0 will point here)
    xor ebp, ebp

    ; --- Prepare TSS in memory ---
    ; Write esp0 and ss0 into TSS structure so CPU will switch stack to esp0 on ring change.
    ; TSS layout: at offset 4 = ESP0 (dword), at offset 8 = SS0 (word).
    mov dword [tss_entry + 4], 0x00090000   ; ESP0 = 0x90000
    mov word  [tss_entry + 8], 0x0010       ; SS0  = 0x10 (kernel data selector)
    mov word  [tss_entry + 102], 0x0068     ; I/O map base = size of TSS (104 = 0x68)

    ; --- Fill TSS descriptor in GDT (offset 5*8 = 0x28) ---
    ; We left the TSS descriptor zeroed in the GDT before; now compute base and limit and write fields.

        ; Compute base = tss_entry
    mov eax, tss_entry       ; eax = base (32-bit)
    ; Compute limit = tss_end - tss_entry - 1
    mov ebx, tss_end
    sub ebx, eax
    dec ebx                  ; ebx = limit

    ; TSS descriptor offset in GDT:
    ; entries: 0:null,1:kcode,2:kdata,3:ucode,4:udata, 5:tss  -> offset = 5*8 = 0x28
    mov edi, gdt_start
    add edi, 0x28            ; EDI points to TSS descriptor in GDT

    ; Write base low (base & 0xFFFF) at [EDI + 2]
    ; NOTE: AX already contains the low 16 bits of EAX (because we did mov eax, tss_entry),
    ; so we can directly write AX before clobbering it.
    mov [edi + 2], ax

    ; Write limit (low 16) at [EDI + 0]
    mov ax, bx               ; low 16 bits of limit (BX = low 16 of EBX)
    mov [edi + 0], ax

    ; Write base mid (byte) at [EDI + 4] = (base >> 16) & 0xFF
    mov edx, eax
    shr edx, 16
    mov [edi + 4], dl        ; DL = (base >> 16) & 0xFF

    ; Write access byte at [EDI + 5] = 0x89  (present, DPL=0, type=9 = 32-bit TSS (available))
    mov byte [edi + 5], 0x89

    ; Write limit high (4 bits) and flags at [EDI + 6].
    ; We want flags = 0x0 (Granularity=0), so only limit high nibble is stored.
    mov eax, ebx             ; eax = limit
    shr eax, 16
    and al, 0x0F             ; keep low 4 bits => limit high nibble
    mov [edi + 6], al

    ; Write base high (byte) at [EDI + 7] = (base >> 24) & 0xFF
    mov eax, tss_entry
    shr eax, 24
    mov [edi + 7], al


    ; Now load TR with TSS selector (selector = index<<3; index=5 -> 5*8 = 0x28)
    mov ax, 0x28
    ltr ax

    ; Now jump to kernel entry (C kernel) at 0x1000 (kernel loaded earlier)
    jmp 0x08:0x1000

; -------------------------
; GDT (flat memory model) - include placeholder (zeros) for TSS descriptor
; Entries (each 8 bytes):
; 0: null
; 1: kernel code  (0x08)
; 2: kernel data  (0x10)
; 3: user  code   (0x18)
; 4: user  data   (0x20)
; 5: TSS (0x28)   <- placeholder zeros, we'll fill in protected mode
; -------------------------
gdt_start:
    dq 0x0000000000000000      ; 0: null
    dq 0x00CF9A000000FFFF      ; 1: kernel code segment (base=0, limit=4GB, exec/read, DPL=0)
    dq 0x00CF92000000FFFF      ; 2: kernel data segment (base=0, limit=4GB, read/write, DPL=0)
    dq 0x00CFFA000000FFFF      ; 3: user code segment   (DPL=3)
    dq 0x00CFF2000000FFFF      ; 4: user data segment   (DPL=3)
    dq 0x0000000000000000      ; 5: TSS descriptor (8 bytes) -- zero for now, fill later
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; -------------------------
disk_error:
    mov si, msg_disk_error
    call print_string
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
; Align and reserve space for TSS structure (104 bytes for 32-bit TSS)
; We'll place it AFTER the GDT in the boot sector, and we'll reference it via label 'tss_entry'.
align 4
tss_entry:
    times 104 db 0
tss_end:

; -------------------------
; Bootloader padding & signature
times 510-($-$$) db 0
dw 0xAA55
