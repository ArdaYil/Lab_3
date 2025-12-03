# analyze.S
# Written 2015 by F Lundevall
# Ported 2024/07 by W Szczerek (from MIPS to RISC-V)
# Copyright abandonded - this file is in the public domain.

	.text
	.globl analyze
analyze:
	li	s0, 0x30
loop:
	mv	a0, s0				# copy from s0 to a0
	
	li	a7, 11				# environment call with a7 = 11 will print out
	ecall					# one byte from a0 to the Run I/O window

	addi	s0, s0, 0x03	# what happens if the constant is changed?
	
	li	t0, 0x5A	
	ble	s0, t0, loop
    	jr 	ra		

## Note to ourselves
## Line 16 had to be changed because we represent the characters
## with ascii notation which are numbers. 0x30 is 48 in decimal and corresponds to 0
## 0x5A is 90 in decimal and correspond to Z. By incrementing with 1 at line 16
## We print all characters from 48 to 90 and by incrementing by 3 we print every third character			
