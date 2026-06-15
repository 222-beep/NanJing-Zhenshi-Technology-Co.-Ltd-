import sys, os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async
import time

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Mode}",
    "{SetMaxToq}",
    "{Recover}",
    "{SetRate}",
    "{Enable}",
    "{Var --clear}",
    "{Recover}",
    "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j3 --value={-0.1,0,0,0,0,0,0,0,0,0}}",
    "{Var --type=jointtarget --name=j4 --value={-0.2,0,0,0,0,0,0,0,0,0}}",
]

# 同步示例指令（MoveAbsJ 依次到 j0~j4）
sync_cmds = [
    "{MoveAbsJ --jointtarget_var=j0}",
    "{MoveAbsJ --jointtarget_var=j1}",
    "{MoveAbsJ --jointtarget_var=j2}",
    "{MoveAbsJ --jointtarget_var=j3}",
    "{MoveAbsJ --jointtarget_var=j4}",
]

# 异步示例指令（SpeedL 往返 + Stop/Start）
async_cmds = [
    "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={0.01,0,0,0,0,0} --last_count=1000}",
    "{SpeedL --vel={-0.01,0,0,0,0,0} --last_count=1000}",
    "{Stop}",
    "{Start}",
]

ROBOT_IP = "192.168.2.145"


def demo_sync(client):
    """同步发送：每条等待响应后再发送下一条"""
    print(f"\n[同步发送] 开始，共 {len(sync_cmds)} 条指令")
    t0 = time.time()
    send_rpcsy(client, sync_cmds, 10000, 0.5)
    print(f"[同步发送] 完成，耗时 {time.time() - t0:.3f} 秒")


def demo_async(client):
    """异步发送：快速下发不阻塞等待"""
    print(f"\n[异步发送] 开始，共 {len(async_cmds)} 条指令")
    t0 = time.time()
    send_rpc_async(client, async_cmds, 10000, 0.5)
    print(f"[异步发送] 完成，耗时 {time.time() - t0:.3f} 秒")


def main():
    """主函数 - 同步发送与异步发送"""
    client = RpcClient(ROBOT_IP)

    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # 初始化
    send_rpcsy(client, init_cmds, 500, 0.1)
    print("初始化完成")

    while True:
        print("\n=== 同步 / 异步 发送 ===")
        print("sync    - 运行同步发送")
        print("async   - 运行异步发送")
        print("compare - 同步 vs 异步 对比")
        print("clear   - 清除所有变量")
        print("exit    - 退出程序")

        user_input = input("请输入命令: ").strip().lower()

        if user_input == "sync":
            demo_sync(client)

        elif user_input == "async":
            demo_async(client)

        elif user_input == "compare":
            demo_sync(client)
            demo_async(client)
            print("\n对比完成")

        elif user_input == "clear":
            send_rpcsy(client, ["{Var --clear}"], 5000, 0.5)
            print("所有变量已清除")

        elif user_input == "exit":
            print("退出程序...")
            send_rpcsy(client, ["{Stop --last_count=10}"], 5000, 1.0)
            break

        else:
            print("未知命令，请重新输入!")


# 程序入口
if __name__ == "__main__":
    main()
