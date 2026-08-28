# libcpp_rpc.so适用环境说明（Linux ARM64）

## 使用环境检查

| 检查项 | 要求 |
|---|---|
| 操作系统 | Ubuntu 20.04、22.04或24.04，64位 |
| CPU架构 | ARM 64位；`uname -m`输出`aarch64`或`arm64` |
| 编译环境 | 支持C++17；建议GCC 9或更高版本 |
| 动态依赖 | `ldd libcpp_rpc.so`没有显示`not found` |

快速检查：

```bash
uname -m
getconf LONG_BIT
cat /etc/os-release
ldd libcpp_rpc.so
```

如果设备输出`x86_64`，请使用`lib/linux/x86/libcpp_rpc.so`。

## 适用范围

本目录提供Linux ARM64版本的`libcpp_rpc.so`。库目录只区分CPU架构，不再按Ubuntu版本划分。

| 项目 | 说明 |
|---|---|
| CPU架构 | ARM64 / AArch64 |
| 操作系统 | 64位Linux |
| 构建基线 | Ubuntu 20.04.6 ARM64 |
| GCC / G++ | 9.4.0 |
| CMake | 3.16.3 |
| glibc | 2.31 |
| 文件格式 | ELF 64-bit LSB shared object，AArch64，小端序 |
| SONAME | `libcpp_rpc.so` |
| 原始文件名 | `libcpp_rpc.so.arm-2004` |
| 发布文件名 | `libcpp_rpc.so` |
| SHA-256 | `dd9cd50508cad59622998649676367bf4aaf3cfb74d8061a3716b109e9f938db` |

不适用于ARM32、x86、x86_64、Windows或macOS。

## Ubuntu兼容性

该库使用Ubuntu 20.04 ARM版本作为统一发布文件。建议的适用范围为：

| 环境 | 状态 | 说明 |
|---|---|---|
| Ubuntu 20.04 ARM64 | 支持 | 原始构建基线 |
| Ubuntu 22.04 ARM64 | 支持，建议实测 | 较新系统通常兼容旧基线动态库 |
| Ubuntu 24.04 ARM64 | 支持，建议实测 | 较新系统通常兼容旧基线动态库 |
| Ubuntu 18.04及更早版本 | 不作为支持目标 | 未纳入当前兼容范围 |

首次部署到22.04或24.04时，应测试连接、同步调用、异步调用、超时和断线重连。

## 最低运行库要求

该文件已经在Ubuntu 20.04.6 ARM64环境完成检测。动态符号表中的最高版本要求为：

| 符号族 | 最低版本 |
|---|---|
| GLIBC | `GLIBC_2.17` |
| GLIBCXX | `GLIBCXX_3.4.22` |
| CXXABI | `CXXABI_1.3.9` |
| GCC | `GCC_3.0` |

运行时直接依赖：

- `libpthread.so.0`
- `libstdc++.so.6`
- `libgcc_s.so.1`
- `libc.so.6`
- `ld-linux-aarch64.so.1`

`libm.so.6`由系统依赖链加载。检测环境中所有依赖均能正常解析，`ldd`没有出现`not found`，且库中没有Python ABI依赖。

Ubuntu 20.04、22.04和24.04 ARM64的默认运行库通常满足上述符号版本要求。

如需在其他发行版部署，可使用下面的命令重新确认：

请在ARM64环境执行：

```bash
file lib/linux/arm/libcpp_rpc.so
ldd lib/linux/arm/libcpp_rpc.so
readelf --version-info lib/linux/arm/libcpp_rpc.so \
  | grep -oE '(GLIBC|GLIBCXX|CXXABI|GCC)_[0-9]+(\.[0-9]+)*' \
  | sort -Vu
sha256sum lib/linux/arm/libcpp_rpc.so
```

最低要求由输出中每个符号族的最高版本决定，`ldd`输出中不得出现`not found`。

## 构建和运行

CMake根据`CMAKE_SYSTEM_PROCESSOR`自动选择：

```text
lib/linux/arm/libcpp_rpc.so
```

构建：

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j"$(nproc)"
```

运行前可配置动态库搜索路径：

```bash
cd ..
export LD_LIBRARY_PATH="$PWD/lib/linux/arm${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
./build/Hello_MoveAbsJ
```

也可以在构建或安装阶段配置RPATH。

## 部署检查

- `file`应显示64位AArch64共享库。
- `ldd`不应出现`not found`。
- SHA-256应与本文记录一致。
- 目标系统应至少提供`GLIBC_2.17`、`GLIBCXX_3.4.22`和`CXXABI_1.3.9`。
- 重新编译或修改二进制后，应重新检测并更新本文。

当前文件包含调试信息且未剥离符号（`with debug_info, not stripped`）。这不影响运行；如需减小发布体积，可以保留原文件作为调试版本，并对交付副本执行`strip --strip-unneeded`。执行后必须重新计算SHA-256。

## 常见错误

| 错误 | 原因 | 处理方法 |
|---|---|---|
| `libcpp_rpc.so: cannot open shared object file` | 未找到动态库 | 设置`LD_LIBRARY_PATH`或RPATH |
| `GLIBC_* not found` | glibc低于库的构建要求 | 使用支持的Ubuntu版本或重新构建 |
| `GLIBCXX_* not found` | `libstdc++.so.6`过旧 | 使用满足符号版本要求的运行库 |
| `Exec format error` | 使用了x86库或架构不匹配 | 确认使用`lib/linux/arm/libcpp_rpc.so` |

> x86_64版本参见[`../x86/README.md`](../x86/README.md)。
