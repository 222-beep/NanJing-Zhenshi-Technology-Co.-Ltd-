# rpc.so使用环境要求（Linux ARM64 / CPython 3.10）

## 是否适用

使用环境需要同时满足以下条件：

| 检查项 | 要求 |
|---|---|
| 操作系统 | Ubuntu 20.04、22.04或24.04，64位 |
| CPU架构 | ARM 64位；`uname -m`输出`aarch64`或`arm64` |
| Python实现 | CPython |
| Python版本 | 3.10.x |
| 动态依赖 | `ldd rpc.so`没有显示`not found` |

Python 3.8、3.11、3.12不能使用本文件，需要对应的ABI版本。x86_64设备请使用`linux/x86/cp310/rpc.so`。

## 一分钟环境检查

```bash
uname -m
getconf LONG_BIT
cat /etc/os-release
python3.10 -c "import platform, sys; print(platform.python_implementation()); print(sys.version); print(platform.machine())"
ldd rpc.so
```

预期结果：

- CPU为`aarch64`或`arm64`。
- 系统位数为`64`。
- Python实现为`CPython`，版本为`3.10.x`。
- `ldd`没有任何`not found`。

## 快速使用

保持交付目录结构完整，在`rpc_py_all`目录执行：

```bash
python3.10 main.py
```

程序会自动加载：

```text
lib/linux/arm/cp310/rpc.so
```

也可以直接验证模块：

```bash
cd lib/linux/arm/cp310
python3.10 -c "import rpc; print('rpc import OK')"
```

## 常见问题

| 现象 | 原因 | 处理方法 |
|---|---|---|
| 找不到`cp310`目录 | 当前Python不是3.10 | 安装CPython 3.10或获取对应ABI版本 |
| `No module named 'rpc'` | 目录不完整或启动位置错误 | 保持目录结构完整，从`rpc_py_all`运行 |
| `wrong ELF class` | Python或系统不是64位 | 使用64位系统和64位CPython 3.10 |
| `Exec format error` | CPU架构不匹配 | x86_64设备改用x86版本 |
| `GLIBC_*`或`GLIBCXX_* not found` | 系统运行库过旧 | 使用支持的Ubuntu版本或重新编译兼容版本 |
| `ldd`显示`not found` | 缺少系统动态库 | 安装对应缺失库后重新检查 |

## 运行库最低要求

| 符号族 | 最低版本 |
|---|---|
| GLIBC | `GLIBC_2.28` |
| GLIBCXX | `GLIBCXX_3.4.26` |
| CXXABI | `CXXABI_1.3.11` |
| GCC | `GCC_3.3.1` |

Ubuntu 20.04、22.04和24.04 ARM64的默认运行库通常满足这些要求。

## 文件校验信息

| 项目 | 值 |
|---|---|
| 构建基线 | Ubuntu 20.04.6 ARM64、GCC 9.4、CPython 3.10 |
| SHA-256 | `af1ea30175faa0c98361539cd0d990fb7f96380607289d00641f90eb2c94ac53` |

校验命令：

```bash
sha256sum rpc.so
```

该文件包含构建路径RUNPATH，但当前依赖均为系统库，通常不影响使用。
