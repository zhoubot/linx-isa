	.file	"pto_tload_store.cpp"
	.text
	.globl	pto_tload_store_i32             #  -- Begin function pto_tload_store_i32
	.p2align	1
	.type	pto_tload_store_i32,@function
pto_tload_store_i32:                    #  @pto_tload_store_i32
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
BSTART.TMA	TSTORE, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a1],[]
B.IOTI	[t#1], last	->t<4KB>
#  %bb.2:
FRET.STK	[ra ~ ra], sp!, 8
.Lfunc_end0:
	.size	pto_tload_store_i32, .Lfunc_end0-pto_tload_store_i32
                                        #  -- End function
	.ident	"clang version 23.0.0git (git@github.com:zhoubot/llvm-project.git cfc6dd5711dcb22eac664da55e1a011c1a49b548)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
