
# NOTE (printer conventions):
# - `BSTART` / `C.BSTART` are the default `.STD` forms (older dumps may show `*.STD`).
# - U-hand pushes are printed as `->u`.
# - Destinations use a tab before the arrow for readability: `,\t->dst`.

00000000000a2914 <.LBB301_9>:
   a2914: 3000         	C.BSTART.STD	ICALL
   a2916: 53d6         	c.setret	0xa2934, ->ra <.LBB301_9+0x20>
   a2918: 02b23059     	sdi	a2, [s0, 8]
   a291c: 04000f95     	addi	zero, 64, 	->t
   a2920: c0310049     	sb	t#1, [a0, a1]
   a2924: 00263619     	ldi	[s1, 16], 	->s1
   a2928: 00063f89     	ld	[s1, zero], 	->t
   a292c: 261a         	c.ldi	[t#1, 32], ->t
   a292e: 061c         	c.setc.tgt	t#1
   a2930: 1306         	c.movr	s1, 	->a0
   a2932: 1ac6         	c.movr	s0, 	->a1
   a2934: 00b4         	C.BSTART.STD	COND, 0xa294a <.LBB301_13>
   a2936: 00964f99     	lbui	[s1, 9], 	->t
   a293a: 0c0c2f95     	andi	t#1, 192, 	->t
   a293e: 040c1075     	setc.nei	t#1, 64
   a2942: 0800         	C.BSTART.STD	
   a2944: 0800         	C.BSTART.STD	
   a2946: 40d53041     	FRET.STK	[ra ~ s2], sp!, 256

00000000000a294a <.LBB301_13>:
   a294a: 0800         	C.BSTART.STD	
   a294c: 1306         	c.movr	s1, 	->a0
   a294e: 1ac6         	c.movr	s0, 	->a1
   a2950: 00063f89     	ld	[s1, zero], 	->t
   a2954: 2e1a         	c.ldi	[t#1, 40], ->t
   a2956: a606         	c.movr	t#1, 	->x0
   a2958: 0800         	C.BSTART.STD	
   a295a: 40d51041     	FEXIT	[ra ~ s2], sp!, 256
   a295e: 2800         	C.BSTART.STD	IND
   a2960: 051c         	c.setc.tgt	x0

00000000000a2962 <.LBB301_16>:
   a2962: ffa04711     	BSTART.STD	DIRECT, 0x8aa7e <_ZSt9terminatev>
   a2966: 5056         	c.setret	0xa2968, ->ra <.LBB301_16+0x6>
   a2968: 0000         	C.BSTOP

00000000000a296a <_ZNK12_GLOBAL__N_116itanium_demangle12ModuleEntity11getBaseNameEv>:
   a296a: 2800         	C.BSTART.STD	IND
   a296c: 189a         	c.ldi	[a0, 24], ->t
   a296e: 1606         	c.movr	t#1, 	->a0
   a2970: 000c3f89     	ld	[t#1, zero], 	->t
   a2974: 361a         	c.ldi	[t#1, 48], ->t
   a2976: 061c         	c.setc.tgt	t#1
   a2978: 0000         	C.BSTOP

00000000000a297a <_ZN12_GLOBAL__N_116itanium_demangle12ModuleEntityD0Ev>:
   a297a: 001a4891     	BSTART.STD	DIRECT, 0xa929c <_ZdlPv>
   a297e: 0000         	C.BSTOP

00000000000a2980 <_ZNK12_GLOBAL__N_116itanium_demangle10NestedName9printLeftERNS0_12OutputBufferE>:
   a2980: 0800         	C.BSTART.STD	
   a2982: 40d50041     	FENTRY	[ra ~ s2], sp!, 256
   a2986: 0800         	C.BSTART.STD	
   a2988: 3000         	C.BSTART.STD	ICALL
   a298a: 5296         	c.setret	0xa299e, ->ra <_ZNK12_GLOBAL__N_116itanium_demangle10NestedName9printLeftERNS0_12OutputBufferE+0x1e>
   a298c: 58c6         	c.movr	a1, 	->s0
   a298e: 6086         	c.movr	a0, 	->s1
   a2990: 00263699     	ldi	[s1, 16], 	->s2
   a2994: 0006bf89     	ld	[s2, zero], 	->t
   a2998: 261a         	c.ldi	[t#1, 32], ->t
   a299a: 061c         	c.setc.tgt	t#1
   a299c: 1346         	c.movr	s2, 	->a0
   a299e: 00f4         	C.BSTART.STD	COND, 0xa29bc <.LBB304_4>
   a29a0: 0096cf99     	lbui	[s2, 9], 	->t
   a29a4: 0c0c2f95     	andi	t#1, 192, 	->t
   a29a8: 040c0075     	setc.eqi	t#1, 64
   a29ac: 3000         	C.BSTART.STD	ICALL
   a29ae: 51d6         	c.setret	0xa29bc, ->ra <.LBB304_4>
   a29b0: 0006bf89     	ld	[s2, zero], 	->t
   a29b4: 2e1a         	c.ldi	[t#1, 40], ->t
   a29b6: 061c         	c.setc.tgt	t#1
   a29b8: 1346         	c.movr	s2, 	->a0
   a29ba: 1ac6         	c.movr	s0, 	->a1

00000000000a29bc <.LBB304_4>:
   a29bc: 01e4         	C.BSTART.STD	COND, 0xa29f8 <.LBB304_8>
   a29be: 0015b199     	ldi	[s0, 8], 	->a1
   a29c2: 10cc         	c.addi	a1, 2, ->t
   a29c4: 0025b119     	ldi	[s0, 16], 	->a0
   a29c8: 01817065     	setc.geu	a0, t#1
   a29cc: 00494911     	BSTART.STD	DIRECT, 0xb4ef0 <realloc>
   a29d0: 5356         	c.setret	0xa29ea, ->ra <.LBB304_4+0x2e>
   a29d2: 3e218f95     	addi	a1, 994, 	->t
   a29d6: 00117f95     	slli	a0, 1, 	->t
   a29da: 018cefc5     	cmp.ltu	t#2, t#1, 	->t
   a29de: c1ac8ff7     	csel	t#1, t#2, t#3, 	->t
   a29e2: 12fa         	c.sdi	t#1, [s0, 16]
   a29e4: 0005b109     	ld	[s0, zero], 	->a0
   a29e8: 1e06         	c.movr	t#1, 	->a1
   a29ea: 0354         	C.BSTART.STD	COND, 0xa2a54 <.LBB304_16>
   a29ec: 00a6         	c.setc.eq	a0, zero
   a29ee: 1005b049     	sd	a0, [s0, zero<<3]
   a29f2: 0062         	C.BSTART.STD	DIRECT, 0xa29fe <.LBB304_9>
   a29f4: 0015b199     	ldi	[s0, 8], 	->a1

00000000000a29f8 <.LBB304_8>:
   a29f8: 0800         	C.BSTART.STD	
   a29fa: 0005b109     	ld	[s0, zero], 	->a0

00000000000a29fe <.LBB304_9>:
   a29fe: 3000         	C.BSTART.STD	ICALL
   a2a00: 54d6         	c.setret	0xa2a26, ->ra <.LBB304_9+0x28>
   a2a02: 1888         	c.add	a0, a1, ->t
   a2a04: 03a00f95     	addi	zero, 58, 	->t
   a2a08: c0310049     	sb	t#1, [a0, a1]
   a2a0c: 039c0059     	sbi	t#1, [t#2, 1]
   a2a10: 0ada         	c.ldi	[s0, 8], ->t
   a2a12: 160c         	c.addi	t#1, 2, ->t
   a2a14: 0afa         	c.sdi	t#1, [s0, 8]
   a2a16: 00363619     	ldi	[s1, 24], 	->s1
   a2a1a: 00063f89     	ld	[s1, zero], 	->t
   a2a1e: 261a         	c.ldi	[t#1, 32], ->t
   a2a20: 061c         	c.setc.tgt	t#1
   a2a22: 1306         	c.movr	s1, 	->a0
   a2a24: 1ac6         	c.movr	s0, 	->a1
   a2a26: 00b4         	C.BSTART.STD	COND, 0xa2a3c <.LBB304_13>
   a2a28: 00964f99     	lbui	[s1, 9], 	->t
   a2a2c: 0c0c2f95     	andi	t#1, 192, 	->t
   a2a30: 040c1075     	setc.nei	t#1, 64
   a2a34: 0800         	C.BSTART.STD	
   a2a36: 0800         	C.BSTART.STD	
   a2a38: 40d53041     	FRET.STK	[ra ~ s2], sp!, 256

00000000000a2a3c <.LBB304_13>:
   a2a3c: 0800         	C.BSTART.STD	
   a2a3e: 1306         	c.movr	s1, 	->a0
   a2a40: 1ac6         	c.movr	s0, 	->a1
   a2a42: 00063f89     	ld	[s1, zero], 	->t
   a2a46: 2e1a         	c.ldi	[t#1, 40], ->t
   a2a48: a606         	c.movr	t#1, 	->x0
   a2a4a: 0800         	C.BSTART.STD	
   a2a4c: 40d51041     	FEXIT	[ra ~ s2], sp!, 256
   a2a50: 2800         	C.BSTART.STD	IND
   a2a52: 051c         	c.setc.tgt	x0

00000000000a2a54 <.LBB304_16>:
   a2a54: ffa00a91     	BSTART.STD	DIRECT, 0x8aa7e <_ZSt9terminatev>
   a2a58: 5056         	c.setret	0xa2a5a, ->ra <.LBB304_16+0x6>
   a2a5a: 0000         	C.BSTOP

00000000000a2a5c <_ZNK12_GLOBAL__N_116itanium_demangle10NestedName11getBaseNameEv>:
   a2a5c: 2800         	C.BSTART.STD	IND
   a2a5e: 189a         	c.ldi	[a0, 24], ->t
   a2a60: 1606         	c.movr	t#1, 	->a0
   a2a62: 000c3f89     	ld	[t#1, zero], 	->t
   a2a66: 361a         	c.ldi	[t#1, 48], ->t
   a2a68: 061c         	c.setc.tgt	t#1
   a2a6a: 0000         	C.BSTOP

00000000000a2a6c <_ZN12_GLOBAL__N_116itanium_demangle10NestedNameD0Ev>:
   a2a6c: 001a0c11     	BSTART.STD	DIRECT, 0xa929c <_ZdlPv>
   a2a70: 0000         	C.BSTOP

00000000000a2a72 <_ZN12_GLOBAL__N_116itanium_demangle19parse_discriminatorEPKcS2_>:
   a2a72: 0494         	C.BSTART.STD	COND, 0xa2b04 <.LBB307_14>
   a2a74: 18a6         	c.setc.eq	a0, a1
   a2a76: 0144         	C.BSTART.STD	COND, 0xa2a9e <.LBB307_5>
   a2a78: 00014209     	lbu	[a0, zero], 	->a2
   a2a7c: 05f21075     	setc.nei	a2, 95
   a2a80: 0424         	C.BSTART.STD	COND, 0xa2b04 <.LBB307_14>
   a2a82: 00110215     	addi	a0, 1, 	->a2
   a2a86: 1926         	c.setc.eq	a2, a1
   a2a88: 0204         	C.BSTART.STD	COND, 0xa2ac8 <.LBB307_9>
   a2a8a: 00024209     	lbu	[a2, zero], 	->a2
   a2a8e: 03021f95     	subi	a2, 48, 	->t
   a2a92: fa56         	c.movi	9, 	->t
   a2a94: 019c6065     	setc.ltu	t#1, t#2
   a2a98: 0362         	C.BSTART.STD	DIRECT, 0xa2b04 <.LBB307_14>
   a2a9a: 00210115     	addi	a0, 2, 	->a0

00000000000a2a9e <.LBB307_5>:
   a2a9e: 0334         	C.BSTART.STD	COND, 0xa2b04 <.LBB307_14>
   a2aa0: 03021f95     	subi	a2, 48, 	->t
   a2aa4: fa56         	c.movi	9, 	->t
   a2aa6: 019c6065     	setc.ltu	t#1, t#2
   a2aaa: 0800         	C.BSTART.STD	
   a2aac: 00110215     	addi	a0, 1, 	->a2

00000000000a2ab0 <.LBB307_7>:
   a2ab0: 0224         	C.BSTART.STD	COND, 0xa2af4 <.LBB307_12>
   a2ab2: 1926         	c.setc.eq	a2, a1
   a2ab4: ffe4         	C.BSTART.STD	COND, 0xa2ab0 <.LBB307_7>
   a2ab6: 00024f89     	lbu	[a2, zero], 	->t
   a2aba: 030c1f95     	subi	t#1, 48, 	->t
   a2abe: 00ac6075     	setc.ltui	t#1, 10
   a2ac2: 00120215     	addi	a2, 1, 	->a2
   a2ac6: 01f2         	C.BSTART.STD	DIRECT, 0xa2b04 <.LBB307_14>

00000000000a2ac8 <.LBB307_9>:
   a2ac8: 01e4         	C.BSTART.STD	COND, 0xa2b04 <.LBB307_14>
   a2aca: 05f21fd5     	cmp.nei	a2, 95, 	->t
   a2ace: 00210215     	addi	a0, 2, 	->a2
   a2ad2: 00320fc5     	cmp.eq	a2, a1, 	->t
   a2ad6: 018cb065     	setc.or	t#2, t#1

00000000000a2ada <.LBB307_10>:
   a2ada: 00f4         	C.BSTART.STD	COND, 0xa2af8 <.LBB307_13>
   a2adc: 00024289     	lbu	[a2, zero], 	->a3
   a2ae0: 03029f95     	subi	a3, 48, 	->t
   a2ae4: fa56         	c.movi	9, 	->t
   a2ae6: 019c6065     	setc.ltu	t#1, t#2
   a2aea: ff84         	C.BSTART.STD	COND, 0xa2ada <.LBB307_10>
   a2aec: 00120215     	addi	a2, 1, 	->a2
   a2af0: 1936         	c.setc.ne	a2, a1
   a2af2: 0092         	C.BSTART.STD	DIRECT, 0xa2b04 <.LBB307_14>

00000000000a2af4 <.LBB307_12>:
   a2af4: 0082         	C.BSTART.STD	DIRECT, 0xa2b04 <.LBB307_14>
   a2af6: 10c6         	c.movr	a1, 	->a0

00000000000a2af8 <.LBB307_13>:
   a2af8: 0800         	C.BSTART.STD	
   a2afa: 05f28fd5     	cmp.eqi	a3, 95, 	->t
   a2afe: 090c         	c.addi	a2, 1, ->t
   a2b00: c82c0177     	csel	t#2, t#1, a0, 	->a0

00000000000a2b04 <.LBB307_14>:
   a2b04: 3800         	C.BSTART.STD	RET
   a2b06: 029c         	c.setc.tgt	ra
   a2b08: 0000         	C.BSTOP

00000000000a2b0a <_ZN12_GLOBAL__N_116itanium_demangle22AbstractManglingParserINS0_14ManglingParserINS_16DefaultAllocatorEEES3_E4makeINS0_8NameTypeEJRA15_KcEEEPNS0_4NodeEDpOT0_>:
   a2b0a: 0800         	C.BSTART.STD	
   a2b0c: 40d50041     	FENTRY	[ra ~ s2], sp!, 256
   a2b10: 0800         	C.BSTART.STD	
   a2b12: 0224         	C.BSTART.STD	COND, 0xa2b56 <.LBB308_5>
   a2b14: 58c6         	c.movr	a1, 	->s0
   a2b16: 6086         	c.movr	a0, 	->s1
   a2b18: 26663699     	ldi	[s1, 4912], 	->s2
   a2b1c: 0016b199     	ldi	[s2, 8], 	->a1
   a2b20: fd019f95     	subi	a1, 4048, 	->t
   a2b24: ff101f95     	subi	zero, 4081, 	->t
   a2b28: 019c6065     	setc.ltu	t#1, t#2
   a2b2c: 0036a111     	BSTART.STD	DIRECT, 0xb05b0 <malloc>
   a2b30: 50d6         	c.setret	0xa2b36, ->ra <_ZN12_GLOBAL__N_116itanium_demangle22AbstractManglingParserINS0_14ManglingParserINS_16DefaultAllocatorEEES3_E4makeINS0_8NameTypeEJRA15_KcEEEPNS0_4NodeEDpOT0_+0x2c>
   a2b32: 00001117     	lui	1, 	->a0
   a2b36: 03c4         	C.BSTART.STD	COND, 0xa2bae <.LBB308_8>
   a2b38: 00a6         	c.setc.eq	a0, zero
   a2b3a: 0800         	C.BSTART.STD	
   a2b3c: 1806         	c.movr	zero, 	->a1
   a2b3e: 00001f97     	lui	1, 	->t
   a2b42: 330c0f95     	addi	t#1, 816, 	->t
   a2b46: c308         	c.add	s1, t#1, ->t
   a2b48: 68013049     	sd	s2, [a0, zero<<3]
   a2b4c: 6886         	c.movr	a0, 	->s2
   a2b4e: 02203059     	sdi	zero, [a0, 8]
   a2b52: 100c3049     	sd	a0, [t#1, zero<<3]

00000000000a2b56 <.LBB308_5>:
   a2b56: 0202ba91     	BSTART.STD	DIRECT, 0x123640 <strlen>
   a2b5a: 5256         	c.setret	0xa2b6c, ->ra <.LBB308_5+0x16>
   a2b5c: 02018f95     	addi	a1, 32, 	->t
   a2b60: 0b7a         	c.sdi	t#1, [s2, 8]
   a2b62: 00368685     	add	s2, a1, 	->s2
   a2b66: 01068615     	addi	s2, 16, 	->s1
   a2b6a: 12c6         	c.movr	s0, 	->a0
   a2b6c: 0800         	C.BSTART.STD	
   a2b6e: f886         	c.movr	a0, 	->t
   a2b70: 1306         	c.movr	s1, 	->a0
   a2b72: 01858f05     	add	s0, t#1, 	->u
   a2b76: 00004f97     	lui	4, 	->t
   a2b7a: 3e0c         	c.addi	t#1, 7, ->t
   a2b7c: 18dc1059     	shi	t#1, [s2, 24]
   a2b80: 01a6cf99     	lbui	[s2, 26], 	->t
   a2b84: 0000ff97     	lui	15, 	->t
   a2b88: 419c2f85     	and	t#1, t#2<<8, 	->t
   a2b8c: 500c3f95     	ori	t#1, 1280, 	->t
   a2b90: 1a2c         	c.srli	t#1, 8, ->t
   a2b92: 34dc0059     	sbi	t#1, [s2, 26]
   a2b96: 0008bf87     	addtpc	139, 	->t
   a2b9a: 426c1f95     	subi	t#1, 1062, 	->t
   a2b9e: 137a         	c.sdi	t#1, [s2, 16]
   a2ba0: 08d5b059     	sdi	s0, [s2, 32]
   a2ba4: 0ade3059     	sdi	u#1, [s2, 40]
   a2ba8: 0800         	C.BSTART.STD	
   a2baa: 40d53041     	FRET.STK	[ra ~ s2], sp!, 256

00000000000a2bae <.LBB308_8>:
   a2bae: ff9fb411     	BSTART.STD	DIRECT, 0x8aa7e <_ZSt9terminatev>
   a2bb2: 5056         	c.setret	0xa2bb4, ->ra <.LBB308_8+0x6>
   a2bb4: 0000         	C.BSTOP

00000000000a2bb6 <_ZN12_GLOBAL__N_116itanium_demangle22AbstractManglingParserINS0_14ManglingParserINS_16DefaultAllocatorEEES3_E4makeINS0_9LocalNameEJRPNS0_4NodeESA_EEES9_DpOT0_>:
   a2bb6: 0800         	C.BSTART.STD	
   a2bb8: 40e50041     	FENTRY	[ra ~ s3], sp!, 256
   a2bbc: 0800         	C.BSTART.STD	
   a2bbe: 0234         	C.BSTART.STD	COND, 0xa2c04 <.LBB309_5>
   a2bc0: 5906         	c.movr	a2, 	->s0
   a2bc2: 60c6         	c.movr	a1, 	->s1
   a2bc4: 6886         	c.movr	a0, 	->s2
   a2bc6: 2666b719     	ldi	[s2, 4912], 	->s3
   a2bca: 00173199     	ldi	[s3, 8], 	->a1
   a2bce: fd019f95     	subi	a1, 4048, 	->t
   a2bd2: ff101f95     	subi	zero, 4081, 	->t
   a2bd6: 019c6065     	setc.ltu	t#1, t#2
   a2bda: 00367591     	BSTART.STD	DIRECT, 0xb05b0 <malloc>
   a2bde: 50d6         	c.setret	0xa2be4, ->ra <_ZN12_GLOBAL__N_116itanium_demangle22AbstractManglingParserINS0_14ManglingParserINS_16DefaultAllocatorEEES3_E4makeINS0_9LocalNameEJRPNS0_4NodeESA_EEES9_DpOT0_+0x2e>
   a2be0: 00001117     	lui	1, 	->a0
   a2be4: 0394         	C.BSTART.STD	COND, 0xa2c56 <.LBB309_7>
   a2be6: 00a6         	c.setc.eq	a0, zero
   a2be8: 0800         	C.BSTART.STD	
   a2bea: 1806         	c.movr	zero, 	->a1
   a2bec: 00001f97     	lui	1, 	->t
   a2bf0: 330c0f95     	addi	t#1, 816, 	->t
   a2bf4: c348         	c.add	s2, t#1, ->t
   a2bf6: 70013049     	sd	s3, [a0, zero<<3]
   a2bfa: 7086         	c.movr	a0, 	->s3
   a2bfc: 02203059     	sdi	zero, [a0, 8]
   a2c00: 100c3049     	sd	a0, [t#1, zero<<3]

00000000000a2c04 <.LBB309_5>:
   a2c04: 0800         	C.BSTART.STD	
   a2c06: 02018f95     	addi	a1, 32, 	->t
   a2c0a: 0bba         	c.sdi	t#1, [s3, 8]
   a2c0c: 00063f09     	ld	[s1, zero], 	->u
   a2c10: 00370f05     	add	s3, a1, 	->u
   a2c14: 010e0115     	addi	u#1, 16, 	->a0
   a2c18: 0005bf09     	ld	[s0, zero], 	->u
   a2c1c: 00004f97     	lui	4, 	->t
   a2c20: 018c0f95     	addi	t#1, 24, 	->t
   a2c24: 19dc1059     	shi	t#1, [u#2, 24]
   a2c28: 01aecf99     	lbui	[u#2, 26], 	->t
   a2c2c: 0000ff97     	lui	15, 	->t
   a2c30: 419c2f85     	and	t#1, t#2<<8, 	->t
   a2c34: 500c3f95     	ori	t#1, 1280, 	->t
   a2c38: 1a2c         	c.srli	t#1, 8, ->t
   a2c3a: 35dc0059     	sbi	t#1, [u#2, 26]
   a2c3e: 0008cf87     	addtpc	140, 	->t
   a2c42: 642c0f95     	addi	t#1, 1602, 	->t
   a2c46: 177a         	c.sdi	t#1, [u#2, 16]
   a2c48: 09df3059     	sdi	u#3, [u#2, 32]
   a2c4c: 0bde3059     	sdi	u#1, [u#2, 40]
   a2c50: 0800         	C.BSTART.STD	
   a2c52: 40e53041     	FRET.STK	[ra ~ s3], sp!, 256

00000000000a2c56 <.LBB309_7>:
   a2c56: ff9f8a11     	BSTART.STD	DIRECT, 0x8aa7e <_ZSt9terminatev>
   a2c5a: 5056         	c.setret	0xa2c5c, ->ra <.LBB309_7+0x6>
   a2c5c: 0000         	C.BSTOP

00000000000a2c5e <_ZNK12_GLOBAL__N_116itanium_demangle9LocalName9printLeftERNS0_12OutputBufferE>:
   a2c5e: 0800         	C.BSTART.STD	
   a2c60: 40d50041     	FENTRY	[ra ~ s2], sp!, 256
   a2c64: 0800         	C.BSTART.STD	
   a2c66: 3000         	C.BSTART.STD	ICALL
   a2c68: 5296         	c.setret	0xa2c7c, ->ra <_ZNK12_GLOBAL__N_116itanium_demangle9LocalName9printLeftERNS0_12OutputBufferE+0x1e>
   a2c6a: 58c6         	c.movr	a1, 	->s0
   a2c6c: 6086         	c.movr	a0, 	->s1
   a2c6e: 00263699     	ldi	[s1, 16], 	->s2
   a2c72: 0006bf89     	ld	[s2, zero], 	->t
   a2c76: 261a         	c.ldi	[t#1, 32], ->t
   a2c78: 061c         	c.setc.tgt	t#1
   a2c7a: 1346         	c.movr	s2, 	->a0
   a2c7c: 00f4         	C.BSTART.STD	COND, 0xa2c9a <.LBB310_4>
   a2c7e: 0096cf99     	lbui	[s2, 9], 	->t
   a2c82: 0c0c2f95     	andi	t#1, 192, 	->t
   a2c86: 040c0075     	setc.eqi	t#1, 64
   a2c8a: 3000         	C.BSTART.STD	ICALL
   a2c8c: 51d6         	c.setret	0xa2c9a, ->ra <.LBB310_4>
   a2c8e: 0006bf89     	ld	[s2, zero], 	->t
   a2c92: 2e1a         	c.ldi	[t#1, 40], ->t
   a2c94: 061c         	c.setc.tgt	t#1
   a2c96: 1346         	c.movr	s2, 	->a0
   a2c98: 1ac6         	c.movr	s0, 	->a1

00000000000a2c9a <.LBB310_4>:
   a2c9a: 01e4         	C.BSTART.STD	COND, 0xa2cd6 <.LBB310_8>
   a2c9c: 0015b199     	ldi	[s0, 8], 	->a1
   a2ca0: 10cc         	c.addi	a1, 2, ->t
   a2ca2: 0025b119     	ldi	[s0, 16], 	->a0
   a2ca6: 01817065     	setc.geu	a0, t#1
   a2caa: 00489191     	BSTART.STD	DIRECT, 0xb4ef0 <realloc>
   a2cae: 5356         	c.setret	0xa2cc8, ->ra <.LBB310_4+0x2e>
   a2cb0: 3e218f95     	addi	a1, 994, 	->t
   a2cb4: 00117f95     	slli	a0, 1, 	->t
   a2cb8: 018cefc5     	cmp.ltu	t#2, t#1, 	->t
   a2cbc: c1ac8ff7     	csel	t#1, t#2, t#3, 	->t
   a2cc0: 12fa         	c.sdi	t#1, [s0, 16]
   a2cc2: 0005b109     	ld	[s0, zero], 	->a0
   a2cc6: 1e06         	c.movr	t#1, 	->a1
   a2cc8: 0354         	C.BSTART.STD	COND, 0xa2d32 <.LBB310_16>
   a2cca: 00a6         	c.setc.eq	a0, zero
   a2ccc: 1005b049     	sd	a0, [s0, zero<<3]
   a2cd0: 0062         	C.BSTART.STD	DIRECT, 0xa2cdc <.LBB310_9>
   a2cd2: 0015b199     	ldi	[s0, 8], 	->a1

00000000000a2cd6 <.LBB310_8>:
   a2cd6: 0800         	C.BSTART.STD	
   a2cd8: 0005b109     	ld	[s0, zero], 	->a0

00000000000a2cdc <.LBB310_9>:
   a2cdc: 3000         	C.BSTART.STD	ICALL
   a2cde: 54d6         	c.setret	0xa2d04, ->ra <.LBB310_9+0x28>
   a2ce0: 1888         	c.add	a0, a1, ->t
   a2ce2: 03a00f95     	addi	zero, 58, 	->t
   a2ce6: c0310049     	sb	t#1, [a0, a1]
   a2cea: 039c0059     	sbi	t#1, [t#2, 1]
   a2cee: 0ada         	c.ldi	[s0, 8], ->t
   a2cf0: 160c         	c.addi	t#1, 2, ->t
   a2cf2: 0afa         	c.sdi	t#1, [s0, 8]
   a2cf4: 00363619     	ldi	[s1, 24], 	->s1
   a2cf8: 00063f89     	ld	[s1, zero], 	->t
   a2cfc: 261a         	c.ldi	[t#1, 32], ->t
   a2cfe: 061c         	c.setc.tgt	t#1
   a2d00: 1306         	c.movr	s1, 	->a0
   a2d02: 1ac6         	c.movr	s0, 	->a1
   a2d04: 00b4         	C.BSTART.STD	COND, 0xa2d1a <.LBB310_13>
   a2d06: 00964f99     	lbui	[s1, 9], 	->t
   a2d0a: 0c0c2f95     	andi	t#1, 192, 	->t
   a2d0e: 040c1075     	setc.nei	t#1, 64
   a2d12: 0800         	C.BSTART.STD	
   a2d14: 0800         	C.BSTART.STD	
   a2d16: 40d53041     	FRET.STK	[ra ~ s2], sp!, 256

00000000000a2d1a <.LBB310_13>:
   a2d1a: 0800         	C.BSTART.STD	
   a2d1c: 1306         	c.movr	s1, 	->a0
   a2d1e: 1ac6         	c.movr	s0, 	->a1
   a2d20: 00063f89     	ld	[s1, zero], 	->t
   a2d24: 2e1a         	c.ldi	[t#1, 40], ->t
   a2d26: a606         	c.movr	t#1, 	->x0
   a2d28: 0800         	C.BSTART.STD	
   a2d2a: 40d51041     	FEXIT	[ra ~ s2], sp!, 256
   a2d2e: 2800         	C.BSTART.STD	IND
   a2d30: 051c         	c.setc.tgt	x0

00000000000a2d32 <.LBB310_16>:
   a2d32: ff9f5311     	BSTART.STD	DIRECT, 0x8aa7e <_ZSt9terminatev>
   a2d36: 5056         	c.setret	0xa2d38, ->ra <.LBB310_16+0x6>
   a2d38: 0000         	C.BSTOP

00000000000a2d3a <_ZN12_GLOBAL__N_116itanium_demangle9LocalNameD0Ev>:
   a2d3a: 00195891     	BSTART.STD	DIRECT, 0xa929c <_ZdlPv>
   a2d3e: 0000         	C.BSTOP

00000000000a2d40 <_ZNK12_GLOBAL__N_116itanium_demangle13ParameterPack19hasRHSComponentSlowERNS0_12OutputBufferE>:
   a2d40: 0094         	C.BSTART.STD	COND, 0xa2d52 <.LBB312_2>
   a2d42: 38ca         	c.lwi	[a1, 28], ->t
   a2d44: fffc0075     	setc.eqi	t#1, -1
   a2d48: 00d2         	C.BSTART.STD	DIRECT, 0xa2d62 <.LBB312_3>
   a2d4a: 0061e219     	lwui	[a1, 24], 	->a2
   a2d4e: 00313299     	ldi	[a0, 24], 	->a3

00000000000a2d52 <.LBB312_2>:
   a2d52: 0800         	C.BSTART.STD	
   a2d54: 2006         	c.movr	zero, 	->a2
   a2d56: 00313299     	ldi	[a0, 24], 	->a3
   a2d5a: 0e32a059     	swi	a3, [a1, 28]
   a2d5e: 0c302059     	swi	zero, [a1, 24]

00000000000a2d62 <.LBB312_3>:
   a2d62: 0174         	C.BSTART.STD	COND, 0xa2d90 <.LBB312_6>
   a2d64: 00527065     	setc.geu	a2, a3
   a2d68: 0164         	C.BSTART.STD	COND, 0xa2d94 <.LBB312_7>
   a2d6a: 109a         	c.ldi	[a0, 16], ->t
   a2d6c: 184c3109     	ld	[t#1, a2<<3], 	->a0
   a2d70: 00a14f99     	lbui	[a0, 10], 	->t
   a2d74: 00914f99     	lbui	[a0, 9], 	->t
   a2d78: 419c3205     	or	t#1, t#2<<8, 	->a2
   a2d7c: 0c022f95     	andi	a2, 192, 	->t
   a2d80: 080c1075     	setc.nei	t#1, 128
   a2d84: 2800         	C.BSTART.STD	IND
   a2d86: 00013f89     	ld	[a0, zero], 	->t
   a2d8a: 000c3f89     	ld	[t#1, zero], 	->t
   a2d8e: 061c         	c.setc.tgt	t#1
