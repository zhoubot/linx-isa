# Manual assembly test for function call
    .text
    .globl _start
    .type _start,@function

# Simple function that returns its argument
simple_ret:
    C.BSTART    RET
.simple_ret_body:
    c.setc.tgt  ra          # Set return target from RA
    # a0 already has the argument, just return it
    C.BSTOP

# Entry point
    .p2align 1
_start:
    # Call simple_ret with argument 42
    CALL simple_ret         # This is our new fused CALL instruction
    # Continue here after return
.after_call:
    C.BSTART
.after_call_body:
    # a0 should still have 42
    ebreak 0
    C.BSTOP

    .size _start, .-_start
