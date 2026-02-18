# Linx C++ Bring-up Contract (C++17, LLVM stack, musl-first)

This document freezes the hosted C++ bring-up contract for LinxISA so gate
scripts and reports use one deterministic policy.

## Scope

- Primary runtime stack: `compiler-rt` + `libunwind` + `libc++abi` + `libc++`.
- Language baseline: `-std=c++17`.
- Bring-up profile `CXX17_NOEH`:
  - `-fno-exceptions`
  - `-fno-rtti`
- libc order: musl lane first, glibc lane second.
- Promotion policy: both `pin` and `external` lanes must pass.

## Triple and lane contract

- Baremetal compile lane: `linx64-linx-none-elf` and `linx32-linx-none-elf`.
- Hosted Linux musl lane: `linx64-unknown-linux-musl`.
- Hosted Linux glibc lane (follow-up): `linx64-unknown-linux-gnu`.

Lane roles:

- `pin` lane: reproducible baseline from superproject submodule SHAs.
- `external` lane: integration health against active external heads.

Promotion rule:

- Never promote runtime/C++ status to green from one lane only.
- Require matching pass classification and evidence in both lanes.

## Clang driver contract

- Linx cross links must not route through host C++ drivers.
- Default linker for Linx Linux and Linx baremetal flows is `ld.lld`.
- Linux Linx default C++ runtime policy in this phase:
  - `stdlib=libc++`
  - `rtlib=compiler-rt`
  - `unwindlib=libunwind` (policy target for hosted C++ gates)

## Sysroot contract (musl)

Musl sysroot base:

- `out/libc/musl/install/<mode>/`

LLVM C++ runtime overlay install root:

- `out/cpp-runtime/musl-cxx17-noeh/install/`

Expected hosted C++ layout consumed by clang:

- Headers:
  - `<overlay>/include/c++/v1`
- Runtime libs:
  - `<overlay>/lib/libc++.a`
  - `<overlay>/lib/libc++abi.a`
  - `<overlay>/lib/libunwind.a`
  - `<overlay>/lib/clang/<ver>/lib/linx64/libclang_rt.builtins-*.a` (or arch-equivalent)

Merge/install policy:

- Overlay artifacts are copied into the active musl sysroot for AVS/QEMU gates.
- No hidden host include/lib fallback is allowed in gate commands.

## Gate IDs and evidence policy

New AVS IDs are reserved for C++ bring-up and recorded in:

- `avs/linx_avs_v1_test_matrix.yaml`
- `avs/linx_avs_v1_test_matrix_status.json`

Gate evidence must include:

- exact command
- lane (`pin` or `external`)
- SHA manifest (llvm/qemu/linux/musl/glibc/...)
- pass/fail classification
- artifact paths (logs + summaries)

## Canonical commands

Build/install musl C++ runtime overlay:

```bash
bash tools/build_linx_llvm_cpp_runtimes.sh --mode phase-b
```

C++ compile gate:

```bash
(cd avs/compiler/linx-llvm/tests && ./run_cpp.sh)
```

Musl C++ runtime smoke gate:

```bash
python3 avs/qemu/run_musl_smoke.py --mode phase-b --link both --sample cpp17_smoke
```

