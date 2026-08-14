Linux运行库说明：

1. x86和ARM都使用Ubuntu 20.04 ABI基线编译的库。
2. CPython 3.10模块位于lib/linux/<arch>/cp310/topic.so。
3. libprotobuf.so.32和libzmq.so.5位于模块的上一级架构目录。
4. topic.so包含RUNPATH=$ORIGIN/..，正常情况下无需配置LD_LIBRARY_PATH或LD_PRELOAD。
5. 底层Topic默认上报频率：RT为250 Hz，NRT为2 Hz；实际接收频率受发布端配置、网络和客户端负载影响。

验证命令：

python3.10 -c "from platform_loader import get_topic_module; print(get_topic_module())"
