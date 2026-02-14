	.file	"test_branch2.c"
	.text
	.globl	add_func                        #  -- Begin function add_func
	.p2align	1
	.type	add_func,@function
add_func:                               #  @add_func
#  %bb.0:                               #  %entry
C.BSTART	RET
.LBB0_0.body:
c.setc.tgt	ra
addw	a1, a0.sw,	->a0
C.BSTOP
.Lfunc_end0:
	.size	add_func, .Lfunc_end0-add_func
                                        #  -- End function
	.globl	_start                          #  -- Begin function _start
	.p2align	1
	.type	_start,@function
_start:                                 #  @_start
#  %bb.0:                               #  %entry
BSTART	CALL, add_func
.LBB1_0.body:
c.setret	.LBB1_1,	->ra
subi	sp, 16,	->sp
sdi	ra, [sp, 8]
c.movi	5,	->a0
c.movi	3,	->a1
.LBB1_1:                                #  %entry
                                        #  Label of block must be emitted
C.BSTART
.LBB1_1.body:
addtpc	global_result,	->a1
swi	a0, [a1, 0]
lwi	[a1, 0],	->a0
	# APP
ebreak	0

	# NO_APP
C.BSTOP
.Lfunc_end1:
	.size	_start, .Lfunc_end1-_start
                                        #  -- End function
	.type	global_result,@object           #  @global_result
	.bss
	.globl	global_result
	.p2align	2, 0x0
global_result:
	.long	0                               #  0x0
	.size	global_result, 4

	.ident	"clang version 23.0.0git (git@github.com:zhoubot/llvm-project.git 4350e980591e09399d2b6b463b673c5f976e8691)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym global_result
