	.file	"pto_flash_attention_auto.cpp"
	.text
	.globl	pto_flash_attention_auto_i32    #  -- Begin function pto_flash_attention_auto_i32
	.p2align	1
	.type	pto_flash_attention_auto_i32,@function
pto_flash_attention_auto_i32:           #  @pto_flash_attention_auto_i32
#  %bb.0:
FENTRY	[ra ~ ra], sp!, 16
#  %bb.2:
BSTART	CALL, _ZN3pto4linx9auto_mode26flash_attention_kernel_i32EPKiS3_S3_Pi, ra=.LBB0_1
.LBB0_1:                                #  Label of block must be emitted
FRET.STK	[ra ~ ra], sp!, 16
.Lfunc_end0:
	.size	pto_flash_attention_auto_i32, .Lfunc_end0-pto_flash_attention_auto_i32
                                        #  -- End function
	.section	.text._ZN3pto4linx9auto_mode26flash_attention_kernel_i32EPKiS3_S3_Pi,"axG",@progbits,_ZN3pto4linx9auto_mode26flash_attention_kernel_i32EPKiS3_S3_Pi,comdat
	.weak	_ZN3pto4linx9auto_mode26flash_attention_kernel_i32EPKiS3_S3_Pi #  -- Begin function _ZN3pto4linx9auto_mode26flash_attention_kernel_i32EPKiS3_S3_Pi
	.p2align	1
	.type	_ZN3pto4linx9auto_mode26flash_attention_kernel_i32EPKiS3_S3_Pi,@function
_ZN3pto4linx9auto_mode26flash_attention_kernel_i32EPKiS3_S3_Pi: #  @_ZN3pto4linx9auto_mode26flash_attention_kernel_i32EPKiS3_S3_Pi
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
C.BSTART.STD
hl.addi	a0, 4096,	->a4
#  %bb.4:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a4],[]
B.IOTI	[], last	->t<4KB>
#  %bb.5:
C.BSTART.STD
hl.addi	a0, 8192,	->a4
#  %bb.6:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a4],[]
B.IOTI	[], last	->t<4KB>
#  %bb.7:
C.BSTART.STD
hl.addi	a0, 12288,	->a4
#  %bb.8:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a4],[]
B.IOTI	[], last	->t<4KB>
#  %bb.9:
C.BSTART.STD
hl.addi	a0, 16384,	->a4
#  %bb.10:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a4],[]
B.IOTI	[], last	->t<4KB>
#  %bb.11:
C.BSTART.STD
hl.addi	a1, 16384,	->a4
hl.addi	a1, 12288,	->a5
hl.addi	a1, 8192,	->a6
hl.addi	a1, 4096,	->a7
#  %bb.12:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a1],[]
B.IOTI	[], last	->t<4KB>
#  %bb.13:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a7],[]
B.IOTI	[], last	->t<4KB>
#  %bb.14:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a6],[]
B.IOTI	[], last	->u<4KB>
#  %bb.15:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a5],[]
B.IOTI	[], last	->u<4KB>
#  %bb.16:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a4],[]
B.IOTI	[], last	->u<4KB>
#  %bb.17:
C.BSTART.STD
hl.addi	a2, 12288,	->a4
hl.addi	a2, 8192,	->a5
hl.addi	a2, 4096,	->a6
#  %bb.18:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a2],[]
B.IOTI	[], last	->t<4KB>
#  %bb.19:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a6],[]
B.IOTI	[], last	->u<4KB>
#  %bb.20:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a5],[]
B.IOTI	[], last	->u<4KB>
#  %bb.21:
BSTART.TMA	TLOAD, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a4],[]
B.IOTI	[], last	->u<4KB>
#  %bb.22:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[t#2, t#7], last	->acc<4KB>
#  %bb.57:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.23:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[t#3, t#8], last	->acc<4KB>
#  %bb.58:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.24:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[t#4, u#1], last	->acc<4KB>
#  %bb.59:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.25:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[t#5, u#2], last	->acc<4KB>
#  %bb.60:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.26:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[t#6, u#3], last	->acc<4KB>
#  %bb.61:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.27:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[t#2, t#8], last	->acc<4KB>
#  %bb.62:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.28:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[t#3, u#1], last	->acc<4KB>
#  %bb.63:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.29:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[t#4, u#2], last	->acc<4KB>
#  %bb.64:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.30:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[t#5, u#3], last	->acc<4KB>
#  %bb.65:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->n<4KB>
#  %bb.31:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[m#1, t#1], last	->acc<4KB>
#  %bb.66:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.32:
BSTART.TMA	TSTORE, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a3],[]
B.IOTI	[m#1], last	->t<4KB>
#  %bb.33:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[m#2, u#4], last	->acc<4KB>
#  %bb.67:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.34:
C.BSTART.STD
hl.addi	a3, 4096,	->a2
#  %bb.35:
BSTART.TMA	TSTORE, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a2],[]
B.IOTI	[m#1], last	->t<4KB>
#  %bb.36:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[m#3, u#5], last	->acc<4KB>
#  %bb.68:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.37:
C.BSTART.STD
hl.addi	a3, 8192,	->a2
#  %bb.38:
BSTART.TMA	TSTORE, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a2],[]
B.IOTI	[m#1], last	->t<4KB>
#  %bb.39:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[m#4, u#6], last	->acc<4KB>
#  %bb.69:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.40:
C.BSTART.STD
hl.addi	a3, 12288,	->a2
#  %bb.41:
BSTART.TMA	TSTORE, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a2],[]
B.IOTI	[m#1], last	->t<4KB>
#  %bb.42:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[m#5, t#1], last	->acc<4KB>
#  %bb.70:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.43:
C.BSTART.STD
hl.addi	a3, 16384,	->a2
#  %bb.44:
BSTART.TMA	TSTORE, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a2],[]
B.IOTI	[m#1], last	->t<4KB>
#  %bb.45:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[m#6, u#4], last	->acc<4KB>
#  %bb.71:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.46:
C.BSTART.STD
hl.addi	a3, 20480,	->a2
#  %bb.47:
BSTART.TMA	TSTORE, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a2],[]
B.IOTI	[m#1], last	->t<4KB>
#  %bb.48:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[m#7, u#5], last	->acc<4KB>
#  %bb.72:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.49:
C.BSTART.STD
hl.addi	a3, 24576,	->a2
#  %bb.50:
BSTART.TMA	TSTORE, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a2],[]
B.IOTI	[m#1], last	->t<4KB>
#  %bb.51:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[m#8, u#6], last	->acc<4KB>
#  %bb.73:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.52:
C.BSTART.STD
hl.addi	a3, 28672,	->a2
#  %bb.53:
BSTART.TMA	TSTORE, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a2],[]
B.IOTI	[m#1], last	->t<4KB>
#  %bb.54:
BSTART.CUBE	MAMULB, INT32
C.B.DIMI	8, 	->lb0
C.B.DIMI	8, 	->lb1
C.B.DIMI	8, 	->lb2
B.IOTI	[n#1, t#1], last	->acc<4KB>
#  %bb.74:
BSTART.CUBE	ACCCVT, INT32
B.IOTI	[], last	->m<4KB>
#  %bb.55:
C.BSTART.STD
hl.addi	a3, 32768,	->a2
#  %bb.56:
BSTART.TMA	TSTORE, INT32
C.B.DIMI	0, 	->lb0
C.B.DIMI	0, 	->lb1
B.ARG	NORM.normal
B.IOR	[a2],[]
B.IOTI	[m#1], last	->t<4KB>
#  %bb.2:
FRET.STK	[ra ~ ra], sp!, 8
.Lfunc_end1:
	.size	_ZN3pto4linx9auto_mode26flash_attention_kernel_i32EPKiS3_S3_Pi, .Lfunc_end1-_ZN3pto4linx9auto_mode26flash_attention_kernel_i32EPKiS3_S3_Pi
                                        #  -- End function
	.ident	"clang version 23.0.0git (git@github.com:zhoubot/llvm-project.git cfc6dd5711dcb22eac664da55e1a011c1a49b548)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
