global syscall_routine
extern c_syscall_routine

section .text
syscall_routine:
    pusha

    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push edi
    push esi
    push edx
    push ecx
    push ebx
    push eax

    add esp, 6*4

    mov [esp + 8*4], eax

    pop gs
    pop fs
    pop es
    pop ds

    popa
    iretd
