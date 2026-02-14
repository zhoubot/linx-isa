	.file	"pto_tmatmul_acc.cpp"
	.text
	.globl	pto_tmatmul_acc_i32_8x8         #  -- Begin function pto_tmatmul_acc_i32_8x8
	.p2align	1
	.type	pto_tmatmul_acc_i32_8x8,@function
pto_tmatmul_acc_i32_8x8:                #  @pto_tmatmul_acc_i32_8x8
#  %bb.0:
FENTRY	[ra ~ ra], sp!, 8
#  %bb.1:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a0],[]
B.IOTI	[], last	->t<4KB>
#  %bb.3:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a1],[]
B.IOTI	[], last	->t<4KB>
#  %bb.4:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[t#1, t#2], last	->acc<4KB>
#  %bb.7:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.5:
BSTART.CUBE	MAMULB.ACC, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[t#1, t#2], last	->acc<4KB>
#  %bb.8:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.6:
BSTART.TMA	TSTORE, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a2],[]
B.IOTI	[m#1], last	->t<4KB>
#  %bb.2:
FRET.STK	[ra ~ ra], sp!, 8
.Lfunc_end0:
	.size	pto_tmatmul_acc_i32_8x8, .Lfunc_end0-pto_tmatmul_acc_i32_8x8
                                        #  -- End function
	.ident	"clang version 23.0.0git (git@github.com:zhoubot/llvm-project.git cfc6dd5711dcb22eac664da55e1a011c1a49b548)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
