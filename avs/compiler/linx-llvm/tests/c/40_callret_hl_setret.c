__asm__(
".text\n"
".globl callret_hl_setret_probe\n"
".type callret_hl_setret_probe,@function\n"
"callret_hl_setret_probe:\n"
"  HL.BSTART.STD CALL, .Lcallret_hl_setret_callee\n"
"  hl.setret .Lcallret_hl_setret_after\n"
"  C.BSTOP\n"
".Lcallret_hl_setret_after:\n"
"  C.BSTART.STD RET\n"
"  c.setc.tgt ra\n"
"  C.BSTOP\n"
".Lcallret_hl_setret_callee:\n"
"  C.BSTART.STD RET\n"
"  c.setc.tgt ra\n"
"  C.BSTOP\n"
".size callret_hl_setret_probe, .-callret_hl_setret_probe\n");

int main(void) { return 0; }
