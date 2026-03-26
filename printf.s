global my_printf
extern printf

%define printf_reg_ptr rbp + rbx * 8 + 24
%define BUFFER_SIZE (256)

section .text
my_printf:
	pop r10

	push r9
	push r8
	push rcx
	push rdx
	push rsi
	push rdi

	push r10

	call my_printf_internal

	pop r10

	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop r8
	pop r9
	xor rax, rax

	; ret

	; push r10 ; ret to caller
	mov [rel main_ret], r10
	call printf WRT ..plt
	mov r10, [rel main_ret]
	push r10
	ret

;--------------------------------------------
; DOES NOT FOLLOW STANDARD ABIs, NEEDS WRAPPER
; INTERNAL FUNCTION
; 
; Arguments are passed purely on stack
; in format: an, an-1, ..., a0, ret
; a0 is format string; an, ..., a1 are arguments
; Returns number of arguments read in RAX, including fmt
;
; Registers preserve conventions are same as System_V ABI
;-------------------------------------------
my_printf_internal:
	push rbp
	mov rbp, rsp

	push rbx

	xor rbx, rbx

	; format string
	mov rsi, [printf_reg_ptr]
	inc rbx

.continue_fmt_print:
	mov al, byte [rsi]

	test al, al
	jz .stop_fmt

	cmp al, '%'
	je .parse_fmt

	push rsi
	call printChar
	pop rsi
	inc rsi

	jmp .continue_fmt_print

.parse_fmt:
	; skip %
	inc rsi
	call parse_fmt_internal

	jmp .continue_fmt_print

.stop_fmt:
	call forcePrintBuffer

.exit:
	mov rax, rbx
	pop rbx
	pop rbp
	ret

;--------------------------------------------
; INTERNAL FUNCTION
;
; Accepts format char position in RSI
; Argument index in RBX
; RBP + 16 points to a0 = fmt
;
; Returns parsing status in RAX: 0 on success, non-zero otherwise
; Increments RSI and RBX according to parsed format
;--------------------------------------------
parse_fmt_internal:
	xor rax, rax
	mov al, byte [rsi]
	
	cmp al, '%'
	je .percent_fmt

	cmp al, 'z'
	jg .unknown_code

	cmp al, 'a'
	jl .unknown_code

	; jump table
	lea rdi, [rel fmt_jump_table]
	jmp [rdi + (rax - 'a') * 8]

.percent_fmt:
	push rsi

	mov al, '%'
	call printChar

	pop rsi
	inc rsi

	jmp .exit_success

.char_fmt:
	push rsi
	
	mov al, byte [printf_reg_ptr]
	call printChar

	pop rsi

	inc rbx
	inc rsi

	jmp .exit_success

.string_fmt:
	push rsi

	xor rdi, rdi
	mov rsi, [printf_reg_ptr]

	push r10
	mov r10, rsi

.roll_string:
	mov al, byte [r10]

	test al, al
	jz .string_end

	call printChar

	inc r10
	jmp .roll_string

.string_end:
	pop r10
	pop rsi

	inc rbx
	inc rsi

	jmp .exit_success

.hex_fmt:
	mov r8, 4
	jmp .xob_fmt
.octal_fmt:
	mov r8, 3
	jmp .xob_fmt
.binary_fmt:
	mov r8, 1
.xob_fmt:
	push rsi

	mov rdi, [printf_reg_ptr]	

	call printXOBNumber

	pop rsi

	inc rbx
	inc rsi

	jmp .exit_success

.decimal_fmt:
	push rsi

	mov rdi, [printf_reg_ptr]

	call printDecimalNumber

	pop rsi

	inc rbx
	inc rsi

	jmp .exit_success

.unknown_code:
	mov rax, 1
	ret
	
.exit_success:
	xor rax, rax
	ret


printHexNumber:
	mov r8, 4
	jmp printXOBNumber
printOctalNumber:
	mov r8, 3
	jmp printXOBNumber
printBinaryNumber:
	mov r8, 1
	jmp printXOBNumber

;------------------------------------------------------
; Prints 8-byte number from rdi in hex/octal/binary to fd 1
;
; Pass r8 = 4 for hex; r8 = 3 for octal; r8 = 1 for binary
;
; Entry: RDI, R8
; Destroys: RAX, RDX, RCX, R11, RDI
;------------------------------------------------------
printXOBNumber:
	push rbx

	mov rbx, rdi

	xor rdx, rdx
	mov rax, 64
	div r8

	; (64 / r8) - 1
	mov rcx, rax
	dec rcx
.phn_loop_ror:
	push rcx
	mov cl, r8b
	ror rbx, cl
	pop rcx
	loop .phn_loop_ror

	; (64 / r8)
	mov rcx, rax

 	dec rcx
 
 .phn_skip_zeros:
 	mov rdx, rbx
 
 	push rcx
 
 	mov rsi, 1
 	mov cl, r8b
 	shl rsi, cl
 	dec rsi
 	and rdx, rsi
 
 	test rdx, rdx
 	jnz .stop_skipping_pop
 
 	mov cl, r8b
 	rol rbx, cl
 
 	pop rcx
 	loop .phn_skip_zeros
 
 	jmp .stop_skipping
 
 .stop_skipping_pop:
 	pop rcx
 .stop_skipping:
 	inc rcx

.phn_loop_pr_dg:
	mov rdx, rbx

	push rcx

	mov rsi, 1
	mov cl, r8b
	shl rsi, cl
	dec rsi
	and rdx, rsi

	call printDigit

	mov cl, r8b
	rol rbx, cl

	pop rcx
	loop .phn_loop_pr_dg

	pop rbx
	ret

;------------------------------------------------------
; Prints 8-byte number from rdi in hex/octal/binary to fd 1
;
; Pass r8 = 4 for hex; r8 = 3 for octal; r8 = 1 for binary
;
; Entry: RDI, R8
; Destroys: RAX, RDX, RCX, R11, RDI
;------------------------------------------------------
printDecimalNumber:
	push rbx

	mov rbx, rdi
	mov r8, 10

	cmp rbx, 0
	jge .not_negative

	push rax
	mov al, '-'
	call printChar
	pop rax

	neg rbx

.not_negative:

	mov rax, rbx

	xor rcx, rcx

.loop_decimal:
	test rax, rax
	jz .ld_exit
	xor rdx, rdx
	div r8

	push rdx
	inc rcx

	jmp .loop_decimal

.ld_exit:
	test rcx, rcx
	jnz .loop_print

	push 0
	inc rcx

.loop_print:
	pop rdx
	push rcx
	call printDigit
	pop rcx
	loop .loop_print

	pop rbx
	ret

;------------------------------------------------------
; Prints DL
;
; Entry: RDX
; Destroys: RAX, RDI, RSI, RDX, {RCX, R11 - by syscall}
;------------------------------------------------------
printDigit:	
	cmp dl, 10d
	jl .phd_write_hex_dg
	add dl, 39d

.phd_write_hex_dg:
	add dl, '0'
	
	push rdx

	mov al, dl
	call printChar

	pop rdx

	ret

;------------------------------------------------------
; Prints AL to buffer
; Destroys: RCX, RAX, RDI, RSI, RDX, R11
;------------------------------------------------------
printChar:
	mov rcx, [rel my_printf_buffer_size]	

	cmp rcx, BUFFER_SIZE
	jl .append_buffer	

	push rax
	call forcePrintBuffer
	pop rax

.append_buffer:
	lea rdi, [rel my_printf_buffer]
	mov byte [rdi + rcx], al
	inc rcx
	mov [rel my_printf_buffer_size], rcx

	cmp al, 0x0A
	jne .exit
	call forcePrintBuffer

.exit:
	ret

forcePrintBuffer:
	mov rcx, [rel my_printf_buffer_size]

	; write (fd, *buf, count)
	mov rax, 1
	mov rdi, 1
	lea rsi, [rel my_printf_buffer]
	mov rdx, rcx
	syscall

	xor rcx, rcx
	mov [rel my_printf_buffer_size], rcx
	ret



section .rodata
fmt_jump_table:
	dq parse_fmt_internal.unknown_code				; a
	dq parse_fmt_internal.binary_fmt				; b
	dq parse_fmt_internal.char_fmt					; c
	dq parse_fmt_internal.decimal_fmt				; d
	times ('o' - 'd' - 1) dq parse_fmt_internal.unknown_code	; ('d', 'o')
	dq parse_fmt_internal.octal_fmt					; o
	times ('s' - 'o' - 1) dq parse_fmt_internal.unknown_code	; ('d', 'o')
	dq parse_fmt_internal.string_fmt				; s
	times ('x' - 's' - 1) dq parse_fmt_internal.unknown_code	; ('s', 'x')
	dq parse_fmt_internal.hex_fmt					; x
	times ('z' - 'x')     dq parse_fmt_internal.unknown_code	; ('x', 'z']


section .data
my_printf_buffer_size: dq 0x00
my_printf_buffer:
	times (BUFFER_SIZE) db 0x00
main_ret: dq 0x00
