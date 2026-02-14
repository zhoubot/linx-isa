# Linx Instruction Set Architecture Manual (AsciiDoc)

This directory contains a draft ISA manual for the **Linx Instruction Set Architecture (Linx ISA)**, written in
**AsciiDoc** and built to **PDF** using
`asciidoctor-pdf` (via Bundler).

The content is specific to Linx’s design (block-structured control flow, `BSTART/BSTOP`, ClockHands temporaries,
template instructions like `FENTRY`, etc).

## Build

From this directory:

```bash
make pdf
```

Outputs:
- `build/linxisa-isa-manual.pdf`

## Regenerate generated sections

The manual includes generated AsciiDoc derived from the canonical spec:
- `spec/isa/spec/current/linxisa-v0.3.json`

Regenerate:

```bash
make gen
```
