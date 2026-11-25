#ifndef ETHERCAT_READ_PDO_HPP
#define ETHERCAT_READ_PDO_HPP

// EthercatReadPDO 子系统 data 字段的线协议结构。
// 与已验证的 topic+pdo 示例逐字段核对一致：
//   16 字节头 + 24 个条目 x 24 字节 = 592 字节。
// 解析前必须校验 version/struct_size/count，防止布局不符时输出垃圾值。

#include "system_state_reader.hpp"

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

#pragma pack(push, 1)
struct EthercatPDOValue {
    int32_t  raw;                 // 实际整数值：8/16 位零扩展，32 位按有符号解释；失败保留历史值
    uint32_t slave_id;            // 从站池下标
    uint16_t index;               // 对象字典索引，如 0x6004
    uint8_t  sub_index;           // 子索引
    uint8_t  size;                // 位宽：8/16/32
    int32_t  read_rc;             // readPdo 返回码，0=成功；INT32_MIN=未调用
    uint32_t last_success_age_ms; // 距上次成功采样的毫秒数；UINT32_MAX=从未成功
    uint8_t  result;              // PDOReadResult: 0=pending 1=success 2=failed
    uint8_t  reason;              // PDOReadReason: 0=none 1=read_error 2=unavailable 3=offline 4=snapshot_error 5=stopped
    uint16_t reserved;
};

struct EthercatReadPDOData {
    uint16_t version;             // 协议版本，当前=1
    uint16_t count;               // 配置的采集对象数，1..24
    uint32_t struct_size;         // 结构体总长，当前=592
    uint64_t sample_seq;          // 采样批次序号，每完成一批 +1
    EthercatPDOValue values[24];
};
#pragma pack(pop)

static_assert(sizeof(EthercatPDOValue) == 24, "EthercatPDOValue must be 24 bytes");
static_assert(sizeof(EthercatReadPDOData) == 592, "EthercatReadPDOData must be 592 bytes");

// 校验子系统 data 是否为合法的 EthercatReadPDOData 报文。
inline bool isValidEthercatPDOData(const std::string& raw) {
    if (raw.size() < sizeof(EthercatReadPDOData)) return false;
    EthercatReadPDOData d;
    std::memcpy(&d, raw.data(), sizeof(d));
    if (d.version != 1 || d.struct_size != sizeof(EthercatReadPDOData) ||
        d.count < 1 || d.count > 24) return false;
    for (uint16_t i = 0; i < d.count; ++i) {
        const auto& v = d.values[i];
        if (v.size != 8 && v.size != 16 && v.size != 32) return false;
        if (v.result > 2 || v.reason > 5) return false;
    }
    return true;
}

inline const char* pdoResultName(uint8_t r) {
    switch (r) {
        case 0: return "pending";
        case 1: return "success";
        case 2: return "failed";
        default: return "unknown";
    }
}

inline const char* pdoReasonName(uint8_t r) {
    switch (r) {
        case 0: return "none";
        case 1: return "read_error";
        case 2: return "unavailable";
        case 3: return "offline";
        case 4: return "snapshot_error";
        case 5: return "stopped";
        default: return "unknown";
    }
}

// 从 NRT 快照中尝试读取指定子系统的 EthercatReadPDO 数据。
// 返回 false 表示无 NRT 数据、索引越界或报文不符合当前协议。
inline bool tryGetEthercatPDOData(size_t subsystem_idx, EthercatReadPDOData& out) {
    auto nrt = SystemStateReader::snapshotNrt();
    if (!nrt.valid() || subsystem_idx >= nrt.subsystemCount()) {
        return false;
    }
    const std::string& raw = nrt.subsystemRawData(subsystem_idx);
    if (!isValidEthercatPDOData(raw)) {
        return false;
    }
    out = nrt.parseSubsystemData<EthercatReadPDOData>(subsystem_idx);
    return true;
}

// 读取指定子系统的 EthercatReadPDO 数据；失败时抛出异常。
inline EthercatReadPDOData getEthercatPDOData(size_t subsystem_idx) {
    EthercatReadPDOData out;
    if (!tryGetEthercatPDOData(subsystem_idx, out)) {
        throw std::runtime_error(
            "Subsystem " + std::to_string(subsystem_idx) +
            " does not contain valid EthercatReadPDO data");
    }
    return out;
}

#endif // ETHERCAT_READ_PDO_HPP