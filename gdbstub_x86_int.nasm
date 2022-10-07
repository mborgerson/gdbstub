;
; Copyright (c) 2016-2022 Matt Borgerson
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
;

bits 32

%define NUM_HANDLERS 32

section .data
global gdb_x86_int_handlers

; Generate table of handlers
gdb_x86_int_handlers:
%macro handler_addr 1
	dd gdb_x86_int_handler_%1
%endmacro
%assign i 0
%rep NUM_HANDLERS
	handler_addr i
	%assign i i+1
%endrep

section .text
extern gdb_x86_int_handler

%macro int 1
gdb_x86_int_handler_%1:
	push    0  ; Dummy Error code
	push    %1 ; Interrupt Vector
	jmp     gdb_x86_int_handler_common
%endmacro

%macro inte 1
gdb_x86_int_handler_%1:
	; Error code already on stack
	push    %1 ; Interrupt Vector
	jmp     gdb_x86_int_handler_common
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
gdb_x86_int_handler_common:
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
	call    gdb_x86_int_handler

	mov     esp, ebp
	pop     ss
	pop     gs
	pop     fs
	pop     es
	pop     ds
	popad
	add     esp, 8 ; Pop error & vector
	iret
