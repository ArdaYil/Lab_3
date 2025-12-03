# timetemplate.S
# Written 2015 by F Lundevall
# Ported 2024/06 by W Szczerek (from MIPS to RISC-V)
# Copyright abandonded - this file is in the public domain.

#############################################################
# Choose the macro syntax for RARS or DTEK-V board.         #
# The syntax for RARS is probably due to its MIPS heritage. #
#############################################################
###################
# Macros for RARS #
###################
#.macro	PUSH (%reg)
#	addi	sp,sp,-4
#	sw	%reg,0(sp) 
#.end_macro

#.macro	POP (%reg)
#	lw	%reg,0(sp)
#	addi	sp,sp,4
#.end_macro
###################
# Macros for DTEK #
###################
#.macro	PUSH reg
#	addi sp,sp,-4
#	sw \reg,0(sp) 
#.endm

#.macro	POP reg
#	lw	\reg,0(sp)
#	addi	sp,sp,4
#.endm
#############################################################

	.data
	.align 2
mytime:	.word 	0x5957
timstr:	.asciz 	"text more text lots of text\0"
	.text
	.globl timetemplate, tick, time2string

# Function for displaying a string with a newline at the end	
display_string:	
	li a7,4
	ecall
	li a0, 10
	li a7,11
	ecall
	jr ra
	
timetemplate:
	la	a0, timstr
	jal     display_string
	
	# wait a little
	li	a0, 2		# ms
	jal	delay
	
	# call tick
	la 	a0, mytime
	jal	tick
	
	# call your function time2string
	la	a0, timstr
	la	t0, mytime
	lw	a1, 0(t0)
	jal	time2string
	
	# go back and do it all again
	j	timetemplate

	
# tick: update time pointed to by $a0
tick:	lw	t0, 0(a0)	# get time
	addi	t0, t0, 1	# increase
	andi	t1, t0, 0xf	# check lowest digit
	sltiu	t2, t1, 0xa	# if digit < a, okay
	bnez	t2, tiend
	addi	t0, t0, 0x6	# adjust lowest digit
	
	andi	t1, t0, 0xf0	# check next digit
	sltiu	t2, t1, 0x60	# if digit < 6, okay
	bnez	t2, tiend
	addi	t0, t0, 0xa0	# adjust digit
	
	li	t3, 0xF
	slli	t3, t3, 0x8
	and	t1, t0, t3	# check minute digit
	addi	t3, x0, 0xA
	slli	t3, t3, 0x8
	slt	t2, t1, t3	# if digit < a, okay
	bnez	t2, tiend
	addi	t0, t0, 0x600	# adjust digit - this one's okay, it's lower than 0x7FF 
	
	li	t3, 0xF
	slli	t3, t3, 0xC
	and	t1, t0, t3	# check last digit
	addi	t3, x0, 0x6
	slli	t3, t3, 0xC
	slt	t2, t1, t3	# if digit < 6, okay
	bnez	t2, tiend
	
	li	t3, 0xA
	slli	t3, t3, 0xC
	add	t0, t0, t3	# adjust last digit
tiend:	sw	t0,0(a0)	# save updated result
	jr	ra		# return

#########################################################
# Place for your functions: time2string, hex2asc, delay.#
#########################################################

hexasc:
    andi a0, a0, 0xF # keep only the low nibble (0..15)

    li   t0, 10
    bltu a0, t0, .Ldigit  # if (a0 < 10) -> '0'..'9'

    # 10..15 -> 'A'..'F'
    addi a0, a0, 0x37 # 'A' (0x41) - 10 = 0x37
    jr   ra

.Ldigit:
    addi a0, a0, 0x30 # '0' (0x30)
    jr   ra

delay:
    # Prologue: save ra
    addi    sp, sp, -16
    sw      ra, 12(sp)

    # Outer loop: while (ms > 0)
.Louter:
    blez    a0, .Ldone        # if ms <= 0, exit

    addi    a0, a0, -1        # ms = ms - 1

    # Inner loop counter i = 0
    li      t1, 471100          # constant for easy change
    mv      t0, x0            # i = 0

.Linner:
    bge     t0, t1, .Louter   # if i >= 471100, go to outer loop
    addi    t0, t0, 1         # i = i + 1
    j       .Linner           # repeat inner loop

.Ldone:
    # Epilogue: restore ra
    lw      ra, 12(sp)
    addi    sp, sp, 16
    jr      ra

time2string:
    # save ra and s0
    addi    sp, sp, -16
    sw      ra, 12(sp)
    sw      s0, 8(sp)
    mv      s0, a0          # s0 = destination pointer

    # minutes tens (bits 15..12)
    srli    t0, a1, 12
    andi    t0, t0, 0xF
    mv      a0, t0
    jal     hexasc
    sb      a0, 0(s0)
    addi    s0, s0, 1

    # minutes ones (bits 11..8)
    srli    t0, a1, 8
    andi    t0, t0, 0xF
    mv      a0, t0
    jal     hexasc
    sb      a0, 0(s0)
    addi    s0, s0, 1

    # colon ':'
    li      t1, 0x3A
    sb      t1, 0(s0)
    addi    s0, s0, 1

    # seconds tens (bits 7..4)
    srli    t0, a1, 4
    andi    t0, t0, 0xF
    mv      a0, t0
    jal     hexasc
    sb      a0, 0(s0)
    addi    s0, s0, 1

    # seconds ones (bits 3..0)
    andi    t0, a1, 0xF
    mv      a0, t0
    jal     hexasc
    sb      a0, 0(s0)
    addi    s0, s0, 1

    # null terminator
    sb      x0, 0(s0)

    # restore ra and s0
    lw      ra, 12(sp)
    lw      s0, 8(sp)
    addi    sp, sp, 16
    jr      ra
