#ifndef __CPP_UTILS__
#define __CPP_UTILS__

#if defined(_WIN32) || defined(_MSC_VER)
	#ifdef CPP_RPC_LIB  //可以通过-DDEEPNETAPI_EXPORT来定义该变量
	#define CPP_RPC_EXPORT __declspec(dllexport)   
	#else
	#define CPP_RPC_EXPORT __declspec(dllimport)  
	#endif
#elif defined(__linux__) || defined(__APPLE__)  
	#ifdef CPP_RPC_LIB
	#define CPP_RPC_EXPORT __attribute__ ((visibility ("default")))   //对库开发者来说，只希望把一些需要公开的函数或者类给使用者，其他的进行隐藏，所以在导出库的时候，需要在编译选项上添加-fvisibility=hidden，只有函数或者类前加了__attribute__ ((visibility ("default")))的可以被调用者使用，其他的全部隐藏。通过在编译选项上添加-fvisibility=hidden则把其他函数进行隐藏，只有__attribute__ ((visibility ("default")))的函数会保持默认，即全局可见，如果编译时不加-fvisibility=hidden，则不管函数或者类前是否添加__attribute__ ((visibility ("default")))，都是全局可见，因为默认是default
	#else
	#define CPP_RPC_EXPORT
	#endif
#endif

// 日志宏
#include <cstdint>
#include <chrono>
#define CPP_RPC_LOG printf

/*
* 获取当前时间戳(ms)
*/
inline uint64_t GetCurrentTS() {

    auto now = std::chrono::system_clock::now();
    auto time_stamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return time_stamp;
}

#endif

//void delay_ms(unsigned int milliseconds);
