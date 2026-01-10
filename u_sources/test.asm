.data
.text
main:
	addi $sp, $zero, 1024
	addi $t0, $zero, 10
	sw $t0, -4($sp)
	addi $t1, $zero, 5
	sw $t1, -8($sp)
	lw $t0, -4($sp)
	lw $t1, -8($sp)
	add $t2, $t0, $t1
	sw $t2, -12($sp)
	lw $t0, -12($sp)
	addi $t1, $zero, 0
	beq $t0, $t1, L0
	lw $t2, -4($sp)
	addi $t3, $zero, 1
	add $t4, $t2, $t3
	sw $t4, -16($sp)
	lw $t4, -16($sp)
	sw $t4, -4($sp)
	j L1
L0:
	addi $t0, $zero, 0
	sw $t0, -8($sp)
L1:
L2:
	lw $t0, -8($sp)
	addi $t1, $zero, 0
	beq $t0, $t1, L3
	lw $t0, -8($sp)
	addi $t2, $zero, 1
	sub $t3, $t0, $t2
	sw $t3, -20($sp)
	lw $t3, -20($sp)
	sw $t3, -8($sp)
	j L2
L3:
	lw $t0, -4($sp)
	addi $t1, $zero, 2
	mult $t0, $t1
	mflo $t2
	sw $t2, -24($sp)
	lw $t2, -24($sp)
	sw $t2, -4($sp)
	lw $t3, -8($sp)
	addi $t4, $zero, 3
	div $t3, $t4
	mflo $t5
	sw $t5, -28($sp)
	lw $t5, -28($sp)
	sw $t5, -4($sp)
	lw $t5, -4($sp)
	add $v0, $t5, $zero
Program_End:
	j Program_End
