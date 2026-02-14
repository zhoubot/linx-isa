	.file	"test_simple.c"
	.text
	.globl	_start                          #  -- Begin function _start
	.p2align	1
	.type	_start,@function
_start:                                 #  @_start
#  %bb.0:                               #  %entry
C.BSTART
.LBB0_0.body:
subi	sp, 136,	->sp
sdi	s0, [sp, 128]
sdi	s1, [sp, 120]
sdi	s2, [sp, 112]
sdi	s3, [sp, 104]
sdi	s4, [sp, 96]
sdi	s5, [sp, 88]
sdi	s6, [sp, 80]
sdi	s7, [sp, 72]
sdi	s8, [sp, 64]
addiw	zero, 42,	->t
c.swi	t#1, [sp, 60]
c.movi	13,	->t
c.swi	t#1, [sp, 56]
lwi	[sp, 60],	->x0
lwi	[sp, 56],	->s6
lwi	[sp, 60],	->s7
lwi	[sp, 56],	->s8
lwi	[sp, 60],	->s4
lwi	[sp, 56],	->s5
lwi	[sp, 60],	->s2
lwi	[sp, 56],	->s3
lwi	[sp, 60],	->x2
lwi	[sp, 56],	->s0
hl.lui	65280,	->t
c.swi	t#1, [sp, 52]
addiw	zero, 255,	->t
c.swi	t#1, [sp, 48]
lwi	[sp, 52],	->x3
lwi	[sp, 48],	->s1
lwi	[sp, 52],	->a7
lwi	[sp, 48],	->x1
lwi	[sp, 52],	->a5
lwi	[sp, 48],	->a6
c.movi	1,	->a2
swi	a2, [sp, 44]
lwi	[sp, 44],	->a4
c.lwi	[sp, 44],	->t
c.sdi	t#1, [sp, 0]
c.movi	-5,	->t
c.swi	t#1, [sp, 40]
c.lwi	[sp, 60],	->t
c.sdi	t#1, [sp, 16]
c.lwi	[sp, 40],	->t
c.sdi	t#1, [sp, 8]
c.lwi	[sp, 40],	->t
c.sdi	t#1, [sp, 24]
c.movr	zero,	->t
c.swi	t#1, [sp, 36]
c.movi	11,	->a1
c.movr	a2,	->a0
.LBB0_1:                                #  %for.body
                                        #  =>This Inner Loop Header: Depth=1
C.BSTART	COND, .LBB0_1
.LBB0_1.body:
c.lwi	[sp, 36],	->t
addw	a0, t#1.sw,	->t
c.swi	t#1, [sp, 36]
addw	a0, a2.sw,	->a0
addw	a0, zero.sw,	->t
c.setc.ne	t#1, a1
#  %bb.2:                               #  %for.cond.cleanup
C.BSTART
.LBB0_2.body:
addw	s6, x0.sw,	->t
subw	s7, s8.sw,	->t
addw	t#1, zero.sw,	->u
addi	zero, 29,	->t
cmp.eq	u#1, t#1.sw,	->u
addw	t#3, zero.sw,	->t
addi	zero, 55,	->x0
cmp.eq	t#1, x0.sw,	->a0
oriw	a0, 2,	->t
csel	a0, t#1.sw,	->a0
mulw	s5, s4,	->t
addw	t#1, zero.sw,	->u
addi	zero, 546,	->t
cmp.eq	u#1, t#1.sw,	->u
oriw	a0, 4,	->t
csel	a0, t#1.sw,	->a0
divw	s2, s3,	->t
addw	t#1, zero.sw,	->t
c.movi	3,	->a2
cmp.eq	t#1, a2.sw,	->u
oriw	a0, 8,	->t
csel	a0, t#1.sw,	->a0
remw	x2, s0,	->t
addw	t#1, zero.sw,	->t
cmp.eq	t#1, a2.sw,	->u
oriw	a0, 16,	->t
csel	a0, t#1.sw,	->a0
andw	s1, x3.sw,	->t
addw	t#1, zero.sw,	->u
c.movr	zero,	->t
cmp.eq	u#1, t#1.sw,	->u
oriw	a0, 32,	->t
csel	a0, t#1.sw,	->a0
orw	x1, a7.sw,	->t
addw	t#1, zero.sw,	->t
hl.lui	65535,	->a2
cmp.eq	t#1, a2.sw,	->u
oriw	a0, 64,	->t
csel	a0, t#1.sw,	->a0
xorw	a6, a5.sw,	->t
addw	t#1, zero.sw,	->t
cmp.eq	t#1, a2.sw,	->u
oriw	a0, 128,	->t
csel	a0, t#1.sw,	->a0
hl.lui	4194303,	->t
andw	a4, t#1.sw,	->t
addw	t#1, zero.sw,	->t
c.movi	1,	->a2
cmp.eq	t#1, a2.sw,	->u
oriw	a0, 256,	->t
csel	a0, t#1.sw,	->a0
addiw	zero, 4095,	->u
c.ldi	[sp, 0],	->t
andw	t#1, u#1.sw,	->t
addw	t#1, zero.sw,	->t
cmp.eq	t#1, a2.sw,	->u
oriw	a0, 512,	->t
csel	a0, t#1.sw,	->a0
c.ldi	[sp, 16],	->t
addw	t#1, zero.sw,	->u
c.ldi	[sp, 8],	->t
addw	t#1, zero.sw,	->t
cmp.lt	t#1, u#1.sw,	->u
oriw	a0, 1024,	->t
csel	a0, t#1.sw,	->u
c.lwi	[sp, 36],	->t
addw	t#1, zero.sw,	->t
cmp.eq	t#1, x0.sw,	->u
c.ldi	[sp, 24],	->t
srliw	t#1, 20,	->u
addiw	zero, 2048,	->t
andw	u#1, t#1.sw,	->t
orw	u#3, t#1.sw,	->a0
lui	1,	->t
orw	a0, t#1.sw,	->t
csel	a0, t#1.sw,	->a0
addi	zero, 512,	->a1
hl.lui	305419896,	->t
c.swi	t#1, [a1, 0]
c.lwi	[a1, 0],	->t
addw	t#1, zero.sw,	->u
hl.lui	305419896,	->t
cmp.eq	u#1, t#1.sw,	->u
lui	2,	->t
orw	a0, t#1.sw,	->t
csel	a0, t#1.sw,	->u
addi	zero, 256,	->t
swi	u#1, [t#1, 0]
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
