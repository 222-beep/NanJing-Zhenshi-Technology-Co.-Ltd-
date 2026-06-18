# Linux x86_64 环境说明

## 已验证环境

| 项目 | 版本 |
|------|------|
| **操作系统** | Ubuntu 20.04.3 LTS (Focal Fossa) |
| **架构** | x86_64 (amd64) |
| **Python** | 3.10.11 |
| **GCC** | 9.4.0 |
| **CMake** | 3.16.3 |
| **glibc** | 2.31 |

## 文件清单

| 文件 | 说明 |
|------|------|
| `topic.so` | pybind11 编译的 Python 扩展模块 |
| `libprotobuf.so.32` | Protocol Buffers 3.x 动态库 |
| `libzmq.so` / `libzmq.so.5` | ZeroMQ 4.3.6 动态库 |

## 运行要求

- **Python 3.10** — 必须精确匹配此版本（.so 文件与 Python 版本绑定）
- **glibc >= 2.31** — 编译时链接的 C 运行时版本

## 部署步骤

### 1. 安装 Python 3.10

系统通常自带 Python 3.8，需额外安装 Python 3.10：

```bash
sudo apt update
sudo apt install python3.10
```

### 2. 拷贝文件

将整个 `Topic_SDK/topic_py/` 目录拷贝到目标机器。

### 3. 验证

在 `topic_py` 目录下运行：

```bash
python3.10 -c "from platform_loader import get_topic_module; t = get_topic_module(); print('OK:', t)"
```

预期输出：`OK: <module 'topic' ...>`

## 动态库依赖关系

```
topic.so
  ├── libprotobuf.so.32
  ├── libzmq.so.5
  ├── libpthread.so.0
  ├── libstdc++.so.6
  ├── libc.so.6          (glibc >= 2.31)
  └── libgcc_s.so.1
```

`platform_loader.py` 会自动将本目录加入 `LD_LIBRARY_PATH`，无需手动设置。

## 常见问题

### ImportError: undefined symbol

如果运行 `main.py` 时报类似以下错误：

```
ImportError: /topic.so: undefined symbol: _ZN6google8protobuf7Message19CopyWithSourceCheckERS1_RKS1_
```

这是因为系统的 protobuf 库与本模块编译时使用的版本不一致。使用 `LD_PRELOAD` 强制加载本目录下的 protobuf 库：

```bash
LD_PRELOAD=<path-to-lib>/libprotobuf.so.32 python3.10 main.py
```

将 `<path-to-lib>` 替换为 `lib/x86/` 目录的实际路径。

### Python 版本不匹配

如果系统默认 `python3` 不是 3.10，务必使用 `python3.10` 命令运行：

```bash
python3.10 main.py
```
