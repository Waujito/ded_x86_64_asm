global my_strlen

section .text

; STR pointer in RDI
my_strlen:
	xor rax, rax

.test_64byte:
	vmovdqu8 zmm0, [rdi]
	vpxord zmm1, zmm1, zmm1
	vpcmpeqb k1, zmm0, zmm1
	kmovq rsi, k1
	
	test rsi, rsi
	jnz .detect_TERM

	add rax, 64d
	add rdi, 64d
	jmp .test_64byte

.detect_TERM:
	bsf rdx, rsi
	add rax, rdx

	ret

	
	
