# platform_loader.py
import sys
import os
import platform
import ctypes

_loaded = False

def get_topic_module():
    """自动检测平台、配置动态库路径，返回 topic 模块"""
    global _loaded
    if not _loaded:
        base_dir = os.path.dirname(os.path.abspath(__file__))
        system = platform.system().lower()
        machine = platform.machine().lower()

        # 确定平台子目录
        if system == 'windows':
            subdir = 'win'
        elif system == 'linux':
            if machine in ('x86_64', 'amd64', 'i386', 'i686'):
                subdir = 'x86'
            elif machine in ('armv7l', 'aarch64', 'arm64'):
                subdir = 'arm'
            else:
                raise RuntimeError(f"Unsupported Linux architecture: {machine}")
        else:
            raise RuntimeError(f"Unsupported OS: {system}")

        target_dir = os.path.join(base_dir, 'lib', subdir)
        if not os.path.isdir(target_dir):
            raise RuntimeError(f"Platform directory not found: {target_dir}")

        # 添加模块搜索路径
        if target_dir not in sys.path:
            sys.path.insert(0, target_dir)

        # 处理动态库依赖路径
        if system == 'windows':
            os.environ['PATH'] = target_dir + ';' + base_dir + ';' + os.environ.get('PATH', '')
            if hasattr(os, 'add_dll_directory'):
                os.add_dll_directory(target_dir)
                os.add_dll_directory(base_dir)
        elif system == 'linux':
            ld_path = target_dir + ':' + base_dir
            current_ld = os.environ.get('LD_LIBRARY_PATH', '')
            if current_ld:
                ld_path = ld_path + ':' + current_ld
            os.environ['LD_LIBRARY_PATH'] = ld_path
            # 重新加载动态库配置（使新路径生效）
            ctypes.CDLL(None, ctypes.RTLD_GLOBAL)

        _loaded = True

    import topic
    return topic