#ifndef CASE
#define CASE 1
#endif

	.text
	.globl _start
	.type _start,@function
_start:
#if CASE == 1
	# CALL target is not a block-start marker (with valid SETRET present).
	BSTART CALL, bad_target, ra=after_bad_call
	C.BSTOP
after_bad_call:
	C.BSTART.STD
	C.BSTOP
bad_target:
	c.movi 1, ->a0
	C.BSTOP
#elif CASE == 2
	# SETRET emitted outside CALL block.
	C.BSTART.STD
	setret bad_ret
	C.BSTOP
bad_ret:
	C.BSTOP
#elif CASE == 3
	# RET block without setc.tgt.
	C.BSTART.STD RET
	C.BSTOP
#elif CASE == 4
	# IND block without setc.tgt.
	C.BSTART.STD IND
	C.BSTOP
#elif CASE == 5
	# ICALL block without setc.tgt.
	C.BSTART.STD ICALL
	setret after_icall
	C.BSTOP
after_icall:
	C.BSTART.STD
	C.BSTOP
#elif CASE == 6
	# CALL returns to a non-marker target via explicit ra= label.
	BSTART CALL, callee, ra=bad_ret
	C.BSTOP
bad_ret:
	c.movi 1, ->a0
	C.BSTOP
callee:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#elif CASE == 7
	# RET jumps to a non-marker target.
	C.BSTART.STD RET
	addtpc bad_ret_target, ->a0
	addi a0, bad_ret_target, ->a0
	c.setc.tgt a0
	C.BSTOP
bad_ret_target:
	c.movi 1, ->a0
	C.BSTOP
#elif CASE == 8
	# IND jumps to a non-marker target.
	C.BSTART.STD IND
	addtpc bad_ind_target, ->a0
	addi a0, bad_ind_target, ->a0
	c.setc.tgt a0
	C.BSTOP
bad_ind_target:
	c.movi 1, ->a0
	C.BSTOP
#elif CASE == 9
	# ICALL jumps to a non-marker target.
	C.BSTART.STD ICALL
	setret after_icall_target
	addtpc bad_icall_target, ->a0
	addi a0, bad_icall_target, ->a0
	c.setc.tgt a0
	C.BSTOP
after_icall_target:
	C.BSTART.STD
	C.BSTOP
bad_icall_target:
	c.movi 1, ->a0
	C.BSTOP
#elif CASE == 10
	# Duplicate SETRET in one CALL header is invalid.
	BSTART CALL, callee, ra=after_call_1
	setret after_call_2
	C.BSTOP
after_call_1:
	C.BSTART.STD
	C.BSTOP
after_call_2:
	C.BSTART.STD
	C.BSTOP
callee:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#elif CASE == 11
	# SETRET in a non-CALL block (after header activity) is invalid.
	C.BSTART.STD
	c.movi 0, ->a0
	setret late_target
	C.BSTOP
late_target:
	C.BSTART.STD
	C.BSTOP
#elif CASE == 12
	# CALL block without SETRET must fault.
	BSTART CALL, callee_missing_setret
	C.BSTOP
callee_missing_setret:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#elif CASE == 13
	# Delayed SETRET inside CALL block is invalid (must be adjacent).
	BSTART CALL, callee
	c.movi 0, ->a0
	setret after_call_delayed
	C.BSTOP
after_call_delayed:
	C.BSTART.STD
	C.BSTOP
callee:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#elif CASE == 14
	# ICALL must place SETRET immediately after BSTART(ICALL).
	C.BSTART.STD ICALL
	c.setc.tgt a0
	setret after_icall_delayed
	C.BSTOP
after_icall_delayed:
	C.BSTART.STD
	C.BSTOP
#elif CASE == 15
	# ICALL block without SETRET must fault as missing-setret.
	C.BSTART.STD ICALL
	C.BSTOP
#elif CASE == 16
	# HL.BSTART CALL without SETRET must fault.
	HL.BSTART.STD CALL, callee_hl_missing_setret
	C.BSTOP
callee_hl_missing_setret:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#elif CASE == 17
	# Delayed SETRET after HL.BSTART CALL is invalid.
	HL.BSTART.STD CALL, callee_hl_delayed_setret
	c.movi 0, ->a0
	setret after_hl_delayed_setret
	C.BSTOP
after_hl_delayed_setret:
	C.BSTART.STD
	C.BSTOP
callee_hl_delayed_setret:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#elif CASE == 18
	# Valid CALL header adjacency + valid RET target setup must not fault.
	BSTART CALL, callee_ok, ra=after_call_ok
	C.BSTOP
after_call_ok:
	C.BSTART	DIRECT, after_call_ok
	C.BSTOP
callee_ok:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#elif CASE == 19
	# Valid ICALL adjacency + setc.tgt target setup must not fault.
	C.BSTART.STD ICALL
	setret after_icall_ok
	addtpc callee_icall_ok, ->a0
	addi a0, callee_icall_ok, ->a0
	c.setc.tgt a0
	C.BSTOP
after_icall_ok:
	C.BSTART	DIRECT, after_icall_ok
	C.BSTOP
callee_icall_ok:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#elif CASE == 20
	# Valid RET block with setc.tgt must not fault.
	C.BSTART.STD RET
	addtpc ret_target_ok, ->a0
	addi a0, ret_target_ok, ->a0
	c.setc.tgt a0
	C.BSTOP
ret_target_ok:
	C.BSTART	DIRECT, ret_target_ok
	C.BSTOP
#elif CASE == 21
	# Valid IND block with setc.tgt must not fault.
	C.BSTART.STD IND
	addtpc ind_target_ok, ->a0
	addi a0, ind_target_ok, ->a0
	c.setc.tgt a0
	C.BSTOP
ind_target_ok:
	C.BSTART	DIRECT, ind_target_ok
	C.BSTOP
#elif CASE == 22
	# Valid HL.BSTART CALL adjacency must not fault.
	HL.BSTART.STD CALL, callee_hl_ok, ra=after_hl_call_ok
	C.BSTOP
after_hl_call_ok:
	C.BSTART	DIRECT, after_hl_call_ok
	C.BSTOP
callee_hl_ok:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#elif CASE == 23
	# HL.SETRET outside CALL/ICALL must fault as invalid sequence.
	C.BSTART.STD
	hl.setret bad_hl_setret_seq
	C.BSTOP
bad_hl_setret_seq:
	C.BSTOP
#elif CASE == 24
	# Valid explicit HL.SETRET adjacency after HL.BSTART CALL must not fault.
	HL.BSTART.STD CALL, callee_hl_setret_ok
	hl.setret after_hl_setret_ok
	C.BSTOP
after_hl_setret_ok:
	C.BSTART	DIRECT, after_hl_setret_ok
	C.BSTOP
callee_hl_setret_ok:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#elif CASE == 25
	# Delayed HL.SETRET after HL.BSTART CALL is invalid (must be adjacent).
	HL.BSTART.STD CALL, callee_hl_setret_delayed
	c.movi 0, ->a0
	hl.setret after_hl_setret_delayed
	C.BSTOP
after_hl_setret_delayed:
	C.BSTART.STD
	C.BSTOP
callee_hl_setret_delayed:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#elif CASE == 26
	# Valid ICALL adjacency with HL.SETRET + setc.tgt must not fault.
	C.BSTART.STD ICALL
	hl.setret after_hl_icall_ok
	addtpc callee_hl_icall_ok, ->a0
	addi a0, callee_hl_icall_ok, ->a0
	c.setc.tgt a0
	C.BSTOP
after_hl_icall_ok:
	C.BSTART	DIRECT, after_hl_icall_ok
	C.BSTOP
callee_hl_icall_ok:
	C.BSTART.STD RET
	c.setc.tgt ra
	C.BSTOP
#else
	C.BSTART.STD
	C.BSTOP
#endif

	.size _start, .-_start
