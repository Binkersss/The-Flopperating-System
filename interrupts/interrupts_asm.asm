BITS 32

section .text

extern interrupts_common

global isr_stub_table

interrupts_common_stub:
    pusha
    cld

    push esp
    call interrupts_common
    add esp, 4

    popa
    add esp, 8
    iret

%macro ISR_NOERR 1
isr_stub_%1:
    push dword 0
    push dword %1
    jmp interrupts_common_stub
%endmacro

%macro ISR_ERR 1
isr_stub_%1:
    push dword %1
    jmp interrupts_common_stub
%endmacro

isr_stub_table:
%assign i 0
%rep 256
    dd isr_stub_%+i
%assign i i+1
%endrep

%assign i 0
%rep 256
    %if (i = 8) || (i = 10) || (i = 11) || (i = 12) || (i = 13) || (i = 14) || (i = 17)
        ISR_ERR i
    %else
        ISR_NOERR i
    %endif
%assign i i+1
%endrep
