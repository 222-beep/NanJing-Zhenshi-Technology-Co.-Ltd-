# Linux x86_64环境说明

本目录提供Ubuntu 20.04 ABI基线的x86_64运行库。

```text
x86/
├── libprotobuf.so.32
├── libzmq.so.5
└── cp310/
    └── topic.so
```

- `cp310/topic.so`：CPython 3.10扩展模块
- `libprotobuf.so.32`：Protocol Buffers 3.21.12运行库
- `libzmq.so.5`：ZeroMQ 4.3.6运行库

`topic.so`包含`RUNPATH=$ORIGIN/..`，会从上一级x86目录自动加载两个运行库。

验证：

```bash
python3.10 -c "from platform_loader import get_topic_module; print(get_topic_module())"
```
