#pragma once

#if defined(_WIN32) || defined(_MSC_VER)
  #define MESSAGEBUS_EXPORT __declspec(dllexport)  
#elif defined(__linux__) || defined(__APPLE__)  
  #define MESSAGEBUS_EXPORT __attribute__ ((visibility ("default")))
#endif

