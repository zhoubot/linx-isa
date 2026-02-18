# LinxISA v0.3 Reconciliation Notes

## Status Summary
- `keep`: 5
- `normalize`: 3
- `defer`: 2
- `drop`: 1

## Decisions
- `simt_lane_policy` -> `keep`: Inactive-lane policy explicit (merge|zero) (AI SIMT behavior is coherent and repeatedly specified)
- `memory_channel_model` -> `keep`: BCC/MTC channel contract with ordering/barrier semantics (Text is coherent with Janus microarchitecture intent)
- `tile_descriptor_model` -> `keep`: B.IOT/B.IOTI/B.IOD/B.ATTR/B.ARG/B.TEXT + C.B.DIMI (Matches current block descriptor machinery and example evidence)
- `cube_baseline_ops` -> `keep`: MAMULB/MAMULBAC/MAMULB.ACC/ACCCVT as staged CUBE baseline (Consistent with example and existing bring-up support)
- `tma_baseline_ops` -> `keep`: TLOAD/TSTORE/TPREFETCH/TMOV staged in TMA profile (Well-defined in raw text and aligned with existing architecture docs)
- `normalize_legacy_l_family` -> `normalize`: L.* -> V.* (Canonical v0.3 naming policy is V.* only)
- `normalize_bstart_par` -> `normalize`: BSTART.PAR -> typed BSTART.{TMA,CUBE,TEPL,VPAR} (Canonical typed block-start policy for v0.3 outputs)
- `normalize_kill_to_reuse` -> `normalize`: Legacy .kill annotations mapped to non-reuse semantics (Canonical descriptor payload uses reuse flag encoding)
- `defer_mamulbmx_group2` -> `defer`: MAMULBMX* group=2 semantics deferred (Raw material contains unresolved/contradictory group=2 details)
- `defer_legacy_052_fragments` -> `defer`: Legacy 0.52 fragments excluded from canonical v0.3 (Raw text includes mixed-version carryover sections)
- `drop_editorial_prompt_sections` -> `drop`: Prompt/editorial text removed (Non-normative authoring prompts are not ISA content)

## Explicit Defer List
- `defer_mamulbmx_group2`: MAMULBMX* group=2 semantics deferred
- `defer_legacy_052_fragments`: Legacy 0.52 fragments excluded from canonical v0.3

## ASM Token Classification
- `B.ARG` -> `keep` (B.ARG)
- `B.IOR` -> `keep` (B.IOR)
- `B.IOT` -> `keep` (B.IOT)
- `B.IOTI` -> `keep` (B.IOTI)
- `B.TEXT` -> `keep` (B.TEXT)
- `BSTART.PAR` -> `normalize` (BSTART.{TMA|CUBE|TEPL|VPAR})
- `C.B.DIMI` -> `keep` (C.B.DIMI)
- `C.BSTART.STD` -> `keep` (C.BSTART.STD)
- `C.BSTOP` -> `keep` (C.BSTOP)
- `L.BSTOP` -> `normalize` (C.BSTOP)
- `l.fexp` -> `normalize` (v.fexp)
- `l.fmax` -> `normalize` (v.fmax)
- `l.fmsub` -> `normalize` (v.fmsub)
- `l.fmul` -> `normalize` (v.fmul)
- `l.fsub` -> `normalize` (v.fsub)
- `l.lw` -> `normalize` (v.lw)
- `l.lwi` -> `normalize` (v.lwi)
- `l.slli` -> `normalize` (v.slli)
- `l.sw` -> `normalize` (v.sw)

## Janus Alignment
- `simt_lane_policy` aligned=True (Janus keyword scan for: Inactive-lane policy explicit (merge|zero))
- `memory_channel_model` aligned=True (Janus keyword scan for: BCC/MTC channel contract with ordering/barrier semantics)
- `tile_descriptor_model` aligned=True (Janus keyword scan for: B.IOT/B.IOTI/B.IOD/B.ATTR/B.ARG/B.TEXT + C.B.DIMI)
- `cube_baseline_ops` aligned=True (Janus keyword scan for: MAMULB/MAMULBAC/MAMULB.ACC/ACCCVT as staged CUBE baseline)
- `tma_baseline_ops` aligned=True (Janus keyword scan for: TLOAD/TSTORE/TPREFETCH/TMOV staged in TMA profile)
- `normalize_legacy_l_family` aligned=True (Janus keyword scan for: L.* -> V.*)
- `normalize_bstart_par` aligned=True (Janus keyword scan for: BSTART.PAR -> typed BSTART.{TMA,CUBE,TEPL,VPAR})
- `normalize_kill_to_reuse` aligned=True (Janus keyword scan for: Legacy .kill annotations mapped to non-reuse semantics)
- `defer_mamulbmx_group2` aligned=True (Janus keyword scan for: MAMULBMX* group=2 semantics deferred)
- `defer_legacy_052_fragments` aligned=True (Janus keyword scan for: Legacy 0.52 fragments excluded from canonical v0.3)
- `drop_editorial_prompt_sections` aligned=True (Janus keyword scan for: Prompt/editorial text removed)

