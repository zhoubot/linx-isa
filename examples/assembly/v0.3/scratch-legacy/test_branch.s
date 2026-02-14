	.file	"test_branch.c"
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
	.globl	test_if                         #  -- Begin function test_if
	.p2align	1
	.type	test_if,@function
test_if:                                #  @test_if
#  %bb.0:                               #  %entry
C.BSTART	RET
.LBB1_0.body:
c.setc.tgt	ra
addw	a0, zero.sw,	->u
c.movi	10,	->t
cmp.lt	t#1, u#1.sw,	->a0
C.BSTOP
.Lfunc_end1:
	.size	test_if, .Lfunc_end1-test_if
                                        #  -- End function
	.globl	test_loop                       #  -- Begin function test_loop
	.p2align	1
	.type	test_loop,@function
test_loop:                              #  @test_loop
#  %bb.0:                               #  %entry
C.BSTART	COND, .LBB2_1
.LBB2_0.body:
addw	a0, zero.sw,	->u
c.movi	1,	->t
setc.lt	u#1, t#1.sw
#  %bb.2:                               #  %for.cond.cleanup.loopexit
C.BSTART	RET
.LBB2_2.body:
c.setc.tgt	ra
c.movi	-1,	->t
addw	a0, t#1.sw,	->a1
c.movi	-2,	->t
addw	a0, t#1.sw,	->t
addi	zero, 32,	->a2
sll	t#1, a2,	->t
srl	t#1, a2,	->u
sll	a1, a2,	->t
srl	t#1, a2,	->t
mul	t#1, u#1,	->t
srli	t#1, 1,	->t
addw	a1, t#1.sw,	->a0
.LBB2_1:
C.BSTART	RET
.LBB2_1.body:
c.setc.tgt	ra
c.movr	zero,	->a0
C.BSTOP
.Lfunc_end2:
	.size	test_loop, .Lfunc_end2-test_loop
                                        #  -- End function
	.globl	_start                          #  -- Begin function _start
	.p2align	1
	.type	_start,@function
_start:                                 #  @_start
#  %bb.0:                               #  %entry
C.BSTART
.LBB3_0.body:
	# APP
ebreak	0

	# NO_APP
C.BSTOP
.Lfunc_end3:
	.size	_start, .Lfunc_end3-_start
                                        #  -- End function
	.ident	"clang version 23.0.0git (git@github.com:zhoubot/llvm-project.git 4350e980591e09399d2b6b463b673c5f976e8691)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
