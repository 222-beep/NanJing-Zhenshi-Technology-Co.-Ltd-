import sys, os, time
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

ROBOT_IP = "192.168.11.11"

# ==================================================================
#  main_send_params.py  ——  发送接口参数说明示例
#
#  本文件不引入新功能，只把 main.py 中两行发送调用的实参逐个拆开说明：
#    send_rpcsy(client, init_cmds, sleep_s=0.1)         # 同步发送
#    send_rpc_async(client, moveabsj_cmd, wait_s=0)     # 异步发送
#
#  定义见 common/rpc/python/rpc_client.py：
#    send_rpcsy(client, cmd_list, sleep_s=0, debug=False, ...)
#    send_rpc_async(client, cmd_list, wait_s=0.5, debug=False, ...)
#
#  与 C++ 版（main_send_params.cpp）的对应关系：
#    * C++ 的模板实参 RespDemo 在 Python 没有对应物——Python 无模板，返回值
#      元素类型固定为 rpc.CommResp，只有 code / index / message 三个字段
#      （分别对应 JSON 的 return_code / subcmd_index / return_message）。
#      指令的扩展返回字段不会进返回值，需要时得自己取原始 JSON，
#      参考 ReadPdo_py / ReadSdo_py 的 send_read*_raw 写法。
#    * 间隔单位不同：Python 是秒（sleep_s / wait_s，float），C++ 是毫秒（int）。
#      C++ 里写 100，这边要写 0.1；照抄 100 会变成等 100 秒。
#    * 其余实参一一对应：client、指令列表、指令间隔、debug。
#
#  写法建议：可选实参一律用关键字形式（sleep_s= / debug= ），既不受位置顺序
#  影响，也不会把间隔误当成别的参数；前两个（client、cmd_list）是位置实参，
#  顺序不可调换。
# ==================================================================

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Recover}",
    "{Mode}",
    "{Enable}",
    "{Var --clear}",
    # "{Var --type=jointtarget --name=（变量名）
    #           --value={ jointtarget 共 10 位，不足补 0，单位：弧度 }}"
    "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}",
    "{Start}"
]

# MoveAbsJ 运动命令
moveabsj_cmd = [
    # "{MoveAbsJ --jointtarget_var=（关节目标变量名，需先在 init_cmds 中通过 Var 预定义）}"
    "{MoveAbsJ --jointtarget_var=j1}",
    "{MoveAbsJ --jointtarget_var=j2}",
    "{MoveAbsJ --jointtarget_var=j0}"
]


def main():
    """主函数"""

    # ==============================================================
    #  实参①：client —— 就是这里创建的 RpcClient 对象，同步/异步共用
    #  同一个实例。
    #    * 构造只需传控制器 IP，端口固定 5868（普通 RPC 字符串指令通道），
    #      构造完成即已尝试建连，用 is_connected() 判断是否成功。
    #    * 不需要每条指令新建连接。
    #    * 每次发指令前封装层都会检查 is_connected()，一旦发现断连，剩余
    #      指令直接中止并打印错误，不会静默丢指令。
    # ==============================================================
    client = RpcClient(ROBOT_IP)
    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # ==================================================================
    #  示例 1：同步发送 send_rpcsy —— 逐个实参说明
    # ==================================================================
    #
    #  send_rpcsy ( client , init_cmds , sleep_s=0.1 );
    #             └─ ① ─┘  └─── ② ───┘    └── ③ ──┘
    #
    #  ① client —— 上面建立的连接对象。
    #
    #  ② init_cmds —— 指令列表，按列表顺序逐条发送。
    #     每个字符串是一条完整指令：最外层 {} 包住整条指令，内部用
    #     "--参数=值" 传参。
    #     同步语义：发一条 → 阻塞等控制器返回该条结果 → 再发下一条，
    #     指令之间天然串行，运动类指令会等到跑完才继续。
    #     传空列表时一条也不发，返回空列表。
    #
    #  ③ sleep_s=0.1 —— 指令间隔，单位「秒」（默认 0）。
    #     含义是「上一条拿到返回后、发下一条之前」再额外等 0.1s，只作用在
    #     指令之间，最后一条之后不再延时。
    #     注意它不是等待时长/超时：把 0.1 改成 0 不会让指令更早结束，只是
    #     去掉指令之间的空隙；初始化类指令填 0 通常就够，留一点间隔是给
    #     控制器留出状态切换的余量。
    #
    #  ④ debug（本调用未写，默认 False）—— 是否让封装层自动打印每条指令的
    #     发送内容与返回结果（含 seq、耗时统计）。调试期打开最方便。
    #
    #  返回值 —— list[rpc.CommResp]：每条指令的响应元素被依次 extend 进同一个
    #     列表（平铺，不是按指令分组的二维结构）。元素字段名是 code / index /
    #     message，不是 JSON 里的 return_code / subcmd_index / return_message。
    #     多条指令时要按 index 对应回具体指令，不能固定取 [0]。
    # ==================================================================
    init_results = send_rpcsy(client, init_cmds, sleep_s=0.1)

    # 逐条打印初始化指令的返回码
    print(f"init_cmds responses: {len(init_results)}")
    for r in init_results:
        print(f"  index:{r.index}  code:{r.code}  message:{r.message}")

    # 同一调用的其它等价写法（按需启用）：
    #   send_rpcsy(client, init_cmds)                    # ③ 省略，间隔 0
    #   send_rpcsy(client, init_cmds, sleep_s=0)         # 同上，显式写 0
    #   send_rpcsy(client, init_cmds, sleep_s=0.1, debug=True)   # ④ 打开自动打印
    #   send_rpcsy(client, init_cmds, 0.1)               # 位置写法，效果同上，
    #                                                    #  但可读性差，不建议

    # ==================================================================
    #  示例 2：异步发送 send_rpc_async —— 逐个实参说明
    # ==================================================================
    #
    #  send_rpc_async ( client , moveabsj_cmd , wait_s=0 );
    #                 └─ ① ─┘  └───── ② ─────┘    └ ③ ┘
    #
    #  ① client —— 与同步共用同一个连接对象。
    #
    #  ② moveabsj_cmd —— 指令列表，格式与同步完全相同，同样按顺序逐条下发。
    #     异步语义：把指令写入连接就立刻返回，不等返回，所以几条指令会在很短
    #     时间内连续到达控制器，由控制器自行排队执行。
    #
    #  ③ wait_s=0 —— 指令间隔，单位「秒」（形参名 wait_s，默认 0.5），含义与
    #     同步的 sleep_s 一致：每条下发完到下一条之间的额外延时。
    #     注意默认值是 0.5 而不是 0，不写就等于每条之间等半秒，异步持续下发
    #     时这个差别很明显，建议显式写出。
    #     异步场景这个间隔才是「下发节奏」控制手段：0 表示尽快灌入，需要限制
    #     下发速率时（例如在线规划类指令按周期喂入）才写非 0 值。
    #     下面用 time.sleep(0.2) 在「整批指令」之间再留停顿，这一层延时是调用方
    #     自己加的，和 ③ 的指令间隔不是同一回事。
    #
    #  返回值 —— 无（None）。不等返回，拿不到解析后的结果；想知道执行结果只能
    #     用后面的 response_callback / timing_callback 可选实参，或改用同步发送。
    # ==================================================================

    # 持续发送 10 组指令
    for _ in range(10):
        send_rpc_async(client, moveabsj_cmd, wait_s=0)
        time.sleep(0.2)

    # 同一调用的其它等价写法（按需启用）：
    #   send_rpc_async(client, moveabsj_cmd)             # ③ 省略，间隔取默认 0.5s
    #   send_rpc_async(client, moveabsj_cmd, wait_s=0.1)  # ③ 每条间隔 0.1s 下发
    #   send_rpc_async(client, moveabsj_cmd, wait_s=0, debug=True)  # ④ 打印每条下发


# 程序入口
if __name__ == "__main__":
    main()
