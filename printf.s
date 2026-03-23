global my_printf

%define printf_reg_ptr rbp + rbx * 8 + 24
 
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

	cmp rax, 6
	jge .glob_cleanup
	lea rsp, [rsp + 6 * 8]

	jmp .finish_cleanup
.glob_cleanup:
	lea rsp, [rsp + rax * 8]

.finish_cleanup:
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

.fmt_loop:
	lea rdi, [rsi - 1]
.gather_buffer:
	inc rdi
	mov al, byte [rdi]

	test al, al
	jz .write_buffer

	cmp al, '%'
	jne .gather_buffer

.write_buffer:
	cmp rdi, rsi
	je .no_write

	push rdi
	
	sub rdi, rsi

	; write (fd, *buf, count)
	mov rax, 1h
	mov rdx, rdi
	mov rdi, 1
	syscall

	pop rsi

.no_write:
	mov al, byte [rsi]
	test al, al
	jz .exit

	; skip %
	inc rsi
	call parse_fmt_internal

	jmp .fmt_loop

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
	mov al, byte [rsi]


switch:
	cmp al, '%'
	je .percent_fmt
	cmp al, 'c'
	je .char_fmt
	cmp al, 's'
	je .string_fmt
	cmp al, 'x'
	je .hex_fmt
	cmp al, 'o'
	je .octal_fmt
	cmp al, 'b'
	je .binary_fmt
	cmp al, 'd'
	je .decimal_fmt
	jmp .unknown_code

.percent_fmt:
	; write (fd, *buf, count)
	mov rax, 1h
	mov rdi, 1
	; rsi is rsi
	mov rdx, 1
	syscall
	inc rsi

	jmp .exit_success

.char_fmt:
	push rsi

	; write (fd, *buf, count)
	mov rax, 1h
	mov rdi, 1
	; in little-endian, points to lowest byte
	lea rsi, [printf_reg_ptr]
	mov rdx, 1
	syscall

	pop rsi

	inc rbx
	inc rsi

	jmp .exit_success

.string_fmt:
	push rsi

	xor rdi, rdi
	mov rsi, [printf_reg_ptr]
	mov rdi, rsi

.roll_string:
	mov al, byte [rdi]
	test al, al
	jz .string_end
	inc rdi
	jmp .roll_string

.string_end:
	; write (fd, *buf, count)
	mov rax, 1h

	sub rdi, rsi
	mov rdx, rdi
	; rsi is rsi :)
	mov rdi, 1
	syscall

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

	mov rax, 1
	mov rdi, 1
	push '-'
	mov rsi, rsp
	mov rdx, 1
	syscall
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

	; write (fd, *buf, count)
	mov rax, 1
	mov rdi, 1
	mov rsi, rsp
	mov rdx, 1
	syscall

	pop rdx

	ret
