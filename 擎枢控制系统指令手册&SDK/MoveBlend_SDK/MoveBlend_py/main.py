import sys, os, time
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'common', 'rpc', 'python')))
from rpc_client import RpcClient, send_rpcsy, send_rpc_async

ROBOT_IP = "192.168.11.11"

# 初始化命令列表
init_cmds = [
    "{Clear}",
    "{Disable}",
    "{Recover}",
    "{Mode}",
    "{Enable}",
    "{SetUsingSP --state=on}",   # 开启最优求解器（笛卡尔空间运动适配）
    "{Start}"
]

# MoveBlend 轨迹命令：first_insert 设置起点 -> insert_line/insert_circle 添加轨迹点 -> start 执行
moveblend_cmd = [
    # "{MoveBlend --type=first_insert}  设置轨迹起点
    #  {MoveBlend --type=insert_line   --robottarget_value={ x,y,z,q1,q2,q3,q4 } --zone={ 过渡区 } --speed=v50}  设置轨迹直线点
    #  {MoveBlend --type=insert_circle --robottarget_value={...} --zone={...} --speed=v50}  设置轨迹圆弧点
    #  {MoveBlend --type=start}  启动轨迹执行"
    "{MoveBlend --type=first_insert}",
    "{MoveBlend --type=insert_line --robottarget_value={0.036,0,0.9,0,0,0,1} --zone={0.1,0.1} --speed=v50}",
    "{MoveBlend --type=insert_line --robottarget_value={0.036,-0.068,0.9,0,0,0,1} --zone={0.1,0.1} --speed=v50}",
    "{MoveBlend --type=start}"
]   


def main():
    """主函数"""
    # 创建客户端连接机器人控制器
    client = RpcClient(ROBOT_IP)
    if not client.is_connected():
        print(f"Connection failed: {client.error_info()}")
        return

    # ==================================================================
    #  示例 1：通用同步 RPC（最常见用法）
    #  返回值只有 return_code / subcmd_index / return_message
    #  执行初始化
    # ==================================================================
    #  send_rpcsy(client, init_cmds, sleep_s=间隔秒, timeout_ms=超时毫秒)
    send_rpcsy(client, init_cmds, sleep_s=0.1, timeout_ms=50000)

    # ==================================================================
    #  示例 2：通用异步 RPC（不等返回，通过回调处理结果）
    #  for 循环重复下发 MoveBlend 轨迹序列
    # ==================================================================
    #  send_rpc_async(client, moveblend_cmd, wait_s=间隔秒, timeout_ms=超时毫秒)

    # 持续发送 10 组指令
    for _ in range(10):
        send_rpc_async(client, moveblend_cmd, wait_s=0, timeout_ms=50000)
        time.sleep(5)


# 程序入口
if __name__ == "__main__":
    main()
