/*
 * linx-libc: Minimal startup code for LinxISA
 * 
 * The emulator sets up sp automatically.
 * This just calls main() and exit().
 */
	.global	_start
	.type	_start, @function
_start:
	C.BSTART
.L_start_body:
	FENTRY	[ra ~ s2], sp!, 16
	
	/* Call main with argc=0, argv=NULL */
	addi	a0, zero, 0
	addi	a1, zero, 0
	
	/* Call main */
	BSTART	CALL, main
	c.setret	.L_after_main,	->ra
	
.L_after_main:
	/* main() returned; a0 holds the exit code. */
	BSTART	CALL, __linx_exit
	c.setret	.L_after_exit,	->ra

.L_after_exit:
	/* __linx_exit is noreturn, but if it ever returns, halt. */
.L_hang:
	addi	zero, zero, 0
	jr	.L_hang
	.size	_start, .-_start
