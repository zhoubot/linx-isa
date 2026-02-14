	.file	"test_or.c"
	.text
	.globl	_start                          #  -- Begin function _start
	.p2align	1
	.type	_start,@function
_start:                                 #  @_start
#  %bb.0:                               #  %entry
C.BSTART
.LBB0_0.body:
subi	sp, 8,	->sp
hl.lui	65280,	->t
c.swi	t#1, [sp, 4]
addiw	zero, 255,	->t
c.swi	t#1, [sp, 0]
addi	zero, 256,	->u
c.lwi	[sp, 4],	->t
c.swi	t#1, [u#1, 0]
addi	zero, 260,	->u
c.lwi	[sp, 0],	->t
c.swi	t#1, [u#1, 0]
lwi	[sp, 4],	->u
c.lwi	[sp, 0],	->t
orw	t#1, u#1.sw,	->u
addi	zero, 264,	->t
swi	u#1, [t#1, 0]
lwi	[sp, 4],	->u
c.lwi	[sp, 0],	->t
andw	t#1, u#1.sw,	->u
addi	zero, 268,	->t
swi	u#1, [t#1, 0]
lwi	[sp, 4],	->u
c.lwi	[sp, 0],	->t
xorw	t#1, u#1.sw,	->u
addi	zero, 272,	->t
swi	u#1, [t#1, 0]
addi	zero, 276,	->u
hl.lui	65535,	->t
c.swi	t#1, [u#1, 0]
addi	zero, 280,	->u
c.movr	zero,	->t
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
