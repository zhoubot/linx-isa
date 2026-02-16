	.text

	.globl callret_tpl_fret_stk_slot_redirect
	.type callret_tpl_fret_stk_slot_redirect,@function
callret_tpl_fret_stk_slot_redirect:
	# FENTRY [ra ~ ra], sp!, 16
	.4byte 0x04a50041
	c.movr ra, ->a1
	addtpc .Lcallret_tpl_fret_stk_redirect, ->a2
	addi a2, .Lcallret_tpl_fret_stk_redirect, ->a2
	sdi a2, [sp, 8]
	addi zero, 17, ->a0
	# FRET.STK [ra ~ ra], sp!, 16
	.4byte 0x04a53041
.Lcallret_tpl_fret_stk_redirect:
	C.BSTART.STD RET
	addi zero, 34, ->a0
	c.setc.tgt a1
	C.BSTOP
	.size callret_tpl_fret_stk_slot_redirect, .-callret_tpl_fret_stk_slot_redirect

	.globl callret_tpl_fret_ra_slot_redirect
	.type callret_tpl_fret_ra_slot_redirect,@function
callret_tpl_fret_ra_slot_redirect:
	# FENTRY [ra ~ ra], sp!, 16
	.4byte 0x04a50041
	c.movr ra, ->a1
	addtpc .Lcallret_tpl_fret_ra_redirect, ->a2
	addi a2, .Lcallret_tpl_fret_ra_redirect, ->a2
	sdi a2, [sp, 8]
	addi zero, 51, ->a0
	# FRET.RA [ra ~ ra], sp!, 16
	.4byte 0x04a52041
.Lcallret_tpl_fret_ra_redirect:
	C.BSTART.STD RET
	addi zero, 68, ->a0
	c.setc.tgt a1
	C.BSTOP
	.size callret_tpl_fret_ra_slot_redirect, .-callret_tpl_fret_ra_slot_redirect
