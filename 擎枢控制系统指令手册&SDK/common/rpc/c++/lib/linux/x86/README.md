# libcpp_rpc.so适用环境说明（Linux x86_64）

## 使用环境检查

| 检查项 | 要求 |
|---|---|
| 操作系统 | Ubuntu 20.04、22.04或24.04，64位 |
| CPU架构 | Intel/AMD 64位；`uname -m`输出`x86_64` |
| 编译环境 | 支持C++17；建议GCC 9或更高版本 |
| 动态依赖 | `ldd libcpp_rpc.so`没有显示`not found` |

快速检查：

```bash
uname -m
getconf LONG_BIT
cat /etc/os-release
ldd libcpp_rpc.so
```

如果设备输出`aarch64`或`arm64`，请使用`lib/linux/arm/libcpp_rpc.so`。

## 适用范围

本目录提供Linux x86_64版本的`libcpp_rpc.so`。库目录只区分CPU架构，不再按Ubuntu版本划分。

| 项目 | 说明 |
|---|---|
| CPU架构 | x86_64 / AMD64 / Intel 64位 |
| 操作系统 | 64位Linux |
| 构建基线 | Ubuntu 20.04、GCC 9.4、C++17 |
| 文件格式 | ELF 64-bit LSB shared object |
| SONAME | `libcpp_rpc.so` |
| SHA-256 | `9d0a693726ecf6d1128a1afbd6fba2217a218406c73075e3354240b570b9503c` |

不适用于32位x86、ARM32、ARM64、Windows或macOS。

## Ubuntu兼容性

该库以Ubuntu 20.04作为最低构建基线。较新Ubuntu通常向后兼容旧版本构建的动态库，因此建议的适用范围为：

| 环境 | 状态 | 说明 |
|---|---|---|
| Ubuntu 20.04 x86_64 | 支持 | 原始构建基线 |
| Ubuntu 22.04 x86_64 | 支持，建议实测 | 系统运行库版本高于构建基线 |
| Ubuntu 24.04 x86_64 | 支持，建议实测 | 系统运行库版本高于构建基线 |
| Ubuntu 18.04及更早版本 | 不作为支持目标 | 即使能够加载，也未纳入当前兼容范围 |

“能够加载”不等同于完整功能已经验证。首次部署到新系统时，应测试连接、同步调用、异步调用、超时和断线重连。

## 最低运行库要求

根据该文件的动态符号记录，最高版本要求为：

| 符号族 | 最低版本 |
|---|---|
| GLIBC | `GLIBC_2.9` |
| GLIBCXX | `GLIBCXX_3.4.21` |
| CXXABI | `CXXABI_1.3.9` |
| GCC | `GCC_3.0` |

主要依赖包括`libstdc++.so.6`、`libgcc_s.so.1`、`libc.so.6`、`libm.so.6`和x86_64动态链接器。Ubuntu 20.04、22.04和24.04的默认运行库通常满足上述要求。

## 构建和运行

CMake根据`CMAKE_SYSTEM_PROCESSOR`自动选择：

```text
lib/linux/x86/libcpp_rpc.so
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
export LD_LIBRARY_PATH="$PWD/lib/linux/x86${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
./build/Hello_MoveAbsJ
```

也可以在构建或安装阶段配置RPATH。

## 部署检查

```bash
file lib/linux/x86/libcpp_rpc.so
ldd lib/linux/x86/libcpp_rpc.so
readelf --version-info lib/linux/x86/libcpp_rpc.so \
  | grep -oE '(GLIBC|GLIBCXX|CXXABI|GCC)_[0-9]+(\.[0-9]+)*' \
  | sort -Vu
sha256sum lib/linux/x86/libcpp_rpc.so
```

检查要求：

- `file`显示`ELF 64-bit`和`x86-64`。
- `ldd`没有`not found`。
- 目标系统提供的符号版本不低于本文要求。
- 文件未重新编译或修改时，SHA-256应与本文一致。

## 常见错误

| 错误 | 原因 | 处理方法 |
|---|---|---|
| `libcpp_rpc.so: cannot open shared object file` | 未找到动态库 | 设置`LD_LIBRARY_PATH`或RPATH |
| `GLIBC_* not found` | glibc过旧 | 使用支持的Ubuntu版本或在更低基线重新构建 |
| `GLIBCXX_* not found` | `libstdc++.so.6`过旧 | 使用满足符号版本要求的运行库 |
| `wrong ELF class` | 32/64位不匹配 | 使用64位程序和系统 |
| `Exec format error` | CPU架构不匹配 | ARM64设备应使用`lib/linux/arm/libcpp_rpc.so` |

> ARM64版本参见[`../arm/README.md`](../arm/README.md)。
