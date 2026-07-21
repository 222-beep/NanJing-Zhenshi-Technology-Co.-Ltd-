#ifndef __CPP_UTILS__
#define __CPP_UTILS__

#if defined(_WIN32) || defined(_MSC_VER)
	#ifdef CPP_RPC_LIB  // 编译库时定义 CPP_RPC_LIB 宏来导出符号
	#define CPP_RPC_EXPORT __declspec(dllexport)
	#else
	#define CPP_RPC_EXPORT __declspec(dllimport)
	#endif
#elif defined(__linux__) || defined(__APPLE__)
	#ifdef CPP_RPC_LIB
	#define CPP_RPC_EXPORT __attribute__ ((visibility ("default"))) // Linux/GCC: 编译时加 -fvisibility=hidden，仅标记该属性的符号对外可见
	#else
	#define CPP_RPC_EXPORT
	#endif
#endif

// Windows 控制台默认 GBK，程序输出 UTF-8 会乱码，启动时自动切到 UTF-8
#if defined(_WIN32) || defined(_MSC_VER)
extern "C" __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int);
namespace {
struct __InitConsole { __InitConsole() { SetConsoleOutputCP(65001); } } __init_console;
}
#endif

// 日志宏
#include <cstdint>
#include <chrono>
#include <ctime>
#include <cstdio>
#include <string>
#define CPP_RPC_LOG printf

/*
* 获取当前时间戳(ms)
*/
inline uint64_t GetCurrentTS() {

    auto now = std::chrono::system_clock::now();
    auto time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return time_stamp;
}

/*
* 获取当前本地时间字符串（可读格式: HH:MM:SS.mmm）
*/
inline std::string GetTimeStr()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto ms_total = duration_cast<milliseconds>(now.time_since_epoch()).count();
    auto t = system_clock::to_time_t(now);
    struct tm lt;
#if defined(_WIN32) || defined(_MSC_VER)
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d",
             lt.tm_hour, lt.tm_min, lt.tm_sec, (int)(ms_total % 1000));
    return buf;
}

// ============================================================================
// 统一错误日志
// ============================================================================

// 常见 errno 值 → 可读名称
inline const char *errno_name(int e)
{
    switch (e)
    {
    case 0:             return "OK";
#ifdef EPIPE
    case EPIPE:         return "EPIPE(Broken pipe)";
#endif
#ifdef ECONNRESET
    case ECONNRESET:    return "ECONNRESET(Connection reset)";
#endif
#ifdef ECONNREFUSED
    case ECONNREFUSED:  return "ECONNREFUSED(Connection refused)";
#endif
#ifdef ETIMEDOUT
    case ETIMEDOUT:     return "ETIMEDOUT(Timeout)";
#endif
#ifdef EAGAIN
    case EAGAIN:        return "EAGAIN(Try again)";
#endif
#ifdef EWOULDBLOCK
#if EAGAIN != EWOULDBLOCK
    case EWOULDBLOCK:   return "EWOULDBLOCK";
#endif
#endif
#ifdef EINTR
    case EINTR:         return "EINTR(Interrupted)";
#endif
#ifdef EINPROGRESS
    case EINPROGRESS:   return "EINPROGRESS(In progress)";
#endif
#ifdef ENOTCONN
    case ENOTCONN:      return "ENOTCONN(Not connected)";
#endif
#ifdef EBADF
    case EBADF:         return "EBADF(Bad fd)";
#endif
#ifdef ENOMEM
    case ENOMEM:        return "ENOMEM(Out of memory)";
#endif
#ifdef EACCES
    case EACCES:        return "EACCES(Permission denied)";
#endif
#ifdef EADDRINUSE
    case EADDRINUSE:    return "EADDRINUSE(Address in use)";
#endif
#ifdef EADDRNOTAVAIL
    case EADDRNOTAVAIL: return "EADDRNOTAVAIL(Addr not available)";
#endif
#ifdef ENETUNREACH
    case ENETUNREACH:   return "ENETUNREACH(Network unreachable)";
#endif
#ifdef EHOSTUNREACH
    case EHOSTUNREACH:  return "EHOSTUNREACH(Host unreachable)";
#endif
    default:            return "UNKNOWN";
    }
}

// ============================================================================
// RPC 调试日志开关（默认关闭，不污染用户控制台）
// ============================================================================
namespace cpp_rpc {
    // 开启/关闭底层调试日志（RPC_ERROR_LOG）
    // 默认关闭，需要排查网络问题时调用 SetDebugLogEnabled(true) 打开
    inline bool g_rpc_debug_enabled = false;

    inline void SetDebugLogEnabled(bool enabled) { g_rpc_debug_enabled = enabled; }
    inline bool IsDebugLogEnabled() { return g_rpc_debug_enabled; }
}

// 统一错误日志输出宏
// 所有关键错误走同一个出口，方便 grep/监控
// 默认不输出，通过 cpp_rpc::SetDebugLogEnabled(true) 打开
#define RPC_ERROR_LOG(fmt, ...) \
    do { \
        if (::cpp_rpc::g_rpc_debug_enabled) \
            fprintf(stderr, "[RPC_ERROR][%s] " fmt "\n", GetTimeStr().c_str(), ##__VA_ARGS__); \
    } while(0)

#endif