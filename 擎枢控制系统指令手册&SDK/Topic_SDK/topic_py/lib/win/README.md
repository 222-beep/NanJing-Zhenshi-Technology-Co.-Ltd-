# Windows 环境说明

## 编译环境

| 项目 | 版本 |
|------|------|
| **操作系统** | Windows 10 / Windows 11 |
| **Python** | 3.10 |
| **编译器** | MSVC (Visual Studio 2022) |
| **CMake** | 4.0.2 |

## 文件清单

| 文件 | 说明 |
|------|------|
| `topic.pyd` | pybind11 编译的 Python 扩展模块 |
| `libprotobuf.dll` | Protocol Buffers 3.x 动态库 |
| `libzmq-v142-mt-4_3_6.dll` | ZeroMQ 4.3.6 动态库 |

## 运行要求

- **Python 3.10** — 必须精确匹配此版本（pyd 文件与 Python 版本绑定）
- **Visual C++ 运行时** — 需安装 [VC++ Redistributable for Visual Studio 2022](https://aka.ms/vs/17/release/vc_redist.x64.exe)

## 验证方法

在 `topic_all_py` 目录下运行：

```bash
python -c "from platform_loader import get_topic_module; t = get_topic_module(); print('OK:', t)"
```

预期输出：`OK: <module 'topic' ...>`

## 库文件依赖关系

```
topic.pyd
  ├── libprotobuf.dll
  └── libzmq-v142-mt-4_3_6.dll
```

`platform_loader.py` 会自动将本目录加入 `PATH` 和 `os.add_dll_directory()`，无需手动配置。
