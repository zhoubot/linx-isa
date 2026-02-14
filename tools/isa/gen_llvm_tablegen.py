#!/usr/bin/env python3
"""
Generate LLVM TableGen instruction definitions from the compiled ISA JSON spec.

This generates TableGen (.td) files that can be used in LLVM backend implementation.
Outputs instruction patterns, register classes, and encoding information.
"""

from __future__ import annotations

import argparse
import json
import re
from collections import defaultdict
from pathlib import Path
from typing import Any, Dict, List, Optional, Set


def _sanitize_identifier(s: str) -> str:
    """Convert a string to a valid TableGen identifier."""
    # Replace invalid chars with underscore
    s = re.sub(r'[^a-zA-Z0-9_]', '_', s)
    # Remove leading digits
    s = re.sub(r'^\d+', '', s)
    # Ensure it starts with a letter
    if s and not s[0].isalpha():
        s = 'Inst_' + s
    return s or 'Inst'


def _get_field_name(field: Dict[str, Any]) -> str:
    """Extract field name from field definition."""
    return str(field.get('name', ''))


def _get_field_bits(field: Dict[str, Any], part_width: int) -> str:
    """Generate TableGen bit range for a field."""
    pieces = field.get('pieces', [])
    if not pieces:
        return ''
    
    # For now, handle simple contiguous fields
    # Multi-piece fields need special handling
    if len(pieces) == 1:
        piece = pieces[0]
        msb = piece.get('insn_msb', 0)
        lsb = piece.get('insn_lsb', 0)
        # TableGen uses [msb:lsb] format, 0-indexed from LSB
        # Need to convert from instruction bit numbering
        width = part_width
        # Convert to TableGen format (LSB=0)
        tablegen_msb = width - 1 - msb
        tablegen_lsb = width - 1 - lsb
        return f'[{tablegen_msb}:{tablegen_lsb}]'
    
    # Multi-piece fields - return first piece for now
    piece = pieces[0]
    msb = piece.get('insn_msb', 0)
    lsb = piece.get('insn_lsb', 0)
    width = part_width
    tablegen_msb = width - 1 - msb
    tablegen_lsb = width - 1 - lsb
    return f'[{tablegen_msb}:{tablegen_lsb}]'


def _is_register_field(field_name: str) -> bool:
    """Check if a field represents a register."""
    reg_fields = {'RegDst', 'SrcL', 'SrcR', 'RegSrc0', 'RegSrc1', 'RegSrc2', 
                  'RegSrc_7_5', 'RegSrc_15_5', 'DstBegin', 'DstEnd'}
    return field_name in reg_fields or 'Reg' in field_name


def _get_operand_type(field: Dict[str, Any], inst: Dict[str, Any]) -> str:
    """Determine TableGen operand type for a field."""
    field_name = _get_field_name(field)
    
    if _is_register_field(field_name):
        return 'GPR'
    
    # Check if signed
    signed = field.get('signed', None)
    if signed is True:
        return 'simm'
    elif signed is False:
        return 'uimm'
    
    # Default based on field name
    if 'simm' in field_name.lower():
        return 'simm'
    elif 'uimm' in field_name.lower() or 'imm' in field_name.lower():
        return 'uimm'
    
    return 'uimm'  # Default


def _generate_instruction_def(inst: Dict[str, Any], inst_index: int) -> str:
    """Generate a single TableGen instruction definition."""
    mnemonic = inst.get('mnemonic', '')
    inst_id = inst.get('id', f'inst_{inst_index}')
    length_bits = inst.get('length_bits', 32)
    group = inst.get('group', '')
    
    # Sanitize identifiers
    def_name = _sanitize_identifier(mnemonic.replace('.', '_'))
    if def_name.startswith('C_'):
        def_name = 'C' + def_name[2:]
    
    encoding = inst.get('encoding', {})
    parts = encoding.get('parts', [])
    
    if not parts:
        return f'// Skipping {mnemonic}: no encoding parts\n'
    
    # Use first part for now (can extend for multi-part)
    part = parts[0]
    mask = part.get('mask', '0x0')
    match = part.get('match', '0x0')
    pattern = part.get('pattern', '')
    fields = part.get('fields', [])
    
    # Build instruction definition
    lines = []
    lines.append(f'def {def_name} : LinxInst<')
    lines.append(f'  (ins'),  # Start operands
    
    # Add operands based on fields
    operands = []
    for field in fields:
        field_name = _get_field_name(field)
        if _is_register_field(field_name):
            operands.append('GPR:$' + field_name.lower())
        else:
            op_type = _get_operand_type(field, inst)
            operands.append(f'{op_type}:${field_name.lower()}')
    
    if operands:
        lines.append('    ' + ', '.join(operands))
    else:
        lines.append('    ')
    
    lines.append('  ),')
    lines.append('  (outs),')  # Outputs - would need semantic analysis
    
    # Add encoding info
    lines.append('  "')
    asm_fmt = inst.get('asm', mnemonic)
    # Replace field references in asm format
    asm_str = asm_fmt
    for field in fields:
        field_name = _get_field_name(field)
        asm_str = asm_str.replace(field_name, f'${{{field_name.lower()}}}')
    lines.append(f'    {asm_str}')
    lines.append('  ",')
    
    # Add pattern bits
    if pattern:
        # Convert pattern to TableGen format
        pattern_td = pattern.replace('.', '?')
        lines.append(f'  [(set (i32 0), (i32 0))],  // TODO: proper pattern')
    
    # Add encoding
    lines.append(f'  [(InstField mask={mask}, value={match})]')
    lines.append('>;')
    lines.append('')
    
    return '\n'.join(lines)


def _group_instructions_by_category(instructions: List[Dict[str, Any]]) -> Dict[str, List[Dict[str, Any]]]:
    """Group instructions by category/group."""
    groups: Dict[str, List[Dict[str, Any]]] = defaultdict(list)
    
    for inst in instructions:
        group = inst.get('group', 'Other')
        groups[group].append(inst)
    
    return groups


def _generate_tablegen_file(spec: Dict[str, Any], output_path: Path) -> None:
    """Generate complete TableGen file."""
    instructions = spec.get('instructions', [])
    
    # Group by category
    groups = _group_instructions_by_category(instructions)
    
    lines = []
    lines.append('// Auto-generated from spec/isa/spec/current/linxisa-v0.3.json')
    lines.append('// DO NOT EDIT: run `python3 tools/isa/gen_llvm_tablegen.py` to regenerate.')
    lines.append('')
    lines.append('// This file contains instruction definitions for LLVM TableGen.')
    lines.append('// Include this file in your LinxInstrInfo.td')
    lines.append('')
    lines.append('// Instruction count: ' + str(len(instructions)))
    lines.append('')
    
    # Generate instruction definitions grouped by category
    for group_name in sorted(groups.keys()):
        group_insts = groups[group_name]
        lines.append(f'// ========================================')
        lines.append(f'// {group_name} ({len(group_insts)} instructions)')
        lines.append(f'// ========================================')
        lines.append('')
        
        for idx, inst in enumerate(group_insts):
            try:
                inst_def = _generate_instruction_def(inst, idx)
                lines.append(inst_def)
            except Exception as e:
                mnemonic = inst.get("mnemonic")
                if not mnemonic:
                    raise ValueError(f"spec entry missing mnemonic: {inst!r}")
                lines.append(f'// Error generating {mnemonic}: {e}')
                lines.append('')
    
    output_path.write_text('\n'.join(lines))
    print(f'Generated {output_path} with {len(instructions)} instructions')


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Generate LLVM TableGen instruction definitions from ISA spec'
    )
    parser.add_argument(
        '--spec',
        type=Path,
        default=Path(__file__).resolve().parents[3] / 'spec/isa/spec/current/linxisa-v0.3.json',
        help='Path to linxisa-v0.3.json'
    )
    parser.add_argument(
        '--out',
        type=Path,
        default=Path(__file__).resolve().parents[3] / 'impl/compiler/llvm/LinxISAInstrInfo.td',
        help='Output TableGen file path'
    )
    
    args = parser.parse_args()
    
    if not args.spec.exists():
        print(f'Error: spec file not found: {args.spec}', file=sys.stderr)
        return 1
    
    spec_data = json.loads(args.spec.read_text())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    _generate_tablegen_file(spec_data, args.out)
    
    return 0


if __name__ == '__main__':
    import sys
    sys.exit(main())
