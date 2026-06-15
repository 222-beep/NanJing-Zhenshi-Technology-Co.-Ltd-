import os
import platform
import sys

def _lib_subdir():
    s, m = platform.system().lower(), platform.machine().lower()
    if s == "windows":
        return "win"
    if s == "linux":
        if m in ("x86_64", "amd64", "i386", "i686"):
            return "x86"
        if m in ("armv7l", "aarch64", "arm64"):
            return "arm"
        raise RuntimeError(f"Unsupported Linux architecture: {m}")
    raise RuntimeError(f"Unsupported OS: {s}")

_root = os.path.dirname(os.path.abspath(__file__))
_workspace = os.path.dirname(os.path.dirname(_root))
_tdir = os.path.join(_workspace, "common", "py_lib", _lib_subdir())
if not os.path.isdir(_tdir):
    raise RuntimeError(f"Platform directory not found: {_tdir}")
sys.path.insert(0, _tdir)
if platform.system() == "Windows":
    os.environ["PATH"] = _tdir + ";" + _root + ";" + os.environ.get("PATH", "")
    if hasattr(os, "add_dll_directory"):
        os.add_dll_directory(_tdir)
        os.add_dll_directory(_root)
elif platform.system() == "Linux":
    _ld = os.environ.get("LD_LIBRARY_PATH", "")
    os.environ["LD_LIBRARY_PATH"] = _tdir + (":" + _ld if _ld else "")

from robot_ext import *
__all__ = ["create_client", "send_rpc_async", "send_rpcsy"]