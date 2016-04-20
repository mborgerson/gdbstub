;
; Copyright (C) 2016  Matt Borgerson
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License along
; with this program; if not, write to the Free Software Foundation, Inc.,
; 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
;

bits 32

%define NUM_HANDLERS 32

section .data
global dbg_int_handlers

; Generate table of handlers
dbg_int_handlers:
%macro handler_addr 1
	dd dbg_int_handler_%1
%endmacro
%assign i 0
%rep NUM_HANDLERS
	handler_addr i
	%assign i i+1
%endrep

section .text
extern dbg_int_handler

%macro int 1
dbg_int_handler_%1:
	push    0  ; Dummy Error code
	push    %1 ; Interrupt Vector
	jmp     dbg_int_handler_common
%endmacro

%macro inte 1
dbg_int_handler_%1:
	; Error code already on stack
	push    %1 ; Interrupt Vector
	jmp     dbg_int_handler_common
%endmacro

; Generate Interrupt Handlers
%assign i 0
%rep NUM_HANDLERS
	%if (i == 8) || ((i >= 10) && (i <= 14)) || (i == 17)
		inte i
	%else
		int i
	%endif
%assign i i+1
%endrep

; Common Interrupt Handler
dbg_int_handler_common:
	pushad
	push    ds
	push    es
	push    fs
	push    gs
	push    ss
	mov     ebp, esp
	
	; Stack:
	; - EFLAGS
	; - CS
	; - EIP
	; - ERROR CODE
	; - VECTOR
	; - EAX
	; - ECX
	; - EDX
	; - EBX
	; - ESP
	; - EBP
	; - ESI
	; - EDI
	; - DS
	; - ES
	; - FS
	; - GS
	; - SS

	push    ebp
	call    dbg_int_handler

	mov     esp, ebp
	pop     ss
	pop     gs
	pop     fs
	pop     es
	pop     ds
	popad
	add     esp, 8 ; Pop error & vector
	iret
