[bits 32]
global gdt_flush

gdt_flush:
    mov eax, [esp + 4] ; pointer to GDT ptr
    lgdt [eax]
    mov ax, 0x10       ; data selector (2nd entry)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:.flush    ; code selector (1st entry)
.flush:
    ret
