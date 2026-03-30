global my_printf
global my_printf_main_ret
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

	; r9 will be used for counting xmm registers left
	mov r9, rax

	call my_printf_internal

	pop r10

	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop r8
	pop r9

	push rax
	mov rax, [rel my_printf_main_ret WRT ..gottpoff]
	mov [fs:rax], r10
	pop rax
	call printf WRT ..plt
	push rax
	mov rax, [rel my_printf_main_ret WRT ..gottpoff]
	mov r10, [fs:rax]
	pop rax
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

	sub rsp, 16
	movups [rsp], xmm0
	push r12
	push rax

	xor r12, r12

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

	pop rax
	pop r12
	movups xmm0, [rsp]
	add rsp, 16

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

	mov rsi, [printf_reg_ptr]
	call printString

.string_end:
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

	xor r8, r8
	call printDecimalNumber

	pop rsi

	inc rbx
	inc rsi

	jmp .exit_success

.float_fmt:
	mov rdx, 0

	call printfFloatingPoint

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
; Prints 8-byte number from rdi in decimal to fd 1
;
; If you want to pad the number to >=k digs, pass r8 = k (non-zero)
;
; Entry: RDI, R8
; Destroys: RAX, RDX, RCX, R11, RDI
;------------------------------------------------------
printDecimalNumber:
	push rbx

	mov rbx, rdi
	mov r9, 10

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
	div r9

	push rdx
	inc rcx

	jmp .loop_decimal	

.ld_exit:

.loop_decimal_feed:
	cmp rcx, r8
	jge .ldf_exit

	push 0
	inc rcx
	jmp .loop_decimal_feed
	
.ldf_exit:
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

%macro push_xmm 1
	sub rsp, 16
	movups [rsp], %1
%endmacro
%macro pop_xmm 1
	movups %1, [rsp]
	add rsp, 16
%endmacro

;---------------------
; 0 if finite(not nan/inf), 1 otherwise
;---------------------
isfinite:
	mov rdx, qword (-1) >> 1
	and rax, rdx
	mov rdx, 7ffh << 52
	cmp rax, rdx
	jl .finite
	mov rax, 1
	ret

.finite:
	xor rax, rax
	ret

;---------------------
; 0 if nan, 1 otherwise
;---------------------
isinf:
	mov rdx, qword (-1) >> 1
	and rax, rdx
	mov rdx, 7ffh << 52
	cmp rax, rdx
	je .inf
	mov rax, 1
	ret

.inf:
	xor rax, rax
	ret


;-------------------------------------
; Prints the 64-bit floating-point
; Modifies r9 and rbx
; Destroys r8, rax, xmm0
;-------------------------------------
printfFloatingPoint:
	call exchangeABIXMM
	
	movq rax, xmm0

	push rax

	; test for sign
	mov rdx, 1 << 63
	and rax, rdx
	test rax, rax
	jz .fin_sign

	push rsi
	mov al, '-'
	call printChar
	pop rsi

.fin_sign:
	mov rax, [rsp]

	call isfinite
	test rax, rax
	jz .finite

	mov rax, [rsp]

	call isinf
	test rax, rax
	jz .inf	

	push rsi
	lea rsi, [rel nan_string]
	call printString
	pop rsi

	pop rax
	ret

.inf:	
	push rsi
	lea rsi, [rel inf_string]
	call printString
	pop rsi

	pop rax
	ret

.finite:

	push rsi

	push_xmm xmm0
	push_xmm xmm1

	movq rax, xmm0
	mov rdx, ~(1 << 63)
	and rax, rdx
	movq xmm0, rax

	call printfFloatingPointTruncated
	; call printfFloatingPointAdvanced


	pop_xmm xmm1
	pop_xmm xmm0
	pop rsi

	pop rax


	ret

%assign FLOAT_PRECISION 6
%assign FLOAT_MUL 1000000; 10 ** FLOAT_PRECISION

printfFloatingPointTruncated:
	; truncated integer part of double
	cvttsd2si rdi, xmm0
	push rdi
	
	xor r8, r8
	call printDecimalNumber

	mov al, '.'
	call printChar

	pop rdi

	; integer to double
	cvtsi2sd xmm1, rdi
	subsd xmm0, xmm1

	mov rax, FLOAT_MUL
	cvtsi2sd xmm1, rax

	mulsd xmm0, xmm1

	cvttsd2si rdi, xmm0
	mov r8, FLOAT_PRECISION
	call printDecimalNumber

	ret

;----------------------------
; Takes double FP number in xmm0
; Returns binary exp = rdx, normalized fp = xmm0
; https://elixir.bootlin.com/musl/v1.2.6/source/src/math/frexp.c
;----------------------------
fp_exponent:
	movq rdx, xmm0
	shr rdx, 52
	and rdx, 7ffh

	; test for subnormal
	test rdx, rdx
	jnz .exchange_normal_form

	ptest xmm0, xmm0
	jnz .exchange_subnormal

	xor rax, rax
	ret

.exchange_subnormal:
	movq rdx, xmm0
	mov rax, 64
	shl rax, 52
	or rdx, rax
	movq xmm0, rdx
	call fp_exponent

	sub rdx, 64
	ret

.exchange_normal_form:
	cmp rdx, 7ffh
	jne .exchange_nf_nt

	ret

.exchange_nf_nt:
	sub rdx, 3feh
	movq rax, xmm0

	mov rdi, 800fffffffffffffh
	and rax, rdi

	mov rdi, 3fe0000000000000h
	or rax, rdi

	movq xmm0, rax
	ret



printfFloatingPointAdvanced:
	push_xmm xmm1

	; xmm0 = norm fp, rdx = bin exp
	call fp_exponent

	ptest xmm0, xmm0
	jz .exit

	mov rax, FLOAT_MUL
	cvtsi2sd xmm1, rax

	mulsd xmm0, xmm1
	cvttsd2si rax, xmm0
	cvtsi2sd xmm1, rax

	push rdi
	mov rdi, rax
	xor r8, r8
	call printDecimalNumber
	pop rdi

	subsd xmm0, xmm1



.exit:
	pop_xmm xmm1
	ret


;-------------------------------------
; Exchanges printf floating-point to xmm0 register
; Entry: r9, rbx - general
; Modifies r9 and rbx
; Destroys r8, rax, xmm0
;-------------------------------------
exchangeABIXMM:
	test r9, r9
	jz .exchange_from_stack

	dec r9
	
	lea rax, [rel .ex_a]
	lea rax, [rax + r12 * (.ex_b - .ex_a)]
	jmp rax

	align 8

%macro  exchangeXMM 1
	movaps xmm0, xmm%1
	jmp .exit
	align 8
%endmacro

.ex_a:
	exchangeXMM 0
.ex_b:
	exchangeXMM 1
	exchangeXMM 2
	exchangeXMM 3
	exchangeXMM 4
	exchangeXMM 5
	exchangeXMM 6
	exchangeXMM 7

.exchange_from_stack:
	movsd xmm0, [printf_reg_ptr]
	inc rbx

.exit:
	inc r12
	ret


;---------------------------------------------------
; Prints string from rsi ptr to buffer
; Destroys everythin printChar destroys + rsi
;---------------------------------------------------
printString:
.roll_string:
	mov al, byte [rsi]

	test al, al
	jz .exit

	push rsi
	call printChar
	pop rsi

	inc rsi
	jmp .roll_string

.exit:
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
	inf_string: db "inf", 0h
	nan_string: db "nan", 0h
	finite_string: db "finite", 0h
fmt_jump_table:
	dq parse_fmt_internal.unknown_code				; a
	dq parse_fmt_internal.binary_fmt				; b
	dq parse_fmt_internal.char_fmt					; c
	dq parse_fmt_internal.decimal_fmt				; d
	dq parse_fmt_internal.unknown_code				; e
	dq parse_fmt_internal.float_fmt					; f
	times ('o' - 'f' - 1) dq parse_fmt_internal.unknown_code	; ('f', 'o')
	dq parse_fmt_internal.octal_fmt					; o
	times ('s' - 'o' - 1) dq parse_fmt_internal.unknown_code	; ('d', 'o')
	dq parse_fmt_internal.string_fmt				; s
	times ('x' - 's' - 1) dq parse_fmt_internal.unknown_code	; ('s', 'x')
	dq parse_fmt_internal.hex_fmt					; x
	times ('z' - 'x')     dq parse_fmt_internal.unknown_code	; ('x', 'z']


section .tbss
my_printf_main_ret: resq 1

section .bss
my_printf_buffer_size: resq 1
my_printf_buffer: resb (BUFFER_SIZE)
