# include/pto/npu/

NPU 侧 PTO 指令实现。不同 SoC 代际会对应不同的优化实现与流水线细节。

## 目录结构

- `a2a3/`：Ascend A2/A3 实现（例如 `TAdd.hpp`、`TMatmul.hpp`、`TLoad.hpp`）
- `a5/`：Ascend A5 实现（例如 `TAdd.hpp`、`TMatmul.hpp`、`TLoad.hpp`）

## 选择 SoC 版本

SoC 选择由构建系统与测试脚本控制：

- `tests/script/run_st.py` / `tests/script/build_st.py`：通过 `-v a3|a5` 选择
- `tests/npu/<soc>/src/st/CMakeLists.txt`：按 SoC 构建对应的 ST 目标与依赖

端到端流程建议从 `docs/getting-started.md` 开始。
