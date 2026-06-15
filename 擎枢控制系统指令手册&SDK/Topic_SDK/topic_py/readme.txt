运行 main.py 文件经典报错：

ImportError：
/topic.so: undefined symbol: _ZN6google8protobuf7Message19CopyWithSourceCheckERS1_RKS1_

解决方案：
使用如下命令运行

 LD_PRELOAD=your_path/libprotobuf.so.32 python3 main.py

注意： your_path 替换为libprotobuf.so.32所在路径，即当前工程文件夹路径
