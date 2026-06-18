# platform_loader.py
import sys
import os
import platform
import ctypes

_loaded = False


def _detect_ubuntu_version():
    """从 /etc/os-release 检测 Ubuntu 版本号，如 '20.04', '22.04'。失败返回 None。"""
    try:
        with open('/etc/os-release', 'r') as f:
            for line in f:
                if line.startswith('VERSION_ID='):
                    version = line.strip().split('=')[1].strip('"')
                    parts = version.split('.')
                    if len(parts) >= 2:
                        return f"{parts[0]}.{parts[1]}"
                    return version
    except Exception:
        pass
    return None


def get_topic_module():
    """自动检测平台、配置动态库路径，返回 topic 模块"""
    global _loaded
    if not _loaded:
        base_dir = os.path.dirname(os.path.abspath(__file__))
        common_topic_dir = os.path.abspath(os.path.join(base_dir, '..', '..', 'common', 'topic'))
        python_lib_dir = os.path.join(common_topic_dir, 'python', 'lib')
        shared_lib_dir = os.path.join(common_topic_dir, 'shared', 'lib')
        system = platform.system().lower()
        machine = platform.machine().lower()

        shared_dir = None

        # 确定平台子目录
        if system == 'windows':
            target_dir = os.path.join(python_lib_dir, 'win')
            shared_dir = os.path.join(shared_lib_dir, 'win', 'Release')
            if not os.path.isdir(target_dir):
                raise RuntimeError(f"Platform directory not found: {target_dir}")
        elif system == 'linux':
            if machine in ('x86_64', 'amd64', 'i386', 'i686'):
                subdir = 'x86'
            elif machine in ('armv7l', 'aarch64', 'arm64'):
                subdir = 'arm'
            else:
                raise RuntimeError(f"Unsupported Linux architecture: {machine}")

            # 检测 Ubuntu 版本，构建版本化路径 lib/linux/{arch}/{version}/
            ubuntu_ver = _detect_ubuntu_version()
            versions_to_try = [ubuntu_ver] if ubuntu_ver else []
            versions_to_try += ['20.04', '22.04']  # fallback 版本列表

            target_dir = None
            for v in versions_to_try:
                candidate = os.path.join(python_lib_dir, 'linux', subdir, v)
                if os.path.isdir(candidate):
                    target_dir = candidate
                    shared_candidate = os.path.join(shared_lib_dir, 'linux', subdir, v, 'Release')
                    if os.path.isdir(shared_candidate):
                        shared_dir = shared_candidate
                    break

            if target_dir is None:
                raise RuntimeError(
                    f"Platform directory not found: common/topic/python/lib/linux/{subdir}/{{20.04,22.04}}"
                )
        else:
            raise RuntimeError(f"Unsupported OS: {system}")

        library_dirs = [target_dir]
        if shared_dir and os.path.isdir(shared_dir):
            library_dirs.append(shared_dir)

        # 添加模块搜索路径
        if target_dir not in sys.path:
            sys.path.insert(0, target_dir)

        # 处理动态库依赖路径
        if system == 'windows':
            os.environ['PATH'] = os.pathsep.join(library_dirs + [base_dir, os.environ.get('PATH', '')])
            if hasattr(os, 'add_dll_directory'):
                for library_dir in library_dirs:
                    os.add_dll_directory(library_dir)
                os.add_dll_directory(base_dir)
        elif system == 'linux':
            current_ld = os.environ.get('LD_LIBRARY_PATH', '')
            ld_parts = library_dirs + ([current_ld] if current_ld else [])
            os.environ['LD_LIBRARY_PATH'] = os.pathsep.join(ld_parts)

            # 优先从 Python 专属目录加载；若不存在，再从 shared 目录加载。
            for lib_name in ('libprotobuf.so.32', 'libprotobuf.so', 'libzmq.so.5', 'libzmq.so'):
                for library_dir in library_dirs:
                    lib_path = os.path.join(library_dir, lib_name)
                    if os.path.exists(lib_path):
                        ctypes.CDLL(lib_path, ctypes.RTLD_GLOBAL)
                        break
            ctypes.CDLL(None, ctypes.RTLD_GLOBAL)

        _loaded = True

    import topic
    return topic