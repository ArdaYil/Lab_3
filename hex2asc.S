  # hexmain.S
  # Written 2015-09-04 by F Lundevall
  # Copyright abandonded - this file is in the public domain.

	.text
	.globl hex2asc

hex2asc:
	li	a0, 12		# test number (from 0 to 15)
	
	addi    sp,sp,-4
	sw      ra,0(sp)
	
	jal	hexasc		# call hexasc
	
	li	a7, 11	# write a0 to stdout
	ecall

	lw      ra,0(sp)
	addi    sp,sp,4
	jr      ra	

  # You can write your own code for hexasc here
  #

# takes to parameters, a0. In a0 lsb specify a number from 0 through 15 all other bits can be ignored.
# ex 0x00000005 is just 5 and we ignore the rest. In other words the rightmost digit in a hex is what we care about
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
