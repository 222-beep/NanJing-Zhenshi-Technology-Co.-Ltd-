# rpc.so使用环境要求（Linux x86_64 / CPython 3.10）

## 是否适用

使用环境需要同时满足以下条件：

| 检查项 | 要求 |
|---|---|
| 操作系统 | Ubuntu 20.04、22.04或24.04，64位 |
| CPU架构 | Intel/AMD 64位；`uname -m`输出`x86_64` |
| Python实现 | CPython |
| Python版本 | 3.10.x |
| 动态依赖 | `ldd rpc.so`没有显示`not found` |

Python 3.8、3.11、3.12不能使用本文件，需要对应的`cp38`、`cp311`或`cp312`版本。ARM设备请使用`linux/arm/cp310/rpc.so`。

## 一分钟环境检查

```bash
uname -m
getconf LONG_BIT
cat /etc/os-release
python3.10 -c "import platform, sys; print(platform.python_implementation()); print(sys.version); print(platform.machine())"
ldd rpc.so
```

预期结果：

- CPU为`x86_64`。
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
lib/linux/x86/cp310/rpc.so
```

也可以直接验证模块：

```bash
cd lib/linux/x86/cp310
python3.10 -c "import rpc; print('rpc import OK')"
```

## 常见问题

| 现象 | 原因 | 处理方法 |
|---|---|---|
| 找不到`cp310`目录 | 当前Python不是3.10 | 安装CPython 3.10或获取对应ABI版本 |
| `No module named 'rpc'` | 目录不完整或启动位置错误 | 保持目录结构完整，从`rpc_py_all`运行 |
| `wrong ELF class` | Python或系统不是64位 | 使用64位系统和64位CPython 3.10 |
| `Exec format error` | CPU架构不匹配 | ARM64设备改用ARM版本 |
| `GLIBC_*`或`GLIBCXX_* not found` | 系统运行库过旧 | 使用支持的Ubuntu版本或重新编译兼容版本 |
| `ldd`显示`not found` | 缺少系统动态库 | 安装对应缺失库后重新检查 |

## 文件校验信息

| 项目 | 值 |
|---|---|
| 构建基线 | Ubuntu 20.04 x86_64、GCC 9.4、CPython 3.10 |
| SHA-256 | `912a02e3cc5b8bebf7c76370430d9deb5db64519ce996f69db64687b8720cf61` |

校验命令：

```bash
sha256sum rpc.so
```
