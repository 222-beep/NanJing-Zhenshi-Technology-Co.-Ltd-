# platform_loader.py
import sys
import os
import platform

_loaded = False
_dll_directory_handles = []


def _python_abi_tag():
    """返回当前CPython ABI目录名，例如Python 3.10对应cp310。"""
    implementation = platform.python_implementation()
    version = sys.version_info[:2]
    if implementation != 'CPython' or version != (3, 10):
        raise RuntimeError(
            f"Unsupported Python runtime: {implementation} "
            f"{version[0]}.{version[1]}; available ABI: CPython 3.10 (cp310)"
        )
    return 'cp310'


def get_topic_module():
    """自动检测平台、配置动态库路径，返回 topic 模块"""
    global _loaded, _dll_directory_handles
    if not _loaded:
        base_dir = os.path.dirname(os.path.abspath(__file__))
        # 动态库统一收口在 common/topic/python/lib，禁止在 Topic_SDK 内独立复制
        lib_dir = os.path.abspath(
            os.path.join(base_dir, '..', '..', 'common', 'topic', 'python', 'lib'))
        system = platform.system().lower()
        machine = platform.machine().lower()
        python_abi = _python_abi_tag()

        # 确定平台子目录
        if system == 'windows':
            if machine not in ('amd64', 'x86_64'):
                raise RuntimeError(f"Unsupported Windows architecture: {machine}")
            dependency_dir = os.path.join(lib_dir, 'win')
            target_dir = os.path.join(dependency_dir, python_abi)
            module_path = os.path.join(target_dir, 'topic.pyd')
        elif system == 'linux':
            if machine in ('x86_64', 'amd64'):
                subdir = 'x86'
            elif machine in ('aarch64', 'arm64'):
                subdir = 'arm'
            else:
                raise RuntimeError(f"Unsupported Linux architecture: {machine}")

            # 所有Linux架构统一使用Ubuntu 20.04 ABI基线库。
            dependency_dir = os.path.join(lib_dir, 'linux', subdir)
            target_dir = os.path.join(dependency_dir, python_abi)
            module_path = os.path.join(target_dir, 'topic.so')
        else:
            raise RuntimeError(f"Unsupported OS: {system}")

        if not os.path.isfile(module_path):
            raise RuntimeError(f"Platform module not found: {module_path}")

        # 添加模块搜索路径
        if target_dir not in sys.path:
            sys.path.insert(0, target_dir)

        # Windows需要显式注册模块目录和公共DLL目录，并保持句柄存活。
        # Linux topic.so包含RUNPATH=$ORIGIN/..，无需修改LD_LIBRARY_PATH。
        if system == 'windows':
            os.environ['PATH'] = (
                target_dir + ';' + dependency_dir + ';' +
                os.environ.get('PATH', '')
            )
            if hasattr(os, 'add_dll_directory'):
                _dll_directory_handles.extend([
                    os.add_dll_directory(target_dir),
                    os.add_dll_directory(dependency_dir),
                ])

        _loaded = True

    import topic
    return topic
