# LinxISA Repository Flow (v0.3)

The repository is organized around a spec-first flow where all implementations consume the same v0.3 catalog.

## Flow

1. ISA definition in `spec/isa/golden/v0.3/`
2. Compiled catalog in `spec/isa/spec/current/linxisa-v0.3.json`
3. Generated decode assets in `spec/isa/generated/codecs/`
4. Compiler/emulator/RTL integration under `impl/`
5. Validation through `tools/regression/run.sh` and `tools/bringup/check26_contract.py`
