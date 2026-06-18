运行 main.py 文件如果出现类似报错：

ImportError：
/topic.so: undefined symbol: _ZN6google8protobuf7Message19CopyWithSourceCheckERS1_RKS1_

说明运行时没有优先加载匹配的 libprotobuf.so.32。

当前版本已由 platform_loader.py 自动配置 common/topic/python/lib 与 common/topic/shared 下的库路径，通常直接运行：

python3 main.py

如果仍需手动指定，请将 LD_PRELOAD 指向 common/topic 下对应平台目录中的 libprotobuf.so.32，例如：

LD_PRELOAD=../../common/topic/python/lib/linux/x86/20.04/libprotobuf.so.32 python3 main.py
