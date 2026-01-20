global context_switch

section .text
; void context_switch(uint32_t **old_sp, uint32_t *new_sp)
context_switch:
    push ebp
    mov ebp, esp
    pushad
    mov eax, [ebp+8]    ; pointer to old_sp
    mov [eax], esp      ; store current esp into *old_sp
    mov esp, [ebp+12]   ; load new_sp into esp
    popad
    pop ebp
    ret
