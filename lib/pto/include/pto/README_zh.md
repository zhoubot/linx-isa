# include/pto/

该目录是 PTO Tile Lib 的主要公共头文件入口，包含：

- Tile 类型系统与共享工具
- PTO 指令 API 声明（Auto/Manual 两种形式）
- CPU 仿真/Stub 支持
- NPU 指令实现（按 SoC 代际划分）

## 推荐的 include

- `include/pto/pto-inst.hpp`：统一入口头（建议上层代码直接 include 该文件）

在 CPU 仿真场景下，该头文件会包含 CPU stub（例如定义 `__CPU_SIM` 时会引入 `pto/common/cpu_stub.hpp`）。

## 目录结构

- `common/`：平台无关的 Tile 与指令基础设施
  - `pto_tile.hpp`：核心 Tile 类型与布局
  - `pto_instr.hpp`、`pto_instr_impl.hpp`：指令声明与共享实现
  - `memory.hpp`、`constants.hpp`、`utils.hpp`、`type.hpp`：通用工具与常量
- `cpu/`：CPU 侧仿真/调试支持（如启用）
- `npu/`：NPU 侧实现（按 SoC 版本拆分）
  - `npu/a2a3/`：Ascend A2/A3 系列
  - `npu/a5/`：Ascend A5 系列

## 相关文档

- 指令参考：`docs/isa/`
