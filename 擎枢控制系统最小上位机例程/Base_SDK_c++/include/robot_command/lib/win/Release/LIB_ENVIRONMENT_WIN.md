# cpp_rpc.dll 运行环境说明 (Windows)

## 文件概要

| 属性 | 值 |
|------|-----|
| 文件格式 | PE32+ (DLL, x64) |
| 指令集架构 | x86-64 (AMD64 / Intel 64-bit) |
| 链接方式 | 动态链接 |
| 编译器 | Visual Studio 2022 (v143) |
| C++ 标准 | C++17 |
| 导出名称 | cpp_rpc.dll |

## 运行时依赖

| 依赖 | 说明 |
|------|------|
| MSVCP140.dll | C++ 标准库 |
| VCRUNTIME140.dll | VC++ 运行时 |
| VCRUNTIME140_1.dll | VC++ 运行时（异常处理） |
| api-ms-win-crt-*.dll | Universal CRT（系统自带） |
| WS2_32.dll | Windows Socket（系统自带） |
| KERNEL32.dll | Windows 内核（系统自带） |

> **核心依赖**: `MSVCP140.dll` / `VCRUNTIME140.dll` / `VCRUNTIME140_1.dll` 需要在目标机器上安装 VC++ 2015-2022 Redistributable 才能使用。

## 兼容操作系统

### 开箱即用

| 操作系统 | 最低版本 | 说明 |
|---------|---------|------|
| Windows 11 | 所有版本 | 系统自带 UCRT，安装 VC++ Redist 即可 |
| Windows 10 | 1607+ | 系统自带 UCRT，安装 VC++ Redist 即可 |
| Windows Server | 2016+ | 安装 VC++ Redist 即可 |

### 需额外处理

| 操作系统 | 问题 | 解决方案 |
|---------|------|---------|
| Windows 10 1507/1511 | UCRT 版本过旧 | 安装 [KB2999226](https://support.microsoft.com/kb/2999226) 或升级系统 |
| Windows 7 SP1 | 无内置 UCRT | 需安装 UCRT + VC++ Redist，仅限 x64 版本 |
| Windows 8.1 | 无内置 UCRT | 需安装 UCRT + VC++ Redist |

> Windows 7 / 8.1 已停止官方支持，建议使用 Windows 10 1607 及以上版本。

## 编译时链接

### 命令行 (MSVC)

```cmd
cl /std:c++17 /EHsc your_app.cpp /I<头文件目录> /link <库目录>\cpp_rpc.lib /OUT:your_app.exe
```

### CMake

```cmake
# 将以下内容加入 CMakeLists.txt
if(WIN32)
    link_directories(${CMAKE_SOURCE_DIR}/lib/win/Release)
    target_link_libraries(your_target cpp_rpc)

    # 构建后自动将 dll 复制到 exe 所在目录（否则运行时会报"找不到 cpp_rpc.dll"）
    add_custom_command(TARGET your_target POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/lib/win/Release/cpp_rpc.dll"
            "$<TARGET_FILE_DIR:your_target>"
    )
endif()
```

### Visual Studio 项目配置

1. **C/C++ → 常规 → 附加包含目录**: 添加 `include/`
2. **链接器 → 常规 → 附加库目录**: 添加 `lib/win/Release/`
3. **链接器 → 输入 → 附加依赖项**: 添加 `cpp_rpc.lib`
4. **C/C++ → 语言 → C++ 语言标准**: 设置为 `ISO C++17 标准 (/std:c++17)`

## 运行时部署

**编译链接**只需要 `cpp_rpc.lib`，但**程序运行**时必须能加载 `cpp_rpc.dll`。

Windows 查找 dll 的优先顺序为：
1. exe 所在目录（**推荐**）
2. 当前工作目录
3. 系统 PATH 环境变量中的目录
4. `C:\Windows\System32\`

### 推荐做法

直接将 `cpp_rpc.dll` 复制到 exe 输出目录（如 `build/Release/`）。如果使用 CMake，建议在 CMakeLists.txt 中配置 `add_custom_command(TARGET ... POST_BUILD)` 自动复制（见上方 CMake 示例）。

## 运行时检查

```cmd
:: 检查依赖是否满足
dumpbin /dependents cpp_rpc.dll
```

## 常见问题

| 现象 | 原因 | 解决方案 |
|------|------|---------|
| 找不到 MSVCP140.dll / VCRUNTIME140.dll | 未安装 VC++ 运行时 | 安装 [Visual C++ 2015-2022 Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe) |
| 应用程序无法正常启动 (0xc000007b) | 32/64 位不匹配 | 确认应用程序和 dll 均为 x64，安装 x64 版 VC++ Redist |
| 找不到 cpp_rpc.dll | dll 不在 exe 所在目录 | 将 `lib/win/Release/cpp_rpc.dll` 复制到 exe 输出目录（如 `build/Release/`），或配置 CMake POST_BUILD 自动复制 |

## 注意事项

- 本库为 **64 位 (x64)** 编译，不支持 32 位 (x86) 应用程序链接
- 本库使用 C++17 标准，调用方代码必须使用兼容的 C++ 标准版本（建议 C++17）
- 本库使用 MSVC 的新预处理器 (`/Zc:preprocessor`)，调用方应使用 Visual Studio 2019 16.6 或更高版本
- 编译时需启用 `/utf-8` 选项以确保中文字符正常处理

---

> 其他平台版本请参阅：[Linux x86-64](../../linux/x86/README.md) | [Linux ARM64](../../linux/arm/README.md)

