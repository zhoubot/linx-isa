	.file	"test_csel.c"
	.text
	.globl	_start                          #  -- Begin function _start
	.p2align	1
	.type	_start,@function
_start:                                 #  @_start
#  %bb.0:                               #  %entry
C.BSTART
.LBB0_0.body:
subi	sp, 12,	->sp
c.movr	zero,	->t
c.swi	t#1, [sp, 8]
addiw	zero, 42,	->a0
swi	a0, [sp, 4]
swi	a0, [sp, 0]
c.lwi	[sp, 4],	->t
addw	t#1, zero.sw,	->u
c.lwi	[sp, 0],	->t
addw	t#1, zero.sw,	->t
cmp.eq	u#1, t#1.sw,	->a0
addiw	zero, 200,	->u
addiw	zero, 100,	->t
csel	t#1, u#1.sw,	->t
c.swi	t#1, [sp, 8]
addi	zero, 256,	->u
c.lwi	[sp, 8],	->t
c.swi	t#1, [u#1, 0]
	# APP
ebreak	0

	# NO_APP
C.BSTOP
.Lfunc_end0:
	.size	_start, .Lfunc_end0-_start
                                        #  -- End function
	.ident	"clang version 23.0.0git (git@github.com:zhoubot/llvm-project.git 4350e980591e09399d2b6b463b673c5f976e8691)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
