#pragma once
// ==================================================================
//  resp_dto.h  ——  返回值结构体定义
//
//  规则：结构体字段名 必须 与 JSON 返回值键名 一字不差。
//        JSON 缺少某个字段 → 解析失败（CallAwait 返回错误码）
//        JSON 多出某个字段 → 自动忽略（不报错）
// ==================================================================

#include "easy_json.h"
#include <cstdint>
#include <vector>

// ==================================================================
//  指令 → 响应类型 对照表
// ==================================================================
//
//  指令                       响应类型                  额外字段
//  ────────────────────────   ──────────────────────    ────────────
//  Clear, Enable, MoveAbsJ,   RespDemo                 (无)
//  SpeedL, Stop, Start ...
//
//  PointChooseIDMove          PointChooseIDMoveResp    target_pq[7]
//
//  如果你的指令有额外字段，照下面的模板新增即可。

// ==================================================================
//  响应类型 ①：RespDemo — 通用（所有无扩展字段的指令）
// ==================================================================

struct RespDemo {
    int32_t return_code;
    int32_t subcmd_index;
    std::string return_message;
    JSON_HELP(subcmd_index, return_code, return_message);
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RespDemo, return_code, subcmd_index, return_message)

// ==================================================================
//  响应类型 ②：PointChooseIDMoveResp — 扩展了 target_pq
//
//  对应的 JSON:
//    {"return_code":0, "subcmd_index":0, "return_message":"",
//     "target_pq":[0.488,0.215,0.463,-0.016,0.734,0.625,0.263]}
//                                    ↑ 字段名和 JSON 键名必须一致
// ==================================================================

struct PointChooseIDMoveResp {
    int32_t return_code;
    int32_t subcmd_index;
    std::string return_message;
    std::vector<double> target_pq;   // 关节位姿 [x,y,z,qx,qy,qz,qw]（NotRunExecute 时为空）
    JSON_HELP(subcmd_index, return_code, return_message, target_pq);
};

// 自定义 from_json，让 target_pq 可选（JSON 中没有时不报错）
inline void to_json(nlohmann::json& j, const PointChooseIDMoveResp& r) {
    j = nlohmann::json{
        {"return_code", r.return_code},
        {"subcmd_index", r.subcmd_index},
        {"return_message", r.return_message},
        {"target_pq", r.target_pq}
    };
}
inline void from_json(const nlohmann::json& j, PointChooseIDMoveResp& r) {
    j.at("return_code").get_to(r.return_code);
    j.at("subcmd_index").get_to(r.subcmd_index);
    j.at("return_message").get_to(r.return_message);
    if (j.contains("target_pq") && !j.at("target_pq").is_null()) {
        j.at("target_pq").get_to(r.target_pq);
    }
}

// ==================================================================
//  打印接口（给 send_rpcsy 模板用）
//
//  RespDemo 不需要额外打印 → 空的默认实现
//  PointChooseIDMoveResp 需要打印 target_pq → 特化版本
//
//  新增响应类型时，照下面的模板加一个特化即可：
//
//  template<>
//  struct RespPrinter<你的类型> {
//      static void print_extra(const 你的类型& r) {
//          printf("你的字段名: ...\n", ...);
//      }
//  };
// ==================================================================

template<typename RespType>
struct RespPrinter {
    static void print_extra(const RespType&) {}   // 默认：不打印额外字段
};

template<>
struct RespPrinter<PointChooseIDMoveResp> {
    static void print_extra(const PointChooseIDMoveResp& r) {
        if (!r.target_pq.empty()) {
            printf("target_pq: [");
            for (size_t i = 0; i < r.target_pq.size(); ++i) {
                if (i > 0) printf(", ");
                printf("%.6f", r.target_pq[i]);
            }
            printf("]\n");
        }
    }
};

// ==================================================================
//  新增响应类型模板（复制以下代码块，改 3 处即可）
// ==================================================================
//
//  // ---- 步骤 1：定义结构体（字段名 = JSON 键名）------------------
//  struct YourCmdResp {
//      int32_t return_code;
//      int32_t subcmd_index;
//      std::string return_message;
//      int32_t your_field;              // 改成你的字段
//      JSON_HELP(subcmd_index, return_code, return_message, your_field);
//  };
//  NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(YourCmdResp, return_code, subcmd_index, return_message, your_field)
//
//  // ---- 步骤 2：特化打印（不需要打印的话可以跳过）-----------------
//  template<>
//  struct RespPrinter<YourCmdResp> {
//      static void print_extra(const YourCmdResp& r) {
//          printf("your_field: %d\n", r.your_field);
//      }
//  };
//
//  // ---- 步骤 3：在 main.cpp 中使用 -------------------------------
//  send_rpcsy<YourCmdResp>(client, cmds, 500, 100);
//
//  // ---- 步骤 4：在代码中拿到返回值 -------------------------------
//  auto res = client.CallAwait<YourCmdResp>(msg, timeout);
//  int val = res.second[0].your_field;   // 直接用
//
// ==================================================================
