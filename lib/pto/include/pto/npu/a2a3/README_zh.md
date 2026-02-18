# include/pto/npu/a2a3/

Ascend A2/A3 系列 PTO 指令实现头文件。

## 概览

- 按指令（或指令族）组织实现，例如：`TAdd.hpp`、`TMatmul.hpp`、`TLoad.hpp`、`TStore.hpp`
- 同时提供一些可复用的算子模式（例如 Reduce/Expand/PartOp 等辅助实现）

## 相关内容

- ISA 语义与示例：`docs/isa/`
- A2/A3 NPU ST 测试：`tests/npu/a2a3/src/st/`
