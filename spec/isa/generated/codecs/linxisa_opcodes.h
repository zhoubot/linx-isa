/* Auto-generated from spec/isa/spec/current/linxisa-v0.3.json. */
/* DO NOT EDIT: run `python3 tools/isa/gen_c_codec.py` to regenerate. */

#pragma once

#include <stddef.h>
#include <stdint.h>

/* A single instruction form (unique encodable bit-pattern). */
typedef struct {
  const char *id;          /* stable identifier */
  const char *mnemonic;    /* draft mnemonic (e.g. 'ADD', 'C.ADD', 'HL.ADDI') */
  const char *asm_fmt;     /* assembly template from the ISA catalog (may be empty) */
  uint16_t length_bits;    /* 16/32/48/64 */
  uint64_t mask;           /* fixed-bit mask over the packed instruction bitvector */
  uint64_t match;          /* fixed-bit value over the packed instruction bitvector */
  uint32_t field_start;    /* index into linxisa_fields[] */
  uint16_t field_count;    /* number of fields for this instruction */
} linxisa_inst_form;

/* A symbolic field (e.g. RegDst, SrcL, simm12, uimm24, ...). */
typedef struct {
  const char *name;
  int8_t signed_hint;      /* -1 unspecified, 0 unsigned, 1 signed */
  uint16_t bit_width;      /* field bit-width */
  uint32_t piece_start;    /* index into linxisa_field_pieces[] */
  uint8_t piece_count;
} linxisa_field;

/* A piece of a field (supports disjoint immediates). */
typedef struct {
  uint8_t insn_lsb;        /* bit position in packed instruction */
  uint8_t width;           /* number of bits */
  uint8_t value_lsb;       /* bit position within the logical field value */
} linxisa_field_piece;

extern const linxisa_inst_form linxisa_inst_forms[];
extern const size_t linxisa_inst_forms_count;
extern const linxisa_field linxisa_fields[];
extern const size_t linxisa_fields_count;
extern const linxisa_field_piece linxisa_field_pieces[];
extern const size_t linxisa_field_pieces_count;

