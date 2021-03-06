;*******************************************************************************
; _____   ____ _______    _______ ____  ______  _____                          *
;|  __ \ / __ \__   __|/\|__   __/ __ \|  ____|/ ____|          Copyright 2008 *
;| |__) | |  | | | |  /  \  | | | |  | | |__  | (___              Daniel Bader *
;|  ___/| |  | | | | / /\ \ | | | |  | |  __|  \___ \           Vincenz Doelle *
;| |    | |__| | | |/ ____ \| | | |__| | |____ ____) |    Johannes Schamburger *
;|_|     \____/  |_/_/    \_\_|  \____/|______|_____/          Dmitriy Traytel *
;                                                                              *
;      Practical Oriented TeAching Tool, Operating (and) Educating System      *
;                                                                              *
;                           www.potatoes-project.tk                            *
;*******************************************************************************/

;@author Dmitriy Traytel

[GLOBAL mutex_lock]
[GLOBAL mutex_unlock]
mutex_lock:
        cli ;instead of tsl (no tsl in nasm!)
        mov eax, [esp+4]
        mov ebx, [eax]
        mov dword [eax], 1
        sti
        cmp dword ebx, 0
        jne do_loop
        ret

do_loop:
        hlt
        jmp mutex_lock

mutex_unlock:
        mov dword eax,[esp+4]
        mov dword [eax], 0
        ret

[GLOBAL set_interrupts]
set_interrupts:
        sti
        ret
;********************************************************************************************
[GLOBAL clear_interrupts]
clear_interrupts:
        cli
        ret
;********************************************************************************************
[GLOBAL outb]
[GLOBAL inb]

outb:
        mov eax, [esp+8] ;value
        mov edx, [esp+4] ;target address
        out dx, al
        ret

inb:
        mov edx, [esp+4] ;source address
        in al, dx
        ret
;********************************************************************************************
[GLOBAL repinsw]

repinsw:
        cld
        mov ecx, [esp+12] ;value
        mov edi, [esp+8] ;dest buffer
        mov edx, [esp+4] ;source address
        rep insw
        ret

;********************************************************************************************
[GLOBAL repoutsw]

repoutsw:
        cld
        mov ecx, [esp+12] ;value
        mov esi, [esp+8] ;source buffer
        mov edx, [esp+4] ;dest address
        rep outsw
        ret
