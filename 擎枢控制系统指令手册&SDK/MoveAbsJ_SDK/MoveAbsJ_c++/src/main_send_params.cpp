#include "rpc_client.h"
#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <future>

// ==================================================================
//  main  ——  发送接口参数说明示例
//
//  本文件不引入新功能，只把 main.cpp 中两行发送调用的实参逐个拆开说明：
//    send_rpcsy<RespDemo>(client, init_cmds, 100);   // 同步发送
//    send_rpcAsy(client, moveabsj_cmd, 0);           // 异步发送
//
//  两个接口的形参一一对应，只是第 4 个实参名不同（同步 sleep_num、异步
//  wait_num），含义与单位完全一致。定义见
//  common/rpc/c++/include/message/rpc_client.h：
//
//    template<typename RespType>                              // ① 模板实参
//    std::vector<RespType> send_rpcsy(
//        cpp_rpc::CPPClient& client,                          // ② client
//        const std::vector<std::string>& cmd_cmd,             // ③ 指令列表
//        int sleep_num = 0,                                   // ④ 指令间隔ms
//        bool debug = false, ...);                            // ⑤ 调试打印
//
//    void send_rpcAsy(
//        cpp_rpc::CPPClient& client,                          // ② client
//        const std::vector<std::string>& cmd_cmd,             // ③ 指令列表
//        int wait_num = 0,                                    // ④ 指令间隔ms
//        bool debug = false, ...);                            // ⑤ 调试打印
//
//  ⑤ 之后还有 response_callback / timing_callback 等可选实参，本文件不涉及，
//  需要时直接查 rpc_client.h。
//
//  Python 对应版见 ../MoveAbsJ_py/main_send_params.py；两侧形参一一对应，
//  但间隔单位不同：C++ 是毫秒，Python 是秒（这里的 100 对应 Python 的 0.1）。
// ==================================================================

int main() {
    const std::string robot_ip = "192.168.11.11";

    // ---- 命令定义 --------------------------------------------------

    std::vector<std::string> init_cmds = {
        "{Clear}",
        "{Disable}",
        "{Recover}",
        "{Mode}",
        "{Enable}",
        "{Var --clear}",
    //    "{Var --type=jointtarget --name=（变量名）
    //                  --value={ jointtarget 共 10 位，不足补 0，单位：弧度 }}"
        "{Var --type=jointtarget --name=j0 --value={0,0,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j1 --value={0.1,-1.5,0,0,0,0,0,0,0,0}}",
        "{Var --type=jointtarget --name=j2 --value={0.2,0,0,0,0,0,0,0,0,0}}",
        "{Start}"
    };

    std::vector<std::string> moveabsj_cmd = {
    //    "{MoveAbsJ --jointtarget_var=（关节目标变量名，需先在 init_cmds 中通过 Var 预定义）}"
        "{MoveAbsJ --jointtarget_var=j1}",
        "{MoveAbsJ --jointtarget_var=j2}",
        "{MoveAbsJ --jointtarget_var=j0}"
    };

    // ---- 连接机器人控制器 -------------------------------------------
    //
    //  参数②：client —— 就是这里构造的 CPPClient 对象，两个发送接口都按
    //  引用接收它（cpp_rpc::CPPClient&）。
    //    * 构造参数为「控制器 IP + 端口」，5868 是普通 RPC 字符串指令通道；
    //      构造完成即已尝试建连，用 IsConnected() 判断是否成功。
    //    * 整个程序复用同一个 client 即可，不需要每条指令新建。
    //    * 每次发指令前封装层都会检查 IsConnected()，一旦发现连接断开，
    //      剩余指令直接中止并打印错误，因此 client 断连后不会静默丢指令。
    // ==================================================================

    std::cout << "Connecting: " << std::endl;
    cpp_rpc::CPPClient client(robot_ip, 5868);
    if (!client.IsConnected()) {
        std::cerr << "Connection failed! Aborting all commands." << std::endl;
        return -1;
    }
    std::cout << "Connected: " << std::endl;

    // ==================================================================
    //  示例 1：同步发送 send_rpcsy —— 逐个实参说明
    // ==================================================================
    //
    //  send_rpcsy <RespDemo> ( client , init_cmds , 100 );
    //             └── ① ──┘   └─ ② ─┘  └─── ③ ───┘  └④┘
    //
    //  ① 模板实参 RespDemo —— 决定返回值 vector 的元素类型，不是普通实参。
    //     RespDemo 只含 return_code / subcmd_index / return_message 三个通用
    //     字段，适用于 Clear / Enable / MoveAbsJ / Start / Stop 等绝大多数指令。
    //     指令带额外返回字段时要换成对应结构体，例如 ReadPdo 用 RespPdo：
    //        send_rpcsy<RespPdo>(client, cmds, 100);   // 返回值多一个 pdo_value
    //     可用结构与字段定义见 common/rpc/c++/include/message/resp_dto.h。
    //
    //  ② client —— 上面建立的连接对象，同步/异步共用同一个。
    //
    //  ③ init_cmds —— 指令列表，按 vector 顺序逐条发送。
    //     每个字符串是一条完整指令：最外层 {} 包住整条指令，内部用
    //     "--参数=值" 传参，一条字符串里的 {} 不能跨行拆分。
    //     同步语义：发一条 → 阻塞等控制器返回该条结果 → 再发下一条，
    //     所以指令之间天然串行，运动类指令会等到跑完才继续。
    //     传空 vector 时一条也不发，直接返回空结果。
    //
    //  ④ 100 —— 指令间隔，单位毫秒（形参名 sleep_num，默认 0）。
    //     含义是「上一条拿到返回后、发下一条之前」再额外等 100ms，
    //     只作用在指令之间，最后一条之后不再延时。
    //     注意这个实参不是等待时长/超时，把 100 改成 0 不会让指令更早结束，
    //     只是去掉指令之间的空隙；初始化类指令填 0 通常就够，
    //     留一点间隔是给控制器留出状态切换的余量。
    //
    //  返回值 —— std::vector<RespDemo>：每条指令的响应元素被依次追加进同一个
    //     vector（平铺，不是按指令分组的二维结构）。多条指令时要按元素里的
    //     subcmd_index 对应回具体指令，不能固定取 [0]。
    // ==================================================================

    auto init_results = send_rpcsy<RespDemo>(client, init_cmds, 100);

    // 逐条打印初始化指令的返回码
    std::cout << "init_cmds responses: " << init_results.size() << std::endl;
    for (const auto& r : init_results) {
        std::cout << "  subcmd_index:" << r.subcmd_index
                  << "  return_code:" << r.return_code
                  << "  return_message:" << r.return_message << std::endl;
    }

    // 同一调用的其它等价写法（按需启用）：
    //   send_rpcsy<RespDemo>(client, init_cmds);            // ④ 省略，间隔 0
    //   send_rpcsy<RespDemo>(client, init_cmds, 0);         // 同上，显式写 0
    //   send_rpcsy<RespDemo>(client, init_cmds, 100, true); // ⑤ debug=true，
    //                                                       //  封装层自动打印
    //                                                       //  发送内容与返回
    //   send_rpcsy<RespDemo>(client, init_cmds, 100, false); // ⑤ 显式静默（默认）

    // ==================================================================
    //  示例 2：异步发送 send_rpcAsy —— 逐个实参说明
    // ==================================================================
    //
    //  send_rpcAsy ( client , moveabsj_cmd , 0 );
    //              └─ ② ─┘  └──── ③ ─────┘  └④┘
    //
    //  ① 模板实参 —— 没有。异步接口返回 void，不等控制器回复，因此拿不到
    //     解析后的结构体，不需要也不能指定 RespType；需要结果时用后面的
    //     response_callback 可选实参（见 rpc_client.h）。
    //
    //  ② client —— 与同步共用同一个连接对象。
    //
    //  ③ moveabsj_cmd —— 指令列表，格式与同步完全相同，同样按顺序逐条下发。
    //     异步语义：把指令写入连接就立刻返回，不等返回，所以几条指令会在
    //     很短时间内连续到达控制器，由控制器自行排队执行。
    //
    //  ④ 0 —— 指令间隔，单位毫秒（形参名 wait_num，默认 0），含义与同步的
    //     sleep_num 一致：每条下发完到下一条之间的额外延时。
    //     异步场景这个间隔才是「下发节奏」控制手段：0 表示尽快灌入，
    //     需要限制下发速率时（例如在线规划类指令按周期喂入）才写非 0 值。
    //     下面用 delay_ms(200) 在「整批指令」之间再留停顿，这一层延时是调用方
    //     自己加的，和 ④ 的指令间隔不是同一回事。
    //
    //  返回值 —— 无（void）。想知道执行结果只能靠回调，或改用同步发送。
    // ==================================================================

    // 持续发送 10 组指令
    for (int i = 0; i < 10; ++i)
    {
        send_rpcAsy(client, moveabsj_cmd, 0);
        delay_ms(200);
    }

    // 同一调用的其它等价写法（按需启用）：
    //   send_rpcAsy(client, moveabsj_cmd);              // ④ 省略，间隔 0
    //   send_rpcAsy(client, moveabsj_cmd, 100);         // ④ 每条间隔 100ms 下发
    //   send_rpcAsy(client, moveabsj_cmd, 0, true);     // ⑤ debug=true，打印每条下发

    return 0;
}
